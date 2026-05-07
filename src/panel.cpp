#include "panel.h"
#include "font.h"
#include "sprite.h"
#include "textures.h"

#include "gl_compat.h"

#include <cmath>
#include <cstdio>
#include <cstring>

// =====================================================================
// Cozy Room RPG — right-side productivity panel.
//
// All rectangles below are computed in OpenGL coordinates: origin at the
// bottom-left of the window, +y up. Hit-test entry points convert from
// GLUT mouse coords (origin top-left, +y down) by `gl_y = win_h - my`.
//
// Layout is recomputed every draw / hit call so we never drift between
// the two — there is no shared cached state.
// =====================================================================

namespace {

// ---------------- Layout (pure functions) -----------------------------

struct Rect { float x, y, w, h; };

struct PanelLayout {
    Rect panel;          // outer panel rect
    Rect status_card;
    Rect pomo_card;
    Rect list_card;

    // Status card sub-rects.
    Rect avatar;

    // Pomodoro sub-rects.
    Rect pomo_btn_primary; // START / PAUSE
    Rect pomo_btn_reset;

    // Item rows. Sized to the runtime maximum; only the first
    // `n_items` are valid for the current GameState.
    Rect items[MAX_ITEMS];
    int  n_items;
};

PanelLayout compute_layout(int win_w, int win_h, int n_items) {
    PanelLayout L{};
    L.n_items = n_items;
    if (L.n_items < 1) L.n_items = 1;
    if (L.n_items > MAX_ITEMS) L.n_items = MAX_ITEMS;
    Layout SL = layout_compute(win_w, win_h);

    L.panel = { (float)SL.panel_x, (float)SL.panel_y,
                (float)SL.panel_w, (float)SL.panel_h };

    const float PAD = 14.0f;
    const float GAP = 12.0f;

    float inner_x = L.panel.x + PAD;
    float inner_w = L.panel.w - 2 * PAD;

    // Top-down packing — easier to reason about in screen-space, but our
    // GL origin is bottom-left, so "top" in our drawing is `y = panel_top`.
    float panel_top = L.panel.y + L.panel.h - PAD;

    // Status card: give the identity/progress block enough room to breathe.
    const float STATUS_H = 138.0f;
    L.status_card = { inner_x, panel_top - STATUS_H, inner_w, STATUS_H };

    // Avatar square inside the status card.
    const float AVATAR = 76.0f;
    L.avatar = { L.status_card.x + 14,
                 L.status_card.y + (L.status_card.h - AVATAR) / 2,
                 AVATAR, AVATAR };

    // Pomodoro card.
    const float POMO_H = 160.0f;
    float pomo_top = L.status_card.y - GAP;
    L.pomo_card = { inner_x, pomo_top - POMO_H, inner_w, POMO_H };

    // Two buttons centered horizontally near the bottom of the pomo card.
    const float BTN_W = 90.0f;
    const float BTN_H = 32.0f;
    const float BTN_GAP = 12.0f;
    float btn_total = BTN_W * 2 + BTN_GAP;
    float btn_x = L.pomo_card.x + (L.pomo_card.w - btn_total) / 2.0f;
    float btn_y = L.pomo_card.y + 12.0f;
    L.pomo_btn_primary = { btn_x,                     btn_y, BTN_W, BTN_H };
    L.pomo_btn_reset   = { btn_x + BTN_W + BTN_GAP,   btn_y, BTN_W, BTN_H };

    // List card fills the rest down to the bottom pad.
    float list_top = L.pomo_card.y - GAP;
    float list_bottom = L.panel.y + PAD;
    L.list_card = { inner_x, list_bottom, inner_w, list_top - list_bottom };

    // Items: header consumes ~28px, then n_items rows packed below.
    const float HEADER_H = 28.0f;
    float row_total_h = L.list_card.h - HEADER_H - 8.0f;
    float row_h = row_total_h / (float)L.n_items;
    if (row_h > 36.0f) row_h = 36.0f;

    float row_y_top = L.list_card.y + L.list_card.h - HEADER_H;
    for (int i = 0; i < L.n_items; i++) {
        L.items[i] = { L.list_card.x + 6,
                       row_y_top - row_h * (i + 1),
                       L.list_card.w - 12,
                       row_h - 2 };
    }

    return L;
}

// ---------------- Drawing primitives ----------------------------------

void draw_rect(float x, float y, float w, float h,
               float r, float g, float b, float a = 1.0f) {
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

void draw_rect_outline(float x, float y, float w, float h,
                       float r, float g, float b, float thick = 2.0f) {
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

// Pseudo-rounded rect: filled body + 1px clipped corners. Lazy & cheap.
void draw_card(float x, float y, float w, float h,
               float r, float g, float b, float a) {
    // Drop the 4 corner pixels by drawing two overlapping quads.
    draw_rect(x + 1, y,     w - 2, h,     r, g, b, a);
    draw_rect(x,     y + 1, w,     h - 2, r, g, b, a);
}

void draw_card_border(float x, float y, float w, float h,
                      float r, float g, float b) {
    glColor3f(r, g, b);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x + 1.0f,     y + 0.5f);
    glVertex2f(x + w - 1.0f, y + 0.5f);
    glVertex2f(x + w - 0.5f, y + 1.0f);
    glVertex2f(x + w - 0.5f, y + h - 1.0f);
    glVertex2f(x + w - 1.0f, y + h - 0.5f);
    glVertex2f(x + 1.0f,     y + h - 0.5f);
    glVertex2f(x + 0.5f,     y + h - 1.0f);
    glVertex2f(x + 0.5f,     y + 1.0f);
    glEnd();
    glLineWidth(1.0f);
}

void draw_circle(float cx, float cy, float r,
                 float cr, float cg, float cb, float ca = 1.0f) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(cr, cg, cb, ca);
    const int N = 28;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= N; i++) {
        float a = (float)i / (float)N * 2.0f * 3.14159265f;
        glVertex2f(cx + std::cos(a) * r, cy + std::sin(a) * r);
    }
    glEnd();
}

void draw_ring(float cx, float cy, float r,
               float cr, float cg, float cb, float ca, float thick = 2.0f) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(cr, cg, cb, ca);
    glLineWidth(thick);
    const int N = 36;
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < N; i++) {
        float a = (float)i / (float)N * 2.0f * 3.14159265f;
        glVertex2f(cx + std::cos(a) * r, cy + std::sin(a) * r);
    }
    glEnd();
    glLineWidth(1.0f);
}

bool inside(const Rect& r, float x, float y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

// ---------------- Component renderers ---------------------------------

void draw_panel_chrome(const PanelLayout& L) {
    // Dark warm background.
    draw_rect(L.panel.x, L.panel.y, L.panel.w, L.panel.h,
              0.10f, 0.08f, 0.12f, 0.95f);
    // Gold/cream border
    draw_card_border(L.panel.x + 1, L.panel.y + 1,
                     L.panel.w - 2, L.panel.h - 2,
                     0.85f, 0.72f, 0.40f);
    // Subtle inner shadow strip across the top.
    draw_rect(L.panel.x, L.panel.y + L.panel.h - 4,
              L.panel.w, 4, 0.0f, 0.0f, 0.0f, 0.25f);
}

void draw_status_card(const PanelLayout& L, const GameState& gs) {
    // Card background.
    draw_card(L.status_card.x, L.status_card.y,
              L.status_card.w, L.status_card.h,
              0.16f, 0.13f, 0.18f, 1.0f);
    draw_card_border(L.status_card.x, L.status_card.y,
                     L.status_card.w, L.status_card.h,
                     0.55f, 0.45f, 0.30f);

    // Avatar block with the selected in-game character.
    float ax = L.avatar.x, ay = L.avatar.y;
    float aw = L.avatar.w, ah = L.avatar.h;
    draw_rect(ax, ay, aw, ah, 0.11f, 0.10f, 0.16f, 1.0f);
    draw_circle(ax + aw / 2, ay + ah / 2, aw * 0.42f,
                0.23f, 0.18f, 0.28f, 1.0f);
    if (g_tex.character.id != 0) {
        const float char_h = ah - 8.0f;
        const float char_w = char_h * 0.5f;
        const float char_x = ax + (aw - char_w) * 0.5f;
        const float char_y = ay + 4.0f;
        sprite_draw(g_tex.character, char_x, char_y, char_w, char_h,
                    3 * 32, 0, 32, 64);
    }
    draw_card_border(ax, ay, aw, ah, 0.85f, 0.72f, 0.40f);

    // Right side: name, progress, HP bar, then stats.
    float tx = ax + aw + 18.0f;
    float right = L.status_card.x + L.status_card.w - 14.0f;
    // Show the saved player name, or "COZY" if onboarding hasn't run yet.
    // Pixel font has no lowercase, but the name capture buffer already
    // holds uppercase letters only.
    const char* status_name = (gs.player_name[0] != '\0') ? gs.player_name : "COZY";
    // Auto-shrink the name's font scale so longer names still fit in the
    // remaining width.
    float name_scale = 3.0f;
    float name_max_w = right - tx;
    while (name_scale > 1.4f && font_width(name_scale, status_name) > name_max_w) {
        name_scale -= 0.2f;
    }
    float name_y = L.status_card.y + L.status_card.h - 16.0f - 7.0f * name_scale;
    font_draw(tx, name_y, name_scale, status_name, 1.0f, 0.92f, 0.65f);

    // % progress below the name, left-aligned.
    float prog = state_overall_progress(gs) * 100.0f;
    char pbuf[16];
    std::snprintf(pbuf, sizeof(pbuf), "%d%%", (int)(prog + 0.5f));
    float progress_y = name_y - 22.0f;
    font_draw(tx, progress_y, 2.0f, pbuf, 0.80f, 0.80f, 0.85f);

    // HP bar.
    float bar_x = tx;
    float bar_y = L.status_card.y + 57.0f;
    float bar_w = right - bar_x;
    float bar_h = 12.0f;

    draw_rect(bar_x, bar_y, bar_w, bar_h, 0.08f, 0.06f, 0.10f, 1.0f);
    float health = gs.health;
    if (health < 0.0f) health = 0.0f;
    if (health > 1.0f) health = 1.0f;
    float hg = 0.30f + 0.55f * health;        // green up
    float hr = 0.95f - 0.55f * health;        // red down
    float hb = 0.20f;
    if (health < 0.25f) { hr = 0.95f; hg = 0.25f; }
    draw_rect(bar_x + 1, bar_y + 1, (bar_w - 2) * health, bar_h - 2,
              hr, hg, hb, 1.0f);
    draw_rect_outline(bar_x, bar_y, bar_w, bar_h, 0.55f, 0.45f, 0.30f, 1.5f);

    char hp_lbl[24];
    std::snprintf(hp_lbl, sizeof(hp_lbl), "HP %d", (int)(health * 100 + 0.5f));
    font_draw(bar_x + 50.0f, progress_y, 2.0f, hp_lbl, 0.85f, 0.85f, 0.90f);

    // Stats row: FOC, STR, IQ, STK
    float sy = L.status_card.y + 14.0f;
    float col_w = (right - tx) / 4.0f;
    char buf[24];
    const float lbl_s = 1.6f;
    const float num_s = 2.4f;
    auto draw_stat = [&](int col, const char* lbl, int val,
                         float r, float g, float b) {
        float cx = tx + col * col_w;
        font_draw(cx, sy + 16, lbl_s, lbl, r, g, b);
        std::snprintf(buf, sizeof(buf), "%d", val);
        font_draw(cx, sy, num_s, buf, 1.0f, 0.95f, 0.85f);
    };
    draw_stat(0, "FOC", gs.stats.focus,    0.55f, 0.80f, 1.0f);
    draw_stat(1, "STR", gs.stats.strength, 1.0f,  0.55f, 0.55f);
    draw_stat(2, "IQ",  gs.stats.iq,       0.75f, 0.65f, 1.0f);
    draw_stat(3, "STK", gs.stats.streak,   1.0f,  0.85f, 0.45f);
}

void format_time(float seconds, char* out, int n) {
    if (seconds < 0) seconds = 0;
    int s = (int)(seconds + 0.5f);
    int m = s / 60;
    int rem = s % 60;
    std::snprintf(out, n, "%02d:%02d", m, rem);
}

void draw_pomo_card(const PanelLayout& L, const GameState& gs) {
    draw_card(L.pomo_card.x, L.pomo_card.y, L.pomo_card.w, L.pomo_card.h,
              0.16f, 0.13f, 0.18f, 1.0f);
    draw_card_border(L.pomo_card.x, L.pomo_card.y,
                     L.pomo_card.w, L.pomo_card.h, 0.55f, 0.45f, 0.30f);

    // Header.
    const char* header = "POMODORO";
    float hw = font_width(2.0f, header);
    font_draw(L.pomo_card.x + (L.pomo_card.w - hw) / 2.0f,
              L.pomo_card.y + L.pomo_card.h - 22,
              2.0f, header, 0.95f, 0.80f, 0.50f);

    // Big time display.
    char tbuf[32];
    float tr = 0.65f, tg = 0.65f, tb = 0.70f;
    if (gs.pomo.phase == POMO_IDLE) {
        std::snprintf(tbuf, sizeof(tbuf), "--:--");
    } else {
        format_time(gs.pomo.remaining_sec, tbuf, sizeof(tbuf));
        if (gs.pomo.phase == POMO_FOCUS) {
            tr = 1.00f; tg = 0.65f; tb = 0.30f;   // warm orange
        } else { // POMO_BREAK
            tr = 0.45f; tg = 0.75f; tb = 1.00f;   // cool blue
        }
    }
    float time_scale = 5.0f;
    float ttw = font_width(time_scale, tbuf);
    float ttx = L.pomo_card.x + (L.pomo_card.w - ttw) / 2.0f;
    float tty = L.pomo_card.y + L.pomo_card.h - 22 - 12 - 7 * time_scale;
    // shadow
    font_draw(ttx + 2, tty - 2, time_scale, tbuf, 0.05f, 0.04f, 0.05f);
    font_draw(ttx,     tty,     time_scale, tbuf, tr, tg, tb);

    // Phase label / sessions count line.
    char info[64];
    if (gs.pomo.phase == POMO_FOCUS) {
        std::snprintf(info, sizeof(info), "FOCUS  SESSIONS %d",
                      gs.pomo.sessions_completed);
    } else if (gs.pomo.phase == POMO_BREAK) {
        std::snprintf(info, sizeof(info), "BREAK  SESSIONS %d",
                      gs.pomo.sessions_completed);
    } else {
        if (gs.pomo.remaining_sec > 0.5f) {
            std::snprintf(info, sizeof(info), "PAUSED  SESSIONS %d",
                          gs.pomo.sessions_completed);
        } else {
            std::snprintf(info, sizeof(info), "READY  SESSIONS %d",
                          gs.pomo.sessions_completed);
        }
    }
    float iw = font_width(1.6f, info);
    font_draw(L.pomo_card.x + (L.pomo_card.w - iw) / 2.0f,
              tty - 18, 1.6f, info, 0.80f, 0.78f, 0.85f);

    // Buttons.
    bool running = (gs.pomo.phase == POMO_FOCUS || gs.pomo.phase == POMO_BREAK);
    const char* primary_label = running ? "PAUSE" : "START";

    auto draw_btn = [&](const Rect& r, const char* label,
                        float br, float bg, float bb) {
        draw_rect(r.x, r.y, r.w, r.h, br * 0.6f, bg * 0.6f, bb * 0.6f, 1.0f);
        draw_rect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, br, bg, bb, 1.0f);
        draw_card_border(r.x, r.y, r.w, r.h, 0.95f, 0.85f, 0.55f);
        float lw = font_width(2.0f, label);
        float lh = 7.0f * 2.0f;
        font_draw(r.x + (r.w - lw) / 2.0f,
                  r.y + (r.h - lh) / 2.0f,
                  2.0f, label, 0.05f, 0.04f, 0.05f);
    };
    if (running) {
        draw_btn(L.pomo_btn_primary, primary_label, 1.00f, 0.72f, 0.40f);
    } else {
        draw_btn(L.pomo_btn_primary, primary_label, 0.55f, 0.85f, 0.55f);
    }
    draw_btn(L.pomo_btn_reset, "RESET", 0.85f, 0.55f, 0.55f);
}

void draw_checkbox(float x, float y, float size, bool checked) {
    draw_rect(x, y, size, size, 0.12f, 0.10f, 0.14f, 1.0f);
    draw_card_border(x, y, size, size, 0.85f, 0.72f, 0.40f);
    if (checked) {
        // diagonal cross from corner to corner, plus a center dot, in green.
        glColor3f(0.55f, 0.95f, 0.55f);
        glLineWidth(2.5f);
        glBegin(GL_LINES);
        glVertex2f(x + 4,        y + size / 2);
        glVertex2f(x + size / 2, y + 3);
        glVertex2f(x + size / 2, y + 3);
        glVertex2f(x + size - 3, y + size - 4);
        glEnd();
        glLineWidth(1.0f);
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

void category_color(ItemCategory c, float& r, float& g, float& b) {
    switch (c) {
        case CAT_FOCUS: r = 0.55f; g = 0.80f; b = 1.00f; break;
        case CAT_BODY:  r = 1.00f; g = 0.55f; b = 0.55f; break;
        case CAT_MIND:  r = 0.75f; g = 0.65f; b = 1.00f; break;
        case CAT_LIFE:  r = 1.00f; g = 0.85f; b = 0.45f; break;
    }
}

void draw_item_list(const PanelLayout& L, const GameState& gs) {
    draw_card(L.list_card.x, L.list_card.y, L.list_card.w, L.list_card.h,
              0.16f, 0.13f, 0.18f, 1.0f);
    draw_card_border(L.list_card.x, L.list_card.y,
                     L.list_card.w, L.list_card.h, 0.55f, 0.45f, 0.30f);

    const char* header = "TODAYS COZY LIST";
    float hw = font_width(2.0f, header);
    font_draw(L.list_card.x + (L.list_card.w - hw) / 2.0f,
              L.list_card.y + L.list_card.h - 22,
              2.0f, header, 0.95f, 0.80f, 0.50f);

    for (int i = 0; i < L.n_items; i++) {
        const Rect& r = L.items[i];
        const ItemDef& def = item_def((ItemId)i);
        bool done = gs.item_done[i];

        // Row striping.
        if (i % 2 == 0) {
            draw_rect(r.x, r.y, r.w, r.h, 0.20f, 0.16f, 0.22f, 0.55f);
        }

        // Sparkle ring on freshly unlocked item.
        if (gs.last_unlocked == i && gs.unlock_anim < 1.0f && done) {
            float t = gs.unlock_anim;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            float ring_r = 8.0f + 28.0f * t;
            float alpha = 1.0f - t;
            float cx = r.x + 18;
            float cy = r.y + r.h / 2;
            draw_ring(cx, cy, ring_r, 1.0f, 0.95f, 0.55f, alpha, 3.0f);
        }

        // Checkbox.
        float box = 18.0f;
        float box_x = r.x + 6;
        float box_y = r.y + (r.h - box) / 2;
        draw_checkbox(box_x, box_y, box, done);

        // Item name.
        float nx = box_x + box + 10;
        float ny = r.y + (r.h - 7 * 1.8f) / 2.0f;
        float nr = done ? 0.55f : 0.95f;
        float ng = done ? 0.95f : 0.92f;
        float nb = done ? 0.55f : 0.85f;
        font_draw(nx, ny, 1.8f, def.name, nr, ng, nb);

        // Stat gain on the right: e.g. "+2 FOCUS"
        float cr_ = 1, cg_ = 1, cb_ = 1;
        category_color(def.category, cr_, cg_, cb_);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "+%d %s", def.stat_gain,
                      category_short(def.category));
        float gw = font_width(1.6f, buf);
        font_draw(r.x + r.w - 8 - gw,
                  r.y + (r.h - 7 * 1.6f) / 2.0f,
                  1.6f, buf, cr_, cg_, cb_);
    }
}

} // namespace

// ===================== Public API =====================================

void panel_draw(const GameState& gs, int win_w, int win_h) {
    PanelLayout L = compute_layout(win_w, win_h, gs.item_count);
    draw_panel_chrome(L);
    draw_status_card(L, gs);
    draw_pomo_card(L, gs);
    draw_item_list(L, gs);
}

int panel_hit_item(const GameState& gs, int mx, int my, int win_w, int win_h) {
    PanelLayout L = compute_layout(win_w, win_h, gs.item_count);
    float fx = (float)mx;
    float fy = (float)(win_h - my); // GLUT -> GL flip
    if (!inside(L.list_card, fx, fy)) return -1;
    for (int i = 0; i < L.n_items; i++) {
        if (inside(L.items[i], fx, fy)) return i;
    }
    return -1;
}

const char* panel_hit_pomo(const GameState& gs, int mx, int my,
                           int win_w, int win_h) {
    PanelLayout L = compute_layout(win_w, win_h, gs.item_count);
    float fx = (float)mx;
    float fy = (float)(win_h - my);
    if (inside(L.pomo_btn_primary, fx, fy)) {
        bool running = (gs.pomo.phase == POMO_FOCUS ||
                        gs.pomo.phase == POMO_BREAK);
        return running ? "pause" : "start";
    }
    if (inside(L.pomo_btn_reset, fx, fy)) {
        return "reset";
    }
    return "";
}
