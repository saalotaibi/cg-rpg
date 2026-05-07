#include "ai.h"
#include "home.h"

#include <cmath>

// ---------------------------------------------------------------------
// Internal AI state — directional random-walk model.
//   IDLE  : stand still for idle_remaining seconds, then pick a direction.
//   WALKING: walk in (dir_x, dir_y) for time_remaining seconds.
//            On wall collision: face opposite, enter short IDLE.
//            On timer expiry:   enter longer IDLE.
// ---------------------------------------------------------------------
enum WanderMode { WANDER_IDLE = 0, WANDER_WALKING };

struct WanderAI {
    WanderMode mode;
    float dir_x, dir_y;          // unit cardinal direction (set when WALKING)
    float time_remaining;        // seconds left in current mode
    // Last facing stored so we can avoid picking the same direction again.
    float last_dir_x, last_dir_y;
};

static WanderAI g_player_ai = {};
static WanderAI g_pet_ai    = {};

// Simple LCG — same seed as original.
static unsigned int g_rng_state = 0xC0FFEEu;
static float frand01() {
    g_rng_state = g_rng_state * 1664525u + 1013904223u;
    return (g_rng_state & 0x00FFFFFFu) / (float)0x01000000;
}
static float frand_range(float a, float b) { return a + frand01() * (b - a); }

// The four cardinal direction vectors.
static const float DIRS[4][2] = {
    { 0.0f, -1.0f },   // N (up in screen coords)
    { 0.0f,  1.0f },   // S
    { -1.0f, 0.0f },   // W
    {  1.0f, 0.0f },   // E
};

// Pick a random cardinal direction, preferring one different from (ex, ey).
// If all three alternatives are the same as (ex, ey) (impossible for 4 dirs)
// we fall back to any random one.
static void pick_direction(WanderAI& ai, float ex, float ey) {
    // Try the three non-excluded directions first in a shuffled order.
    int order[4] = { 0, 1, 2, 3 };
    // Fisher-Yates shuffle (4 elements).
    for (int i = 3; i > 0; i--) {
        int j = (int)(frand01() * (float)(i + 1));
        if (j > i) j = i;
        int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
    }
    for (int i = 0; i < 4; i++) {
        int d = order[i];
        float dx = DIRS[d][0], dy = DIRS[d][1];
        // Skip the direction we just came from.
        if (dx == ex && dy == ey) continue;
        ai.dir_x = dx;
        ai.dir_y = dy;
        return;
    }
    // Fallback: first available (shouldn't reach here).
    int d = (int)(frand01() * 4.0f) % 4;
    ai.dir_x = DIRS[d][0];
    ai.dir_y = DIRS[d][1];
}

// Pick a facing index from a movement vector. 0=down, 1=left, 2=right, 3=up.
// Note: the LimeZu Premade_Character spritesheet's left/right columns are
// laid out the opposite of what you'd expect, so we swap dx here.
static int facing_from_vec(float dx, float dy) {
    if (std::fabs(dx) > std::fabs(dy)) {
        return (dx < 0.0f) ? 2 : 1;
    } else {
        return (dy < 0.0f) ? 3 : 0;
    }
}

// Try to move (x, y) by (dx, dy) using axis-separated walkability checks.
// Returns true if any movement happened.
static bool try_move(const GameState& gs, float& x, float& y, float dx, float dy) {
    bool moved = false;
    if (dx != 0.0f) {
        float nx = x + dx;
        if (home_is_walkable_for_state(gs, nx, y)) { x = nx; moved = true; }
    }
    if (dy != 0.0f) {
        float ny = y + dy;
        if (home_is_walkable_for_state(gs, x, ny)) { y = ny; moved = true; }
    }
    return moved;
}

void ai_init(Player& p, Pet& pet) {
    Layout L = layout_compute(WIN_DEFAULT_W, WIN_DEFAULT_H);
    p.x = L.home_w * 0.5f;
    p.y = L.home_h * 0.5f;
    pet.x = p.x + 110.0f;
    pet.y = p.y + 30.0f;

    g_player_ai = {};
    g_player_ai.mode           = WANDER_IDLE;
    g_player_ai.time_remaining = frand_range(1.5f, 3.5f);
    pick_direction(g_player_ai, 0.0f, 0.0f);   // seed last_dir to something

    g_pet_ai = {};
    // Cat just sits — we don't bother initialising its wander state.
}

// Directional wander tick. Returns true if the entity walked this frame.
static bool wander_tick(const GameState& gs, WanderAI& ai,
                        float& x, float& y, int& facing, float dt) {
    ai.time_remaining -= dt;

    if (ai.mode == WANDER_IDLE) {
        if (ai.time_remaining <= 0.0f) {
            // Done idling: pick a fresh direction and start walking.
            pick_direction(ai, ai.last_dir_x, ai.last_dir_y);
            ai.mode           = WANDER_WALKING;
            ai.time_remaining = frand_range(1.0f, 3.0f);
            facing = facing_from_vec(ai.dir_x, ai.dir_y);
        }
        return false;
    }

    // WALKING mode.
    facing = facing_from_vec(ai.dir_x, ai.dir_y);

    if (ai.time_remaining <= 0.0f) {
        // Natural end of walk: idle a while.
        ai.last_dir_x = ai.dir_x;
        ai.last_dir_y = ai.dir_y;
        ai.mode           = WANDER_IDLE;
        ai.time_remaining = frand_range(1.5f, 3.5f);
        return false;
    }

    float step = AI_WALK_SPEED * dt;
    bool moved = try_move(gs, x, y, ai.dir_x * step, ai.dir_y * step);

    if (!moved) {
        // Blocked by a wall: face the opposite direction, then idle briefly.
        ai.last_dir_x     = ai.dir_x;
        ai.last_dir_y     = ai.dir_y;
        facing            = facing_from_vec(-ai.dir_x, -ai.dir_y);
        ai.mode           = WANDER_IDLE;
        ai.time_remaining = frand_range(0.6f, 1.4f);
        return false;
    }

    return true;
}

void ai_update(GameState& gs, Player& p, Pet& pet,
               float dt,
               bool user_up, bool user_down, bool user_left, bool user_right) {
    bool player_moved = false;

    if (gs.user_controlling) {
        float dx = 0, dy = 0;
        if (user_left)  dx -= 1.0f;
        if (user_right) dx += 1.0f;
        if (user_up)    dy -= 1.0f;
        if (user_down)  dy += 1.0f;
        if (dx != 0.0f && dy != 0.0f) {
            const float inv = 0.70710678f;
            dx *= inv; dy *= inv;
        }
        if (dx != 0.0f || dy != 0.0f) {
            p.facing = facing_from_vec(dx, dy);
            float step = USER_WALK_SPEED * dt;
            player_moved = try_move(gs, p.x, p.y, dx * step, dy * step);
        }
        // Drop AI state so it doesn't resume from a stale direction.
        g_player_ai.mode           = WANDER_IDLE;
        g_player_ai.time_remaining = 0.0f;
    } else {
        player_moved = wander_tick(gs, g_player_ai, p.x, p.y, p.facing, dt);
    }

    p.moving = player_moved;
    if (player_moved) p.walk_phase += dt * 8.0f;
    else              p.walk_phase  = 0.0f;

    // Cat stays put; tick walk_phase for the idle animation.
    pet.walk_phase += dt * 4.0f;
    (void)g_pet_ai;
}
