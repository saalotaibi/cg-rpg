// main.cpp — Cozy Room RPG game loop.
//
// Single-screen layout: home (left ~70%) + side panel (right ~30%).
// AFK wander AI takes over when the user hasn't pressed WASD for 5s.
// Pomodoro + health tick each frame. Completing an item triggers a
// sparkle ring animation in the panel and updates stats.
//
// Launch flow: TITLE -> ONBOARD wizard (3 steps) -> GAME, with the
// player profile saved to ~/.cozy-room/profile.json. A saved profile may
// prefill state, but the wizard is always shown before play.

#include <GL/glew.h>
#include <GL/glut.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "scene.h"
#include "home.h"
#include "panel.h"
#include "ai.h"
#include "ui.h"
#include "textures.h"
#include "player.h"
#include "pet.h"
#include "onboard.h"
#include "profile.h"

static GameState gs;
static Player    player;
static Pet       cat;
static int       win_w = WIN_DEFAULT_W;
static int       win_h = WIN_DEFAULT_H;
static int       prev_time_ms = 0;

// True iff a profile was loaded at startup. Used for reset/save state only;
// starting the game always goes through onboarding.
static bool      g_have_profile = false;

// Held-key flags. WASD + arrow keys map to the same direction bools.
static bool key_w = false, key_a = false, key_s = false, key_d = false;

static void trigger_item(ItemId id) {
    int idx = (int)id;
    if (idx < 0 || idx >= gs.item_count) return;
    if (gs.item_done[idx]) return;
    state_complete_item(gs, id);
    // unlock_anim is reset by state_complete_item; the sparkle ring in the
    // panel will animate. No scene switch needed.
}

static void enter_game() {
    gs.current_scene = SCENE_GAME;
    prev_time_ms = glutGet(GLUT_ELAPSED_TIME);
}

static void start_onboarding() {
    onboard_init(gs);
    gs.current_scene = SCENE_ONBOARD;
    prev_time_ms = glutGet(GLUT_ELAPSED_TIME);
}

static void display() {
    glClearColor(0.06f, 0.06f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (gs.current_scene == SCENE_TITLE) {
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluOrtho2D(0, win_w, 0, win_h);
        glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
        ui_draw_title(win_w, win_h, gs.time_total);
        glutSwapBuffers();
        return;
    }

    if (gs.current_scene == SCENE_ONBOARD) {
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluOrtho2D(0, win_w, 0, win_h);
        glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
        onboard_draw(gs, win_w, win_h);
        glutSwapBuffers();
        return;
    }

    // 2D pass: home + panel.
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0, win_w, 0, win_h);
    glMatrixMode(GL_MODELVIEW);  glLoadIdentity();

    home_draw(gs, player, cat, win_w, win_h);
    panel_draw(gs, win_w, win_h);

    glutSwapBuffers();
}

static void reshape(int w, int h) {
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    win_w = w;
    win_h = h;
    glViewport(0, 0, w, h);
}

static void timer(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - prev_time_ms) / 1000.0f;
    if (dt > 0.1f) dt = 0.1f;          // clamp huge frames (e.g. resume from pause)
    if (dt < 0.0f) dt = 0.0f;
    prev_time_ms = now;
    gs.time_total += dt;

    if (gs.current_scene == SCENE_ONBOARD) {
        // Wizard runs as a static screen — no AI / pomodoro / etc.
        // We only need gs.time_total for the cursor-blink effect. If
        // the user just finished step 3, save and switch to the game.
        if (onboard_complete(gs)) {
            profile_save(gs);
            g_have_profile = true;
            enter_game();
        }
    }

    if (gs.current_scene == SCENE_GAME) {
        bool any_input = key_w || key_a || key_s || key_d;
        if (any_input) {
            gs.afk_seconds = 0.0f;
            gs.user_controlling = true;
        } else {
            gs.afk_seconds += dt;
            if (gs.afk_seconds > AFK_THRESHOLD) gs.user_controlling = false;
        }
        ai_update(gs, player, cat, dt, key_w, key_s, key_a, key_d);
        state_pomo_tick(gs, dt);

        // Health drains slowly when no pomo session is running.
        if (gs.pomo.phase == POMO_IDLE) {
            gs.health -= dt / 600.0f;   // -1.0 over 10 minutes idle
            if (gs.health < 0.0f) gs.health = 0.0f;
        }

        // unlock_anim animates 0 -> 1 on every newly-completed item (drives
        // the sparkle ring in the panel).
        if (gs.unlock_anim < 1.0f) {
            gs.unlock_anim += dt * 0.6f;
            if (gs.unlock_anim >= 1.0f) gs.unlock_anim = 1.0f;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

static void key_down(unsigned char k, int, int) {
    // Title screen: SPACE / ENTER starts onboarding. R resets the saved profile.
    if (gs.current_scene == SCENE_TITLE) {
        if (k == 27) std::exit(0);
        if (k == 'r' || k == 'R') {
            profile_delete();
            g_have_profile = false;
            // Wipe profile-derived state so it's obvious in the panel.
            gs.player_name[0] = '\0';
            gs.character_index = 0;
            state_set_default_items();
            gs.item_count = item_count();
            std::memset(gs.item_done, 0, sizeof(gs.item_done));
            return;
        }
        if (k == ' ' || k == '\r' || k == '\n') {
            start_onboarding();
            return;
        }
        return;
    }

    if (gs.current_scene == SCENE_ONBOARD) {
        if (k == 27) std::exit(0);
        onboard_key(gs, k);
        return;
    }

    switch (k) {
        case 27: std::exit(0);
        case 'w': case 'W': key_w = true; break;
        case 'a': case 'A': key_a = true; break;
        case 's': case 'S': key_s = true; break;
        case 'd': case 'D': key_d = true; break;
        case 'p': case 'P':
            if (gs.pomo.phase == POMO_IDLE) state_pomo_start(gs);
            else                            state_pomo_pause(gs);
            break;
        case 'r': case 'R':
            state_pomo_reset(gs);
            break;
        case 'h': case 'H':
            gs.show_help = !gs.show_help;
            break;
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8':
        case '9': {
            int idx = k - '1';
            if (idx >= 0 && idx < gs.item_count) {
                trigger_item((ItemId)idx);
            }
            break;
        }
        case '0': {
            // Slot 10. Slots 11/12 don't have a digit shortcut — mouse
            // click only.
            int idx = 9;
            if (idx < gs.item_count) trigger_item((ItemId)idx);
            break;
        }
        default: break;
    }
}

static void key_up(unsigned char k, int, int) {
    switch (k) {
        case 'w': case 'W': key_w = false; break;
        case 'a': case 'A': key_a = false; break;
        case 's': case 'S': key_s = false; break;
        case 'd': case 'D': key_d = false; break;
        default: break;
    }
}

static void special_down(int k, int, int) {
    if (gs.current_scene == SCENE_TITLE) return;
    if (gs.current_scene == SCENE_ONBOARD) {
        onboard_special(gs, k);
        return;
    }
    switch (k) {
        case GLUT_KEY_UP:    key_w = true; break;
        case GLUT_KEY_DOWN:  key_s = true; break;
        case GLUT_KEY_LEFT:  key_a = true; break;
        case GLUT_KEY_RIGHT: key_d = true; break;
        default: break;
    }
}

static void special_up(int k, int, int) {
    switch (k) {
        case GLUT_KEY_UP:    key_w = false; break;
        case GLUT_KEY_DOWN:  key_s = false; break;
        case GLUT_KEY_LEFT:  key_a = false; break;
        case GLUT_KEY_RIGHT: key_d = false; break;
        default: break;
    }
}

static void mouse(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return;
    if (button != GLUT_LEFT_BUTTON) return;

    if (gs.current_scene == SCENE_TITLE) {
        start_onboarding();
        return;
    }
    if (gs.current_scene == SCENE_ONBOARD) {
        onboard_mouse(gs, x, y, win_w, win_h);
        return;
    }
    if (gs.current_scene != SCENE_GAME) return;

    int it = panel_hit_item(gs, x, y, win_w, win_h);
    if (it >= 0) {
        trigger_item((ItemId)it);
        return;
    }
    const char* btn = panel_hit_pomo(gs, x, y, win_w, win_h);
    if (btn && btn[0]) {
        if      (std::strcmp(btn, "start") == 0) state_pomo_start(gs);
        else if (std::strcmp(btn, "pause") == 0) state_pomo_pause(gs);
        else if (std::strcmp(btn, "reset") == 0) state_pomo_reset(gs);
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(win_w, win_h);
    glutCreateWindow("Cozy Room RPG");

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(err));
        return 1;
    }

    if (!textures_load_all()) {
        std::fprintf(stderr, "WARNING: some textures failed to load.\n");
    }

    state_init(gs);

    // Try to load a saved profile so title/onboarding can reuse saved state.
    // SPACE still starts onboarding before entering the game.
    g_have_profile = profile_load(gs);
    if (g_have_profile) {
        // Apply the saved character to the live texture.
        onboard_apply_character(gs.character_index);
    }

    player_init(player);
    pet_init(cat);
    ai_init(player, cat);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key_down);
    glutKeyboardUpFunc(key_up);
    glutSpecialFunc(special_down);
    glutSpecialUpFunc(special_up);
    glutMouseFunc(mouse);
    glutIgnoreKeyRepeat(1);

    prev_time_ms = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
