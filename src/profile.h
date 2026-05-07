#pragma once

#include "scene.h"

// =====================================================================
// Cozy Room RPG — player profile persistence.
//
// The profile lives at:  ~/.cozy-room/profile.json
//
// Format is a tiny line-based JSON that the hand-rolled parser in
// profile.cpp can chew on; nothing fancy, just key/value pairs and a
// `tasks` array. Users may hand-edit the file safely.
//
// Schema (all keys optional; a missing key keeps the GameState default):
//   {
//     "name": "ALICE",
//     "character_index": 2,
//     "tasks": [
//       { "name": "DRINK WATER", "category": "LIFE",  "stat_gain": 1 },
//       ...
//     ]
//   }
// =====================================================================

// Loads the profile. Returns true if a profile file existed and was
// applied to `gs` (player_name, character_index, item table, item_count
// + cleared item_done). Returns false if no profile file is present
// (caller should run onboarding).
bool profile_load(GameState& gs);

// Writes the current profile to disk. Creates the directory if needed.
void profile_save(const GameState& gs);

// Removes the profile file (used by the title-screen "R" reset key).
void profile_delete();

// Path of the profile file in the user's home directory. Returned as a
// pointer into a static buffer; safe to use until the next call.
const char* profile_path();
