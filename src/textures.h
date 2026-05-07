#pragma once

#include "texture.h"

enum RewardTexture {
    REWARD_PLANT_STAGE_1 = 0,
    REWARD_PLANT_STAGE_2,
    REWARD_PLANT_STAGE_3,
    REWARD_PLANT_STAGE_4,
    REWARD_RUG,
    REWARD_WALL_ART,
    REWARD_STUDY_DESK,
    REWARD_POT,
    REWARD_FLOOR_LAMP,
    REWARD_BOOKCASE,
    REWARD_CHAIR,
    REWARD_BED,
    REWARD_ROUND_RUG,
    REWARD_TEXTURE_COUNT
};

// All textures used by the new single-screen design.
struct Textures {
    // The cozy home backdrop (LimeZu Generic_Home_1).
    // Layer 1 = empty home (floors + walls + doors).
    // Layer 2 is loaded for asset completeness but no longer drawn over the room.
    Texture home_layer1;
    Texture home_layer2;

    // Character + pet
    Texture character;
    Texture cat;

    // Milestone reward decorations, unlocked by overall task percentage.
    Texture rewards[REWARD_TEXTURE_COUNT];

    bool loaded;
};

extern Textures g_tex;

bool textures_load_all();
void textures_free_all();
