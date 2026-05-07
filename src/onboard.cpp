#include "onboard.h"

#include "font.h"
#include "sprite.h"
#include "texture.h"
#include "textures.h"

#include <GL/glew.h>
#include <GL/glut.h>

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// =====================================================================
// Onboarding wizard.
//
// Visual style mirrors ui_draw_title (ui.cpp): warm dark gradient,
// big chunky pixel-font headings, golden accents.
//
// The carousel loads the 5 character sheets shipped under
// assets/textures/characters/0[1-5].png. We render each at 2x scale
// from the idle-down cell of its 32x64 cell layout.
// =====================================================================

// ---------------- Carousel character sheets --------------------------

namespace {

const char* CHARACTER_FILES[ONB_NUM_CHARACTERS] = {
    "assets/textures/characters/01.png",
    "assets/textures/characters/02.png",
    "assets/textures/characters/03.png",
    "assets/textures/characters/04.png",
    "assets/textures/characters/05.png",
};

Texture g_carousel_tex[ONB_NUM_CHARACTERS] = {};
bool    g_carousel_loaded = false;

struct Rect {
    float x, y, w, h;
};

bool inside_rect(const Rect& r, float x, float y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

void ensure_carousel_loaded() {
    if (g_carousel_loaded) return;
    g_carousel_loaded = true;
    for (int i = 0; i < ONB_NUM_CHARACTERS; i++) {
        if (!texture_load(&g_carousel_tex[i], CHARACTER_FILES[i])) {
            std::fprintf(stderr, "onboard: missing %s\n", CHARACTER_FILES[i]);
            // Fall back to the main character.png so the slot isn't blank.
            g_carousel_tex[i] = g_tex.character;
        }
    }
}

// ---------------- Drawing primitives ---------------------------------

void draw_rect(float x, float y, float w, float h,
               float r, float g, float b, float a = 1.0f) {
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x,     y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
}

void draw_outline(float x, float y, float w, float h,
                  float r, float g, float b, float thick = 2.0f) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(r, g, b);
    glLineWidth(thick);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x + 0.5f,     y + 0.5f);
    glVertex2f(x + w - 0.5f, y + 0.5f);
    glVertex2f(x + w - 0.5f, y + h - 0.5f);
    glVertex2f(x + 0.5f,     y + h - 0.5f);
    glEnd();
    glLineWidth(1.0f);
}

void draw_gradient_bg(int win_w, int win_h) {
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor3f(0.10f, 0.12f, 0.25f); glVertex2f(0,            0);
    glColor3f(0.10f, 0.12f, 0.25f); glVertex2f((float)win_w, 0);
    glColor3f(0.30f, 0.20f, 0.40f); glVertex2f((float)win_w, (float)win_h);
    glColor3f(0.30f, 0.20f, 0.40f); glVertex2f(0,            (float)win_h);
    glEnd();
}

void draw_centered(float y, float scale, const char* text,
                   float r, float g, float b, int win_w) {
    float w = font_width(scale, text);
    font_draw(((float)win_w - w) * 0.5f, y, scale, text, r, g, b);
}

// Pulses 0..1 from a sine, used for the cursor / hint blinking.
float pulse01(float t, float hz = 1.5f) {
    float s = std::sin(t * hz * 6.2831853f);
    return 0.5f + 0.5f * s;
}

// ---------------- Footer hint bar ------------------------------------

void draw_footer(int win_w, const char* hint) {
    draw_centered(40.0f, 1.6f, hint, 0.85f, 0.85f, 0.95f, win_w);
}

// ---------------- Step 1: name --------------------------------------

void draw_step_name(const GameState& gs, int win_w, int win_h) {
    draw_centered(win_h * 0.78f, 5.0f, "WHATS YOUR NAME",
                  1.0f, 0.85f, 0.45f, win_w);
    draw_centered(win_h * 0.70f, 2.0f, "TYPE A Z UPPERCASE   ENTER WHEN DONE",
                  0.85f, 0.80f, 0.90f, win_w);

    // Input box.
    const float box_w = 540.0f;
    const float box_h = 90.0f;
    float box_x = (win_w - box_w) * 0.5f;
    float box_y = win_h * 0.45f;

    draw_rect(box_x, box_y, box_w, box_h, 0.10f, 0.08f, 0.18f, 1.0f);
    draw_outline(box_x, box_y, box_w, box_h, 0.95f, 0.80f, 0.45f, 2.0f);

    const char* shown = gs.player_name[0] ? gs.player_name : "";
    float scale = 4.0f;
    float tw = font_width(scale, shown);
    float text_x = box_x + 24.0f;
    float text_y = box_y + (box_h - 7 * scale) * 0.5f;
    font_draw(text_x, text_y, scale, shown, 1.0f, 0.95f, 0.85f);

    // Blinking caret immediately after the text.
    float caret_x = text_x + tw + 4.0f;
    float caret_alpha = pulse01(gs.time_total, 1.8f);
    draw_rect(caret_x, text_y, scale, 7 * scale,
              1.0f, 0.95f, 0.55f, caret_alpha);

    draw_footer(win_w, "ENTER CONTINUE   BACKSPACE DELETE   ESC QUIT");
}

// ---------------- Step 2: character carousel -------------------------

// Returns the bottom-left x/y and size used to draw a single character
// preview at 2x scale (32x64 source -> 64x128 dest).
struct CarouselSlot {
    float x, y;
    float w, h;
};

CarouselSlot carousel_slot(int win_w, int win_h, int relative_index) {
    // 5 slots side-by-side, centered on the selected one. The selected
    // slot draws at 3x scale, neighbors at 2x, with horizontal padding.
    (void)win_h;
    CarouselSlot s{};
    s.w = 96.0f;            // 32 * 3
    s.h = 192.0f;           // 64 * 3
    if (relative_index != 0) { s.w = 64.0f; s.h = 128.0f; }
    float center_x = win_w * 0.5f;
    float gap = 130.0f;
    s.x = center_x + relative_index * gap - s.w * 0.5f;
    s.y = win_h * 0.40f;
    return s;
}

void draw_step_character(const GameState& gs, int win_w, int win_h) {
    ensure_carousel_loaded();

    draw_centered(win_h * 0.85f, 5.0f, "PICK A CHARACTER",
                  1.0f, 0.85f, 0.45f, win_w);
    draw_centered(win_h * 0.78f, 2.0f, "ARROWS TO SCROLL   ENTER TO PICK",
                  0.85f, 0.80f, 0.90f, win_w);

    int sel = gs.onboard.char_index;
    if (sel < 0) sel = 0;
    if (sel >= ONB_NUM_CHARACTERS) sel = ONB_NUM_CHARACTERS - 1;

    // 32x64 cell layout: idle-down lives at column 3, row 0 (see home.cpp).
    const int CHAR_W = 32, CHAR_H = 64;
    const int IDLE_DOWN_COL = 3;

    // Render the 5 slots: left-2, left-1, center, right-1, right-2.
    for (int rel = -2; rel <= 2; rel++) {
        int idx = sel + rel;
        if (idx < 0 || idx >= ONB_NUM_CHARACTERS) continue;
        CarouselSlot s = carousel_slot(win_w, win_h, rel);

        // Background card for the slot.
        float bg_pad = 14.0f;
        float bg_x = s.x - bg_pad;
        float bg_y = s.y - bg_pad - 24.0f;     // extra room for label
        float bg_w = s.w + 2 * bg_pad;
        float bg_h = s.h + 2 * bg_pad + 28.0f;

        if (rel == 0) {
            draw_rect(bg_x, bg_y, bg_w, bg_h, 0.18f, 0.14f, 0.25f, 1.0f);
            draw_outline(bg_x, bg_y, bg_w, bg_h, 1.0f, 0.85f, 0.45f, 3.0f);
        } else {
            draw_rect(bg_x, bg_y, bg_w, bg_h, 0.13f, 0.10f, 0.20f, 0.85f);
            draw_outline(bg_x, bg_y, bg_w, bg_h, 0.55f, 0.45f, 0.30f, 2.0f);
        }

        // Sprite.
        glColor4f(1, 1, 1, rel == 0 ? 1.0f : 0.85f);
        sprite_draw(g_carousel_tex[idx],
                    s.x, s.y, s.w, s.h,
                    IDLE_DOWN_COL * CHAR_W, 0, CHAR_W, CHAR_H);

        // "k/N" label below.
        char label[16];
        std::snprintf(label, sizeof(label), "%d/%d",
                      idx + 1, ONB_NUM_CHARACTERS);
        float lab_scale = (rel == 0) ? 2.5f : 1.8f;
        float lw = font_width(lab_scale, label);
        font_draw(s.x + (s.w - lw) * 0.5f,
                  bg_y + 10,
                  lab_scale, label,
                  rel == 0 ? 1.0f : 0.85f,
                  rel == 0 ? 0.95f : 0.80f,
                  rel == 0 ? 0.55f : 0.55f);
    }

    // Pulsing arrow hints either side of center.
    float pulse = pulse01(gs.time_total, 1.5f);
    float ar = 0.6f + 0.4f * pulse;
    draw_centered(win_h * 0.40f - 200.0f, 2.0f,
                  "LEFT          RIGHT",
                  ar, ar * 0.95f, 0.55f, win_w);

    draw_footer(win_w, "ARROWS SCROLL   ENTER PICK   BACKSPACE BACK");
}

// ---------------- Step 3: tasks --------------------------------------

void category_color(ItemCategory c, float& r, float& g, float& b) {
    switch (c) {
        case CAT_FOCUS: r = 0.55f; g = 0.80f; b = 1.00f; break;
        case CAT_BODY:  r = 1.00f; g = 0.55f; b = 0.55f; break;
        case CAT_MIND:  r = 0.75f; g = 0.65f; b = 1.00f; break;
        case CAT_LIFE:  r = 1.00f; g = 0.85f; b = 0.45f; break;
    }
}

const char* category_short(ItemCategory c) {
    switch (c) {
        case CAT_FOCUS: return "FOCUS";
        case CAT_BODY:  return "BODY";
        case CAT_MIND:  return "MIND";
        case CAT_LIFE:  return "LIFE";
    }
    return "?";
}

const int MAX_TASK_POINTS = 5;

struct TaskLayout {
    Rect list;
    Rect rows[MAX_ITEMS + 1];
    int row_count;
    int add_index;
    Rect start_button;
    float row_h;
    float name_scale;
    float meta_scale;
};

bool has_add_task_row(const OnboardState& O) {
    return O.draft_count < MAX_ITEMS;
}

bool cursor_is_real_task(const OnboardState& O) {
    return O.task_cursor >= 0 && O.task_cursor < O.draft_count;
}

bool cursor_is_add_task_row(const OnboardState& O) {
    return has_add_task_row(O) && O.task_cursor == O.draft_count;
}

void clamp_task_cursor(OnboardState& O) {
    int max_cursor = has_add_task_row(O) ? O.draft_count : O.draft_count - 1;
    if (max_cursor < 0) max_cursor = 0;
    if (O.task_cursor < 0) O.task_cursor = 0;
    if (O.task_cursor > max_cursor) O.task_cursor = max_cursor;
}

int nonempty_task_count(const OnboardState& O) {
    int n = 0;
    for (int i = 0; i < O.draft_count; i++) {
        if (O.draft_names[i][0] != '\0') n++;
    }
    return n;
}

TaskLayout compute_task_layout(int win_w, int win_h, int draft_count) {
    TaskLayout L{};
    L.add_index = (draft_count < MAX_ITEMS) ? draft_count : -1;
    L.row_count = draft_count + (L.add_index >= 0 ? 1 : 0);
    if (L.row_count < 1) L.row_count = 1;

    L.list.w = 760.0f;
    L.list.h = win_h * 0.56f;
    L.list.x = (win_w - L.list.w) * 0.5f;
    L.list.y = win_h * 0.20f;

    const float top_pad = 12.0f;
    const float row_gap = 3.0f;
    L.row_h = (L.list.h - top_pad * 2.0f) / (float)L.row_count;
    if (L.row_h > 36.0f) L.row_h = 36.0f;
    if (L.row_h < 24.0f) L.row_h = 24.0f;
    L.name_scale = (L.row_h < 30.0f) ? 1.55f : 1.8f;
    L.meta_scale = (L.row_h < 30.0f) ? 1.35f : 1.55f;

    float row_y_top = L.list.y + L.list.h - top_pad;
    for (int i = 0; i < L.row_count; i++) {
        L.rows[i] = {
            L.list.x + 12.0f,
            row_y_top - L.row_h * (i + 1),
            L.list.w - 24.0f,
            L.row_h - row_gap
        };
    }

    L.start_button = {
        win_w * 0.5f - 92.0f,
        L.list.y - 64.0f,
        184.0f,
        42.0f
    };
    return L;
}

void draw_step_tasks(const GameState& gs, int win_w, int win_h) {
    draw_centered(win_h * 0.90f, 4.5f, "CUSTOMIZE YOUR TASKS",
                  1.0f, 0.85f, 0.45f, win_w);
    draw_centered(win_h * 0.84f, 1.8f,
                  "TYPE TO EDIT   ALT ARROWS CHANGE TYPE AND POINTS",
                  0.85f, 0.80f, 0.90f, win_w);

    const OnboardState& O = gs.onboard;
    TaskLayout TL = compute_task_layout(win_w, win_h, O.draft_count);

    draw_rect(TL.list.x, TL.list.y, TL.list.w, TL.list.h,
              0.13f, 0.10f, 0.20f, 1.0f);
    draw_outline(TL.list.x, TL.list.y, TL.list.w, TL.list.h,
                 0.85f, 0.72f, 0.40f, 2.0f);

    for (int i = 0; i < TL.row_count; i++) {
        Rect r = TL.rows[i];
        bool is_add = (i == TL.add_index);
        bool selected = (O.task_cursor == i);

        if (selected) {
            draw_rect(r.x, r.y, r.w, r.h, 0.30f, 0.25f, 0.40f, 1.0f);
            draw_outline(r.x, r.y, r.w, r.h, 1.0f, 0.95f, 0.55f, 2.0f);
        } else if (is_add) {
            draw_rect(r.x, r.y, r.w, r.h, 0.11f, 0.09f, 0.17f, 0.95f);
            draw_outline(r.x, r.y, r.w, r.h, 0.45f, 0.42f, 0.55f, 1.0f);
        } else if (i % 2 == 0) {
            draw_rect(r.x, r.y, r.w, r.h, 0.18f, 0.14f, 0.24f, 0.65f);
        }

        float tag_w = 14.0f;
        float cr = 1, cg = 1, cb = 1;
        if (is_add) {
            draw_rect(r.x + 6.0f, r.y + 4.0f, tag_w, r.h - 8.0f,
                      0.45f, 0.42f, 0.55f, 1.0f);
            font_draw(r.x + 6.0f + tag_w + 12.0f,
                      r.y + (r.h - 7 * TL.name_scale) * 0.5f,
                      TL.name_scale, "+ NEW TASK",
                      0.72f, 0.70f, 0.86f);
            continue;
        }

        const ItemDef& d = O.draft_items[i];
        category_color(d.category, cr, cg, cb);
        draw_rect(r.x + 6.0f, r.y + 4.0f, tag_w, r.h - 8.0f,
                  cr, cg, cb, 1.0f);

        const char* name = d.name ? d.name : "";
        float name_x = r.x + 6.0f + tag_w + 12.0f;
        float name_y = r.y + (r.h - 7 * TL.name_scale) * 0.5f;
        font_draw(name_x, name_y, TL.name_scale, name,
                  1.0f, 0.95f, 0.85f);
        if (selected) {
            float caret_x = name_x + font_width(TL.name_scale, name) + 4.0f;
            float a = 0.35f + 0.65f * pulse01(gs.time_total, 1.8f);
            draw_rect(caret_x, name_y, 3.0f, 7.0f * TL.name_scale,
                      1.0f, 0.95f, 0.55f, a);
        }

        char buf[32];
        std::snprintf(buf, sizeof(buf), "+%d %s",
                      d.stat_gain, category_short(d.category));
        float gw = font_width(TL.meta_scale, buf);
        font_draw(r.x + r.w - 12.0f - gw,
                  r.y + (r.h - 7 * TL.meta_scale) * 0.5f,
                  TL.meta_scale, buf, cr, cg, cb);
    }

    char info[96];
    std::snprintf(info, sizeof(info),
                  "%d/%d TASKS   CLICK START WHEN READY",
                  O.draft_count, MAX_ITEMS);
    draw_centered(TL.list.y + TL.list.h + 18.0f, 1.8f, info,
                  0.95f, 0.85f, 0.55f, win_w);

    bool can_start = nonempty_task_count(O) > 0;
    if (can_start) {
        draw_rect(TL.start_button.x, TL.start_button.y,
                  TL.start_button.w, TL.start_button.h,
                  0.54f, 0.85f, 0.36f, 1.0f);
        draw_outline(TL.start_button.x, TL.start_button.y,
                     TL.start_button.w, TL.start_button.h,
                     1.0f, 0.95f, 0.55f, 2.0f);
    } else {
        draw_rect(TL.start_button.x, TL.start_button.y,
                  TL.start_button.w, TL.start_button.h,
                  0.24f, 0.22f, 0.28f, 1.0f);
        draw_outline(TL.start_button.x, TL.start_button.y,
                     TL.start_button.w, TL.start_button.h,
                     0.45f, 0.42f, 0.55f, 2.0f);
    }
    const char* start_label = "START";
    float sw = font_width(2.4f, start_label);
    font_draw(TL.start_button.x + (TL.start_button.w - sw) * 0.5f,
              TL.start_button.y + (TL.start_button.h - 7.0f * 2.4f) * 0.5f,
              2.4f, start_label,
              can_start ? 0.08f : 0.72f,
              can_start ? 0.14f : 0.70f,
              can_start ? 0.10f : 0.80f);

    draw_footer(win_w, "TYPE NAME   ALT ARROWS EDIT   ALT BACKSPACE DELETE");
}

// ---------------- Helpers --------------------------------------------

bool apply_drafts_to_state(GameState& gs) {
    OnboardState& O = gs.onboard;
    ItemDef clean[MAX_ITEMS];
    int clean_count = 0;

    for (int i = 0; i < O.draft_count; i++) {
        if (O.draft_names[i][0] == '\0') continue;
        O.draft_items[i].name = O.draft_names[i];
        if (O.draft_items[i].stat_gain < 1) O.draft_items[i].stat_gain = 1;
        if (O.draft_items[i].stat_gain > MAX_TASK_POINTS) {
            O.draft_items[i].stat_gain = MAX_TASK_POINTS;
        }
        clean[clean_count++] = O.draft_items[i];
    }
    if (clean_count <= 0) return false;

    state_set_items(clean, clean_count);
    gs.item_count = item_count();
    std::memset(gs.item_done, 0, sizeof(gs.item_done));
    gs.last_unlocked = -1;
    gs.unlock_anim = 1.0f;
    return true;
}

void seed_default_drafts(OnboardState& O) {
    O.draft_count = item_count();
    if (O.draft_count > MAX_ITEMS) O.draft_count = MAX_ITEMS;
    for (int i = 0; i < O.draft_count; i++) {
        const ItemDef& d = item_def((ItemId)i);
        std::strncpy(O.draft_names[i], d.name ? d.name : "",
                     ONB_MAX_TASK_NAME);
        O.draft_names[i][ONB_MAX_TASK_NAME] = '\0';
        O.draft_items[i].name = O.draft_names[i];
        O.draft_items[i].category = d.category;
        O.draft_items[i].stat_gain = d.stat_gain;
    }
    clamp_task_cursor(O);
}

void delete_task(OnboardState& O, int idx) {
    if (idx < 0 || idx >= O.draft_count) return;
    for (int i = idx; i + 1 < O.draft_count; i++) {
        std::strncpy(O.draft_names[i], O.draft_names[i + 1],
                     ONB_MAX_TASK_NAME);
        O.draft_names[i][ONB_MAX_TASK_NAME] = '\0';
        O.draft_items[i] = O.draft_items[i + 1];
        O.draft_items[i].name = O.draft_names[i];
    }
    O.draft_count--;
    clamp_task_cursor(O);
}

void append_task_char(OnboardState& O, int idx, char c) {
    if (idx < 0 || idx >= O.draft_count) return;
    int len = (int)std::strlen(O.draft_names[idx]);
    if (len >= ONB_MAX_TASK_NAME) return;
    if (c == ' ' && len == 0) return;
    O.draft_names[idx][len] = c;
    O.draft_names[idx][len + 1] = '\0';
    O.draft_items[idx].name = O.draft_names[idx];
}

void backspace_task_char(OnboardState& O, int idx) {
    if (idx < 0 || idx >= O.draft_count) return;
    int len = (int)std::strlen(O.draft_names[idx]);
    if (len <= 0) return;
    O.draft_names[idx][len - 1] = '\0';
    O.draft_items[idx].name = O.draft_names[idx];
}

void add_task_from_char(OnboardState& O, char c) {
    if (O.draft_count >= MAX_ITEMS) return;
    int idx = O.draft_count++;
    O.draft_names[idx][0] = '\0';
    O.draft_items[idx].name = O.draft_names[idx];
    O.draft_items[idx].category = CAT_LIFE;
    O.draft_items[idx].stat_gain = 1;
    O.task_cursor = idx;
    append_task_char(O, idx, c);
}

bool is_task_char(unsigned char k) {
    return std::isalnum(k) || k == ' ';
}

void cycle_selected_category(OnboardState& O, int dir) {
    if (!cursor_is_real_task(O)) return;
    int v = (int)O.draft_items[O.task_cursor].category + dir;
    if (v < 0) v = 3;
    if (v > 3) v = 0;
    O.draft_items[O.task_cursor].category = (ItemCategory)v;
}

void adjust_selected_points(OnboardState& O, int delta) {
    if (!cursor_is_real_task(O)) return;
    int v = O.draft_items[O.task_cursor].stat_gain + delta;
    if (v < 1) v = 1;
    if (v > MAX_TASK_POINTS) v = MAX_TASK_POINTS;
    O.draft_items[O.task_cursor].stat_gain = v;
}

void complete_tasks_if_ready(GameState& gs) {
    if (apply_drafts_to_state(gs)) {
        gs.onboard.step = ONB_DONE;
    }
}

} // namespace

// =====================================================================
// Public API
// =====================================================================

int onboard_character_count() { return ONB_NUM_CHARACTERS; }

const char* onboard_character_path(int index) {
    if (index < 0 || index >= ONB_NUM_CHARACTERS) return CHARACTER_FILES[0];
    return CHARACTER_FILES[index];
}

void onboard_apply_character(int index) {
    if (index < 0 || index >= ONB_NUM_CHARACTERS) index = 0;
    const char* src = CHARACTER_FILES[index];
    const char* dst = "assets/textures/character.png";

    // Plain binary copy. Cheaper than dragging in any PNG-encoder code.
    FILE* in  = std::fopen(src, "rb");
    FILE* out = std::fopen(dst, "wb");
    if (in && out) {
        unsigned char buf[8192];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), in)) > 0) {
            std::fwrite(buf, 1, n, out);
        }
    }
    if (in)  std::fclose(in);
    if (out) std::fclose(out);

    // Reload the live texture so the running game sees the new sprite.
    Texture fresh{};
    if (texture_load(&fresh, dst)) {
        texture_free(&g_tex.character);
        g_tex.character = fresh;
    }
}

void onboard_init(GameState& gs) {
    OnboardState& O = gs.onboard;
    std::memset(&O, 0, sizeof(O));
    O.step = ONB_NAME;
    O.char_index = 0;
    O.task_cursor = -1;
    seed_default_drafts(O);
    O.task_cursor = O.draft_count;
    clamp_task_cursor(O);
}

bool onboard_complete(const GameState& gs) {
    return gs.onboard.step == ONB_DONE;
}

void onboard_draw(const GameState& gs, int win_w, int win_h) {
    draw_gradient_bg(win_w, win_h);

    // Step counter pill in the top-left.
    {
        int n = (int)gs.onboard.step + 1;
        if (n > 3) n = 3;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "STEP %d OF 3", n);
        font_draw(40.0f, win_h - 50.0f, 2.5f, buf,
                  1.0f, 0.85f, 0.55f);
    }

    switch (gs.onboard.step) {
        case ONB_NAME:      draw_step_name(gs, win_w, win_h); break;
        case ONB_CHARACTER: draw_step_character(gs, win_w, win_h); break;
        case ONB_TASKS:     draw_step_tasks(gs, win_w, win_h); break;
        case ONB_DONE:      break;
    }
}

// ---------------- Input handling -------------------------------------

static void handle_name_key(GameState& gs, unsigned char k) {
    int len = (int)std::strlen(gs.player_name);
    if (k == '\r' || k == '\n') {
        if (len > 0) {
            gs.onboard.step = ONB_CHARACTER;
        }
        return;
    }
    if (k == 8 || k == 127) { // backspace / delete
        if (len > 0) gs.player_name[len - 1] = '\0';
        return;
    }
    if (k == 27) { // escape exits the program upstream; we ignore here.
        return;
    }
    // Accept letters (auto-uppercase) and space.
    if (std::isalpha((unsigned char)k) || k == ' ') {
        if (len < ONB_MAX_NAME) {
            char c = (char)std::toupper((unsigned char)k);
            gs.player_name[len] = c;
            gs.player_name[len + 1] = '\0';
        }
    }
}

static void handle_character_key(GameState& gs, unsigned char k) {
    if (k == '\r' || k == '\n') {
        gs.character_index = gs.onboard.char_index;
        onboard_apply_character(gs.character_index);
        gs.onboard.step = ONB_TASKS;
        return;
    }
    if (k == 8 || k == 127) { // backspace = back to name step
        gs.onboard.step = ONB_NAME;
        return;
    }
    // Letter shortcuts: 1-5 to pick directly.
    if (k >= '1' && k <= '5') {
        int idx = k - '1';
        if (idx < ONB_NUM_CHARACTERS) gs.onboard.char_index = idx;
    }
}

static void handle_tasks_key(GameState& gs, unsigned char k) {
    OnboardState& O = gs.onboard;
    clamp_task_cursor(O);
    int mods = glutGetModifiers();
    bool alt = (mods & GLUT_ACTIVE_ALT) != 0;

    if (k == '\t') {
        complete_tasks_if_ready(gs);
        return;
    }
    if (k == 8 || k == 127) {
        if (alt) {
            if (cursor_is_real_task(O)) delete_task(O, O.task_cursor);
        } else if (cursor_is_real_task(O)) {
            backspace_task_char(O, O.task_cursor);
        }
        return;
    }

    if (k == '\r' || k == '\n') return;
    if (!is_task_char(k)) return;

    char up = (char)std::toupper((unsigned char)k);
    if (cursor_is_add_task_row(O)) {
        if (up != ' ') add_task_from_char(O, up);
        return;
    }
    if (cursor_is_real_task(O)) {
        append_task_char(O, O.task_cursor, up);
        return;
    }
}

void onboard_key(GameState& gs, unsigned char k) {
    switch (gs.onboard.step) {
        case ONB_NAME:      handle_name_key(gs, k); break;
        case ONB_CHARACTER: handle_character_key(gs, k); break;
        case ONB_TASKS:     handle_tasks_key(gs, k); break;
        case ONB_DONE:      break;
    }
}

void onboard_special(GameState& gs, int special_key) {
    OnboardState& O = gs.onboard;
    switch (gs.onboard.step) {
        case ONB_CHARACTER:
            if (special_key == GLUT_KEY_LEFT) {
                if (O.char_index > 0) O.char_index--;
            } else if (special_key == GLUT_KEY_RIGHT) {
                if (O.char_index + 1 < ONB_NUM_CHARACTERS) O.char_index++;
            }
            break;
        case ONB_TASKS:
            clamp_task_cursor(O);
            {
                int mods = glutGetModifiers();
                bool alt = (mods & GLUT_ACTIVE_ALT) != 0;
                if (alt) {
                    if (special_key == GLUT_KEY_LEFT) {
                        cycle_selected_category(O, -1);
                    } else if (special_key == GLUT_KEY_RIGHT) {
                        cycle_selected_category(O, 1);
                    } else if (special_key == GLUT_KEY_UP) {
                        adjust_selected_points(O, 1);
                    } else if (special_key == GLUT_KEY_DOWN) {
                        adjust_selected_points(O, -1);
                    }
                    break;
                }

                int max_cursor = has_add_task_row(O) ? O.draft_count
                                                     : O.draft_count - 1;
                if (max_cursor < 0) max_cursor = 0;
                if (special_key == GLUT_KEY_UP) {
                    O.task_cursor--;
                    if (O.task_cursor < 0) O.task_cursor = max_cursor;
                } else if (special_key == GLUT_KEY_DOWN) {
                    O.task_cursor++;
                    if (O.task_cursor > max_cursor) O.task_cursor = 0;
                }
            }
            break;
        default:
            break;
    }
}

void onboard_mouse(GameState& gs, int mx, int my, int win_w, int win_h) {
    if (gs.onboard.step != ONB_TASKS) return;

    OnboardState& O = gs.onboard;
    clamp_task_cursor(O);

    float fx = (float)mx;
    float fy = (float)(win_h - my);
    TaskLayout TL = compute_task_layout(win_w, win_h, O.draft_count);

    if (inside_rect(TL.start_button, fx, fy)) {
        complete_tasks_if_ready(gs);
        return;
    }

    for (int i = 0; i < TL.row_count; i++) {
        if (inside_rect(TL.rows[i], fx, fy)) {
            O.task_cursor = i;
            clamp_task_cursor(O);
            return;
        }
    }
}
