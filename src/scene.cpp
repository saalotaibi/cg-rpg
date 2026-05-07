#include "scene.h"

#include <cstring>
#include <string>
#include <vector>

// =====================================================================
// Runtime item table.
//
// Originally this was a `static const ItemDef ITEMS[ITEM_COUNT]`. The
// onboarding wizard now lets the user customize their task list, so
// the table has to be mutable at runtime. Two storage vectors keep the
// layout stable:
//   - g_item_names: owns the heap-allocated name strings.
//   - g_items:      ItemDef table; .name pointers reference g_item_names.
// Both vectors stay parallel — index i of one matches index i of the
// other. We rebuild from scratch on every state_set_* call.
// =====================================================================
namespace {

std::vector<std::string> g_item_names;
std::vector<ItemDef>     g_items;

const ItemDef DEFAULT_ITEMS[] = {
    { "DRINK WATER",  CAT_LIFE,  1 },
    { "STRETCH",      CAT_BODY,  1 },
    { "DEEP WORK",    CAT_FOCUS, 2 },
    { "READ PAGES",   CAT_MIND,  2 },
    { "EXERCISE",     CAT_BODY,  2 },
    { "MEDITATE",     CAT_FOCUS, 1 },
    { "TIDY DESK",    CAT_LIFE,  1 },
    { "GO OUTSIDE",   CAT_LIFE,  2 },
};
const int DEFAULT_ITEM_COUNT =
    sizeof(DEFAULT_ITEMS) / sizeof(DEFAULT_ITEMS[0]);

void rebuild_table_from(const ItemDef* src, int n) {
    g_item_names.clear();
    g_items.clear();
    if (n > MAX_ITEMS) n = MAX_ITEMS;
    g_item_names.reserve((size_t)n);
    g_items.reserve((size_t)n);
    for (int i = 0; i < n; i++) {
        g_item_names.emplace_back(src[i].name ? src[i].name : "");
    }
    // Second pass: take pointers AFTER all strings are stored, so
    // earlier .data() pointers don't get invalidated by reallocations.
    for (int i = 0; i < n; i++) {
        ItemDef d = src[i];
        d.name = g_item_names[(size_t)i].c_str();
        g_items.push_back(d);
    }
}

void ensure_table_initialized() {
    if (g_items.empty()) {
        rebuild_table_from(DEFAULT_ITEMS, DEFAULT_ITEM_COUNT);
    }
}

} // namespace

const ItemDef& item_def(ItemId id) {
    ensure_table_initialized();
    int i = (int)id;
    if (i < 0) i = 0;
    if (i >= (int)g_items.size()) i = (int)g_items.size() - 1;
    return g_items[(size_t)i];
}

int item_count() {
    ensure_table_initialized();
    return (int)g_items.size();
}

void state_set_default_items() {
    rebuild_table_from(DEFAULT_ITEMS, DEFAULT_ITEM_COUNT);
}

void state_set_items(const ItemDef* items, int n) {
    if (!items || n <= 0) {
        state_set_default_items();
        return;
    }
    rebuild_table_from(items, n);
}

void state_init(GameState& gs) {
    gs.current_scene = SCENE_TITLE;

    gs.player_name[0]   = '\0';
    gs.character_index  = 0;

    // Default item table; onboarding (or profile load) may replace this.
    ensure_table_initialized();
    gs.item_count = item_count();
    std::memset(gs.item_done, 0, sizeof(gs.item_done));
    gs.last_unlocked = -1;
    gs.unlock_anim = 1.0f;

    gs.stats = { 0, 0, 0, 0 };
    gs.pomo = { POMO_IDLE, 0.0f, 0 };
    gs.health = 0.6f;

    gs.afk_seconds = 0.0f;
    gs.user_controlling = true;   // grace period: AI only takes over after AFK_THRESHOLD elapses

    gs.time_total = 0.0f;
    gs.show_help = false;

    // Onboarding sub-state.
    std::memset(&gs.onboard, 0, sizeof(gs.onboard));
    gs.onboard.step = ONB_NAME;
    gs.onboard.task_cursor = -1;
}

void state_complete_item(GameState& gs, ItemId id) {
    int i = (int)id;
    if (i < 0 || i >= gs.item_count) return;
    if (gs.item_done[i]) return;
    gs.item_done[i] = true;
    gs.last_unlocked = i;
    gs.unlock_anim = 0.0f;

    const ItemDef& d = item_def(id);
    switch (d.category) {
        case CAT_FOCUS: gs.stats.focus    += d.stat_gain; break;
        case CAT_BODY:  gs.stats.strength += d.stat_gain; break;
        case CAT_MIND:  gs.stats.iq       += d.stat_gain; break;
        case CAT_LIFE:  gs.stats.streak   += d.stat_gain; break;
    }
    gs.health += 0.08f;
    if (gs.health > 1.0f) gs.health = 1.0f;
}

float state_overall_progress(const GameState& gs) {
    if (gs.item_count <= 0) return 0.0f;
    int done = 0;
    for (int i = 0; i < gs.item_count; i++) if (gs.item_done[i]) done++;
    return (float)done / (float)gs.item_count;
}

void state_pomo_start(GameState& gs) {
    if (gs.pomo.phase == POMO_IDLE) {
        gs.pomo.phase = POMO_FOCUS;
        gs.pomo.remaining_sec = POMO_FOCUS_SECONDS;
    }
    // If paused mid-phase (remaining_sec preserved), starting just resumes.
}

void state_pomo_pause(GameState& gs) {
    if (gs.pomo.phase != POMO_IDLE) {
        // We model "pause" by snapping back to IDLE while preserving
        // remaining_sec so a subsequent start resumes.
        // Caller can distinguish "paused mid-focus" by remaining_sec > 0.
        gs.pomo.phase = POMO_IDLE;
    }
}

void state_pomo_reset(GameState& gs) {
    gs.pomo.phase = POMO_IDLE;
    gs.pomo.remaining_sec = 0.0f;
}

void state_pomo_tick(GameState& gs, float dt) {
    if (gs.pomo.phase == POMO_IDLE) return;
    gs.pomo.remaining_sec -= dt;
    if (gs.pomo.remaining_sec > 0.0f) return;

    if (gs.pomo.phase == POMO_FOCUS) {
        gs.pomo.sessions_completed++;
        gs.stats.focus += 1;
        gs.health += 0.05f;
        if (gs.health > 1.0f) gs.health = 1.0f;
        gs.pomo.phase = POMO_BREAK;
        gs.pomo.remaining_sec = POMO_BREAK_SECONDS;
    } else { // POMO_BREAK
        gs.pomo.phase = POMO_IDLE;
        gs.pomo.remaining_sec = 0.0f;
    }
}

Layout layout_compute(int win_w, int win_h) {
    (void)win_w;
    (void)win_h;

    Layout L{};
    L.win_w = WIN_DEFAULT_W;
    L.win_h = WIN_DEFAULT_H;

    // Side panel: fixed 360px wide on the right.
    const int PANEL_W = 360;
    L.panel_x = WIN_DEFAULT_W - PANEL_W;
    L.panel_y = 0;
    L.panel_w = PANEL_W;
    L.panel_h = WIN_DEFAULT_H;

    // Home backdrop: the rest, with a small inset.
    const int INSET = 12;
    L.home_x = INSET;
    L.home_y = INSET;
    L.home_w = (WIN_DEFAULT_W - PANEL_W) - 2 * INSET;
    L.home_h = WIN_DEFAULT_H - 2 * INSET;
    return L;
}
