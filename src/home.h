#pragma once
#include "scene.h"
#include "player.h"
#include "pet.h"

// Draws the home backdrop, reward decorations, the player, and the pet.
// Caller is expected to have set up the 2D ortho projection already.
void home_draw(const GameState& gs, const Player& p, const Pet& pet,
               int win_w, int win_h);

// Returns true if the pixel coord (x, y) -- in *home backdrop coordinates*
// (same convention as Player::x, Player::y; origin at backdrop top-left,
// y growing downward) -- is walkable. Cheap lookup into a precomputed mask.
bool home_is_walkable(float x, float y);

// Same coordinate convention as home_is_walkable(), but also treats unlocked
// reward furniture as solid so manual and AI movement cannot pass through it.
bool home_is_walkable_for_state(const GameState& gs, float x, float y);
