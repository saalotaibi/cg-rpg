#pragma once

// =====================================================================
// Cozy Room RPG — single-screen redesign.
//
// The whole game is one screen:
//   - Left ~70%: a top-down view of a cozy home (LimeZu Generic_Home_1).
//     The character lives in this home and wanders around when the user
//     is AFK. The character can also be steered with WASD.
//   - Right ~30%: a persistent productivity panel with a status card,
//     Pomodoro timer, health bar, and cute item checklist.
//
// Completing items / pomodoros raises stats and animates a sparkle ring
// in the panel.
// =====================================================================

enum Scene {
    SCENE_TITLE = 0,
    SCENE_ONBOARD,   // first-run wizard: name -> character -> tasks
    SCENE_GAME,
    SCENE_COUNT
};

// ---------------------------------------------------------------------
// Items the player can check off. Each maps to a stat gain.
//
// The default 8-item set is kept as named enum constants, but the runtime
// item table is mutable: see `g_items` in scene.cpp / `item_def()` /
// `item_count()` / `state_set_items()`. ITEM_COUNT is preserved as the
// *default* count (used to size some legacy arrays).
// ---------------------------------------------------------------------
enum ItemId {
    ITEM_DRINK_WATER = 0,
    ITEM_STRETCH,
    ITEM_DEEP_WORK,
    ITEM_READ_PAGES,
    ITEM_EXERCISE,
    ITEM_MEDITATE,
    ITEM_TIDY_DESK,
    ITEM_GO_OUTSIDE,
    ITEM_COUNT
};

// Maximum number of items the runtime list can hold. GameState arrays
// are sized to this; the active count is `gs.item_count`.
#define MAX_ITEMS 16

enum ItemCategory {
    CAT_FOCUS = 0,   // +Focus
    CAT_BODY,        // +Strength
    CAT_MIND,        // +IQ
    CAT_LIFE         // +Health / +Streak
};

struct ItemDef {
    const char* name;       // display label (owned by scene.cpp's storage)
    ItemCategory category;
    int stat_gain;
};

// Lookup by index. id is treated as an int — values may exceed
// ITEM_COUNT once the user customizes their task list.
const ItemDef& item_def(ItemId id);

// Active item count after profile load / onboarding (defaults to 8).
int item_count();

// Replace the runtime item table with a custom set, copying names into
// scene-owned storage. Use n=0 to reset to the default 8-item table.
void state_set_default_items();
void state_set_items(const ItemDef* items, int n);

// ---------------------------------------------------------------------
// Stats — RPG-flavored numbers in the status card.
// ---------------------------------------------------------------------
struct Stats {
    int focus;
    int strength;
    int iq;
    int streak;
};

// ---------------------------------------------------------------------
// Pomodoro timer.
// ---------------------------------------------------------------------
enum PomoPhase {
    POMO_IDLE = 0,
    POMO_FOCUS,
    POMO_BREAK
};

struct PomodoroState {
    PomoPhase phase;
    float remaining_sec;        // counts down to 0
    int sessions_completed;
};

#define POMO_FOCUS_SECONDS  (25.0f * 60.0f)
#define POMO_BREAK_SECONDS  (5.0f  * 60.0f)

// ---------------------------------------------------------------------
// Onboarding wizard sub-state. Kept inside GameState so onboard.cpp
// owns no globals beyond the wizard's input buffers.
// ---------------------------------------------------------------------
enum OnboardStep {
    ONB_NAME = 0,
    ONB_CHARACTER,
    ONB_TASKS,
    ONB_DONE
};

#define ONB_MAX_NAME       16   // character cap (excluding null)
#define ONB_MAX_TASK_NAME  16
#define ONB_NUM_CHARACTERS 5

struct OnboardState {
    OnboardStep step;
    int  char_index;             // 0..ONB_NUM_CHARACTERS-1

    // Tasks-step working state.
    ItemDef draft_items[MAX_ITEMS];
    char    draft_names[MAX_ITEMS][ONB_MAX_TASK_NAME + 1];
    int     draft_count;
    int     task_cursor;         // 0..draft_count; draft_count = "+ new task"
};

// ---------------------------------------------------------------------
// Single shared GameState. All agents read/write fields here.
// ---------------------------------------------------------------------
struct GameState {
    Scene current_scene;

    // Player profile.
    char  player_name[ONB_MAX_NAME + 1];   // null-terminated; empty until set
    int   character_index;                 // 0..ONB_NUM_CHARACTERS-1

    // Item list.
    int   item_count;                      // active number of items (<= MAX_ITEMS)
    bool  item_done[MAX_ITEMS];
    int   last_unlocked;                   // index in item table, -1 = none
    float unlock_anim;                     // 0..1, drives sparkle ring in panel

    Stats         stats;
    PomodoroState pomo;
    float         health;       // 0..1

    float afk_seconds;          // seconds since last user input
    bool  user_controlling;     // true while WASD held

    float time_total;
    bool  show_help;

    // Onboarding sub-state. Only valid while current_scene == SCENE_ONBOARD.
    OnboardState onboard;
};

void  state_init(GameState& gs);
void  state_complete_item(GameState& gs, ItemId id);
float state_overall_progress(const GameState& gs); // 0..1

void state_pomo_start(GameState& gs);
void state_pomo_pause(GameState& gs);
void state_pomo_reset(GameState& gs);
void state_pomo_tick(GameState& gs, float dt);   // counts down + auto-advances

// ---------------------------------------------------------------------
// Screen partition — all renderers read their bounds from here.
// ---------------------------------------------------------------------
struct Layout {
    int win_w, win_h;
    int home_x, home_y, home_w, home_h;     // backdrop region
    int panel_x, panel_y, panel_w, panel_h; // side panel region
};

Layout layout_compute(int win_w, int win_h);

// Default window size. Render harness should use this.
#define WIN_DEFAULT_W 1280
#define WIN_DEFAULT_H 800
