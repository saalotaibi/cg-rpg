#pragma once

#include "scene.h"

// =====================================================================
// First-run onboarding wizard.
//
// Three steps:
//   1. ONB_NAME       — type a player name (uppercase, max 16 chars)
//   2. ONB_CHARACTER  — left/right scroll a 5-sheet carousel, Enter selects
//   3. ONB_TASKS      — edit daily tasks, then click START
//
// onboard_complete(gs) flips to true once step 3 is finalized; main.cpp
// then writes the profile and transitions to SCENE_GAME.
//
// All wizard state is on GameState (gs.onboard) — no globals here.
// =====================================================================

void onboard_init(GameState& gs);

// Routes a regular printable / control key (ASCII).
void onboard_key(GameState& gs, unsigned char k);

// Routes a special key (GLUT_KEY_*). Used for arrow navigation in the
// character carousel and the task-list cursor.
void onboard_special(GameState& gs, int special_key);

// Routes a left-click in GLUT mouse coordinates. Used by the task editor's
// row selection and START button.
void onboard_mouse(GameState& gs, int mx, int my, int win_w, int win_h);

// Renders the wizard at full-screen. Caller should clear the framebuffer
// + set up an ortho 2D projection first (same as ui_draw_title).
void onboard_draw(const GameState& gs, int win_w, int win_h);

// Returns true once the user finished step 3 (and therefore main.cpp
// should save the profile and switch to SCENE_GAME).
bool onboard_complete(const GameState& gs);

// Number of character spritesheets that ship with the game. Same as
// ONB_NUM_CHARACTERS but exposed as a function so callers don't have
// to know the macro.
int onboard_character_count();

// Path of one of the carousel character sheets, as installed under
// assets/textures/characters/. Index in [0, ONB_NUM_CHARACTERS).
const char* onboard_character_path(int index);

// Reload g_tex.character from assets/textures/characters/<index>.png so the
// rest of the game sees the selected sprite immediately.
void onboard_apply_character(int index);
