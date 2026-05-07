#pragma once
#include "scene.h"
#include "player.h"
#include "pet.h"

// AFK behavior: if no input for AFK_THRESHOLD seconds, the AI takes over
// and wanders the player + cat around the home backdrop.
#define AFK_THRESHOLD       5.0f
#define USER_WALK_SPEED   220.0f   // px/sec while held
#define AI_WALK_SPEED      80.0f   // px/sec while wandering

void ai_init(Player& p, Pet& pet);

// Per-frame update. dt is in seconds. Updates Player and Pet positions,
// using home_is_walkable_for_state() to keep them out of walls and rewards.
//
// When gs.user_controlling is true, the player is moved by user input
// (the four direction flags). Otherwise the AI wanders both characters.
void ai_update(GameState& gs, Player& p, Pet& pet,
               float dt,
               bool user_up, bool user_down, bool user_left, bool user_right);
