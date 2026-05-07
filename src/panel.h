#pragma once
#include "scene.h"

// Renders the right-side productivity panel: status card, pomodoro timer,
// item checklist. Owns nothing in GameState; reads only.
void panel_draw(const GameState& gs, int win_w, int win_h);

// Returns the ItemId at the given screen-space (mx, my) click, or -1 if none.
// (mx, my) follow GLUT mouse-callback convention: origin at top-left, my grows
// downward. Used by the AI agent to wire mouse-click -> state_complete_item.
int panel_hit_item(const GameState& gs, int mx, int my, int win_w, int win_h);

// Returns one of the strings "start", "pause", "reset", "" for the pomodoro
// button at (mx, my). Empty string means no button hit.
// (mx, my) follow GLUT mouse-callback convention: origin at top-left.
const char* panel_hit_pomo(const GameState& gs, int mx, int my, int win_w, int win_h);
