#include "home.h"

#include "textures.h"
#include "sprite.h"

#include <GL/glew.h>
#include <GL/glut.h>

#include <cmath>

// =====================================================================
// Source-pixel constants for the home assets.
// =====================================================================
static const int SRC_W = 448;
static const int SRC_H = 428;

// Walkable mask: one cell per 32 source-pixels. A cell is walkable if
// the floor pixels there are warm-tinted (wood / cream tile) rather than
// pure neutral gray (walls).
static const int MASK_CELL    = 32;
static const int MASK_COLS    = SRC_W / MASK_CELL;  // 14
static const int MASK_ROWS    = SRC_H / MASK_CELL;  // 13

// Precomputed from assets/textures/home/home_layer1.png with the same
// floor-color rule the runtime used before this was made static.
static const unsigned char WALKABLE_MASK[MASK_ROWS][MASK_COLS] = {
    { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
};

// Cat sheet: 1152x32, with 12 frames at a 96-pixel stride. The previous
// agent's note ("48x32 per frame") was wrong -- empty-column analysis shows
// each cat occupies ~50-56 pixels with ~40-50 pixels of transparent padding
// between sprites. Stride 96 yields one complete cat per frame.
static const int CAT_FRAME_W     = 96;
static const int CAT_FRAME_H     = 32;
static const int CAT_FRAME_COUNT = 12;

// Character sheet: 32x64 cells. Idle row sy=0, walk row sy=64.
// Idle cols (per facing): facing=1 left=0, facing=3 up=1, facing=2 right=2, facing=0 down=3.
// Walk col_starts: facing=1 left=0, facing=3 up=6, facing=2 right=12, facing=0 down=18.
static const int CHAR_W = 32;
static const int CHAR_H = 64;

struct Frame { int sx, sy, sw, sh; };

static const Frame CHAR_IDLE_FRAMES[4] = {
    {  96, 0, CHAR_W, CHAR_H },  // down
    {   0, 0, CHAR_W, CHAR_H },  // left
    {  64, 0, CHAR_W, CHAR_H },  // right
    {  32, 0, CHAR_W, CHAR_H },  // up
};

static const Frame CHAR_WALK_FRAMES[4][6] = {
    {
        { 576, 64, CHAR_W, CHAR_H }, { 608, 64, CHAR_W, CHAR_H },
        { 640, 64, CHAR_W, CHAR_H }, { 672, 64, CHAR_W, CHAR_H },
        { 704, 64, CHAR_W, CHAR_H }, { 736, 64, CHAR_W, CHAR_H },
    },
    {
        {   0, 64, CHAR_W, CHAR_H }, {  32, 64, CHAR_W, CHAR_H },
        {  64, 64, CHAR_W, CHAR_H }, {  96, 64, CHAR_W, CHAR_H },
        { 128, 64, CHAR_W, CHAR_H }, { 160, 64, CHAR_W, CHAR_H },
    },
    {
        { 384, 64, CHAR_W, CHAR_H }, { 416, 64, CHAR_W, CHAR_H },
        { 448, 64, CHAR_W, CHAR_H }, { 480, 64, CHAR_W, CHAR_H },
        { 512, 64, CHAR_W, CHAR_H }, { 544, 64, CHAR_W, CHAR_H },
    },
    {
        { 192, 64, CHAR_W, CHAR_H }, { 224, 64, CHAR_W, CHAR_H },
        { 256, 64, CHAR_W, CHAR_H }, { 288, 64, CHAR_W, CHAR_H },
        { 320, 64, CHAR_W, CHAR_H }, { 352, 64, CHAR_W, CHAR_H },
    },
};

static const Frame CAT_FRAMES[CAT_FRAME_COUNT] = {
    {   0, 0, CAT_FRAME_W, CAT_FRAME_H },
    {  96, 0, CAT_FRAME_W, CAT_FRAME_H },
    { 192, 0, CAT_FRAME_W, CAT_FRAME_H },
    { 288, 0, CAT_FRAME_W, CAT_FRAME_H },
    { 384, 0, CAT_FRAME_W, CAT_FRAME_H },
    { 480, 0, CAT_FRAME_W, CAT_FRAME_H },
    { 576, 0, CAT_FRAME_W, CAT_FRAME_H },
    { 672, 0, CAT_FRAME_W, CAT_FRAME_H },
    { 768, 0, CAT_FRAME_W, CAT_FRAME_H },
    { 864, 0, CAT_FRAME_W, CAT_FRAME_H },
    { 960, 0, CAT_FRAME_W, CAT_FRAME_H },
    { 1056, 0, CAT_FRAME_W, CAT_FRAME_H },
};

struct BackdropFit {
    float scale;       // backdrop_pixels -> home-rect pixels
    float off_x;       // offset within home rect (left padding)
    float off_y;       // offset within home rect (top padding, in home-rect space)
    float draw_w;      // SRC_W * scale
    float draw_h;      // SRC_H * scale
};

// Fixed for WIN_DEFAULT_W x WIN_DEFAULT_H and the source room art.
static const BackdropFit HOME_FIT = {
    1.8130841f,
    41.86916f,
    0.0f,
    812.2617f,
    776.0f,
};

static BackdropFit fit_backdrop(int home_w, int home_h) {
    (void)home_w;
    (void)home_h;
    return HOME_FIT;
}

static bool home_to_source(float p_x, float p_y, float& src_x, float& src_y) {
    src_x = (p_x - HOME_FIT.off_x) / HOME_FIT.scale;
    src_y = (p_y - HOME_FIT.off_y) / HOME_FIT.scale;
    if (src_x < 0 || src_x >= SRC_W) return false;
    if (src_y < 0 || src_y >= SRC_H) return false;
    return true;
}

bool home_is_walkable(float p_x, float p_y) {
    float src_x, src_y;
    if (!home_to_source(p_x, p_y, src_x, src_y)) return false;
    int mx = (int)(src_x / MASK_CELL);
    int my = (int)(src_y / MASK_CELL);
    if (mx < 0 || mx >= MASK_COLS) return false;
    if (my < 0 || my >= MASK_ROWS) return false;
    return WALKABLE_MASK[my][mx] != 0;
}

// =====================================================================
// Backdrop drawing. Slight phase-based tint applied to the layers.
// =====================================================================
static void compute_phase_tint(PomoPhase phase, float& r, float& g, float& b) {
    // Subtle, ~5% shift. Default = neutral.
    r = 1.0f; g = 1.0f; b = 1.0f;
    switch (phase) {
        case POMO_FOCUS:  r = 1.03f; g = 1.00f; b = 0.95f; break;  // warm
        case POMO_BREAK:  r = 0.96f; g = 0.99f; b = 1.04f; break;  // cool
        case POMO_IDLE:   default: break;
    }
}

static void draw_backdrop(const GameState& gs, int home_x, int home_y,
                          int home_w, int home_h, BackdropFit fit) {
    float dx = home_x + fit.off_x;
    // GL bottom-left origin: the backdrop's top edge sits at home_y + home_h - off_y.
    float dy = home_y + (home_h - fit.off_y - fit.draw_h);

    float tr, tg, tb;
    compute_phase_tint(gs.pomo.phase, tr, tg, tb);

    sprite_draw_tinted(g_tex.home_layer1,
                       dx, dy, fit.draw_w, fit.draw_h,
                       0, 0, SRC_W, SRC_H,
                       tr, tg, tb, 1.0f);
}

// =====================================================================
// Milestone reward decorations. Coordinates are in the 448x428 source
// backdrop space, measured from the top-left. They reveal by overall
// completion percentage, not by individual task slot.
// =====================================================================
struct RewardDecor {
    RewardTexture tex;
    float threshold;
    float src_x;
    float src_y;
    float scale;
    bool floor_anchor;
    bool blocks_movement;
    float hit_left, hit_top, hit_right, hit_bottom;
};

static const RewardDecor REWARD_DECOR[] = {
    // Draw coordinates come from items.json. Hit rectangles were precomputed
    // from those draw values and the reward sprite dimensions.
    { REWARD_BED,        0.10f, 289.69f, 151.03f, 0.91f, true,  true,  283.69f,  45.11f, 397.61f, 155.03f },
    { REWARD_WALL_ART,   0.30f, 342.67f,   8.76f, 0.70f, false, false,   0.00f,   0.00f,   0.00f,   0.00f },
    { REWARD_FLOOR_LAMP, 0.50f, 353.22f, 271.29f, 0.70f, true,  true,  347.22f, 177.69f, 392.82f, 275.29f },
    { REWARD_RUG,        0.70f, 190.35f, 194.25f, 0.72f, true,  false,   0.00f,   0.00f,   0.00f,   0.00f },
    { REWARD_BOOKCASE,   0.80f, 287.62f, 366.18f, 0.88f, true,  true,  281.62f, 235.46f, 378.10f, 370.18f },
    { REWARD_STUDY_DESK, 0.85f,  92.25f, 424.80f, 0.91f, true,  true,   86.25f, 333.44f, 171.05f, 428.80f },
    { REWARD_ROUND_RUG,  0.88f, 214.83f, 348.79f, 0.74f, false, false,   0.00f,   0.00f,   0.00f,   0.00f },
    { REWARD_CHAIR,      0.90f, 155.28f, 109.92f, 0.77f, true,  true,  149.28f,  32.00f, 198.24f, 113.92f },
    { REWARD_POT,        1.00f, 357.98f, 408.00f, 0.54f, true,  true,  351.98f, 369.44f, 389.90f, 412.00f },
};

struct PlantStage {
    float threshold;
    RewardTexture tex;
    float hit_left, hit_top, hit_right, hit_bottom;
};

static const PlantStage PLANT_STAGES[] = {
    { 0.20f, REWARD_PLANT_STAGE_1, 89.52f, 80.46f, 125.52f, 128.46f },
    { 0.40f, REWARD_PLANT_STAGE_2, 89.52f, 72.46f, 125.52f, 128.46f },
    { 0.60f, REWARD_PLANT_STAGE_3, 89.52f, 64.46f, 125.52f, 128.46f },
    { 0.80f, REWARD_PLANT_STAGE_4, 89.52f, 56.46f, 149.52f, 128.46f },
};

static const PlantStage* plant_stage_for_progress(float progress) {
    for (int i = 3; i >= 0; i--) {
        if (progress + 0.0001f >= PLANT_STAGES[i].threshold) {
            return &PLANT_STAGES[i];
        }
    }
    return nullptr;
}

static bool point_in_rect(float x, float y,
                          float left, float top, float right, float bottom) {
    return x >= left && x <= right && y >= top && y <= bottom;
}

static bool reward_obstacle_at(const GameState& gs, float src_x, float src_y) {
    float progress = state_overall_progress(gs);

    const PlantStage* plant = plant_stage_for_progress(progress);
    if (plant && point_in_rect(src_x, src_y, plant->hit_left, plant->hit_top,
                               plant->hit_right, plant->hit_bottom)) {
        return true;
    }

    for (const RewardDecor& d : REWARD_DECOR) {
        if (progress + 0.0001f < d.threshold) continue;
        if (!d.blocks_movement) continue;
        if (point_in_rect(src_x, src_y, d.hit_left, d.hit_top,
                          d.hit_right, d.hit_bottom)) {
            return true;
        }
    }
    return false;
}

bool home_is_walkable_for_state(const GameState& gs, float p_x, float p_y) {
    if (!home_is_walkable(p_x, p_y)) return false;

    float src_x, src_y;
    if (!home_to_source(p_x, p_y, src_x, src_y)) return false;
    return !reward_obstacle_at(gs, src_x, src_y);
}

static void draw_reward_texture(RewardTexture id, float src_x, float src_y,
                                float source_scale, bool floor_anchor,
                                int home_x, int home_y, int home_h,
                                BackdropFit fit) {
    if (id < 0 || id >= REWARD_TEXTURE_COUNT) return;

    const Texture& tex = g_tex.rewards[id];
    if (tex.id == 0 || tex.w <= 0 || tex.h <= 0) return;

    float draw_w = tex.w * source_scale;
    float draw_h = tex.h * source_scale;
    float top_y = floor_anchor ? (src_y - draw_h) : src_y;

    float dx = home_x + fit.off_x + src_x * fit.scale;
    float dy = home_y + (home_h - fit.off_y - (top_y + draw_h) * fit.scale);
    sprite_draw(tex, dx, dy, draw_w * fit.scale, draw_h * fit.scale,
                0, 0, tex.w, tex.h);
}

static void draw_reward_decor(const GameState& gs, int home_x, int home_y,
                              int home_h, BackdropFit fit) {
    float progress = state_overall_progress(gs);

    const PlantStage* plant = plant_stage_for_progress(progress);
    if (plant) {
        draw_reward_texture(plant->tex, 95.52f, 124.46f, 0.50f, true,
                            home_x, home_y, home_h, fit);
    }

    for (const RewardDecor& d : REWARD_DECOR) {
        if (progress + 0.0001f < d.threshold) continue;
        draw_reward_texture(d.tex, d.src_x, d.src_y, d.scale, d.floor_anchor,
                            home_x, home_y, home_h, fit);
    }
}

// =====================================================================
// Character + pet drawing.
// Player/Pet position is in home-rect-space pixels (top-down y).
// Anchor: bottom-center of sprite at (p.x, p.y).
// =====================================================================
static void draw_shadow(float cx, float cy, float radius_x, float radius_y) {
    // Soft alpha-blended ellipse. cy is in screen space (GL bottom-left).
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0, 0, 0, 0.35f);
    glVertex2f(cx, cy);
    glColor4f(0, 0, 0, 0.0f);
    const int N = 18;
    for (int i = 0; i <= N; i++) {
        float a = (float)i / N * 6.2831853f;
        glVertex2f(cx + std::cos(a) * radius_x,
                   cy + std::sin(a) * radius_y);
    }
    glEnd();
    glDisable(GL_BLEND);
    glColor4f(1, 1, 1, 1);
}

static void player_screen_pos(const Player& p, int home_x, int home_y, int home_h,
                              float& out_x, float& out_y) {
    out_x = home_x + p.x;
    out_y = home_y + (home_h - p.y);
}

static void draw_character(const Player& p, int home_x, int home_y, int home_h) {
    float feet_x, feet_y;
    player_screen_pos(p, home_x, home_y, home_h, feet_x, feet_y);

    // Render at 2x source size: 64 wide, 128 tall.
    const float DRAW_W = 64.0f;
    const float DRAW_H = 128.0f;

    draw_shadow(feet_x, feet_y - 2.0f, DRAW_W * 0.32f, DRAW_W * 0.10f);

    Frame frame;
    if (p.moving) {
        int frame_idx = (int)(p.walk_phase * 6.0f) % 6;
        if (frame_idx < 0) frame_idx += 6;
        int facing = p.facing;
        if (facing < 0 || facing > 3) facing = 0;
        frame = CHAR_WALK_FRAMES[facing][frame_idx];
    } else {
        int facing = p.facing;
        if (facing < 0 || facing > 3) facing = 0;
        frame = CHAR_IDLE_FRAMES[facing];
    }

    // Bottom-center anchor: dx = feet_x - DRAW_W/2, dy = feet_y (sprite extends up).
    sprite_draw(g_tex.character,
                feet_x - DRAW_W * 0.5f, feet_y,
                DRAW_W, DRAW_H,
                frame.sx, frame.sy, frame.sw, frame.sh);
}

static void draw_cat(const Pet& pet, int home_x, int home_y, int home_h) {
    if (!pet.visible) return;

    float feet_x = home_x + pet.x;
    float feet_y = home_y + (home_h - pet.y);

    // The cat sheet frame has wide transparent padding. Full 2x made the pet
    // read larger than the player, so keep it closer to floor-object scale.
    const float CAT_SCALE = 1.25f;
    const float DRAW_W = (float)CAT_FRAME_W * CAT_SCALE;
    const float DRAW_H = (float)CAT_FRAME_H * CAT_SCALE;

    draw_shadow(feet_x, feet_y - 2.0f, DRAW_W * 0.20f, DRAW_W * 0.05f);

    int frame_idx = (int)(pet.walk_phase * CAT_FRAME_COUNT) % CAT_FRAME_COUNT;
    if (frame_idx < 0) frame_idx += CAT_FRAME_COUNT;
    const Frame& frame = CAT_FRAMES[frame_idx];

    // The cat sprite is centered horizontally within its 96px frame, so
    // bottom-center anchor works directly.
    float dx = feet_x - DRAW_W * 0.5f;
    float dy = feet_y;
    sprite_draw(g_tex.cat,
                dx, dy, DRAW_W, DRAW_H,
                frame.sx, frame.sy, frame.sw, frame.sh);
}

static void draw_living_sprites(const Player& p, const Pet& pet,
                                int home_x, int home_y, int home_h) {
    // Larger top-down y is lower/closer in the room and should be drawn last.
    if (pet.visible && pet.y < p.y) {
        draw_cat(pet, home_x, home_y, home_h);
        draw_character(p, home_x, home_y, home_h);
    } else {
        draw_character(p, home_x, home_y, home_h);
        draw_cat(pet, home_x, home_y, home_h);
    }
}

// =====================================================================
// Public draw entry point.
// =====================================================================
void home_draw(const GameState& gs, const Player& p, const Pet& pet,
               int win_w, int win_h) {
    Layout L = layout_compute(win_w, win_h);
    BackdropFit fit = fit_backdrop(L.home_w, L.home_h);

    draw_backdrop(gs, L.home_x, L.home_y, L.home_w, L.home_h, fit);
    draw_reward_decor(gs, L.home_x, L.home_y, L.home_h, fit);
    draw_living_sprites(p, pet, L.home_x, L.home_y, L.home_h);
}
