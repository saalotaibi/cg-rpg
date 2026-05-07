#include "textures.h"
#include <cstdio>

Textures g_tex = {};

static bool load_one(Texture* out, const char* path) {
    if (!texture_load(out, path)) {
        fprintf(stderr, "textures_load_all: missing %s\n", path);
        return false;
    }
    return true;
}

bool textures_load_all() {
    bool ok = true;
    ok &= load_one(&g_tex.home_layer1, "assets/textures/home/home_layer1.png");
    ok &= load_one(&g_tex.home_layer2, "assets/textures/home/home_layer2.png");
    ok &= load_one(&g_tex.character,   "assets/textures/character.png");
    ok &= load_one(&g_tex.cat,         "assets/textures/cat.png");
    ok &= load_one(&g_tex.rewards[REWARD_PLANT_STAGE_1],
                   "assets/textures/rewards/plant_stage_1.png");
    ok &= load_one(&g_tex.rewards[REWARD_PLANT_STAGE_2],
                   "assets/textures/rewards/plant_stage_2.png");
    ok &= load_one(&g_tex.rewards[REWARD_PLANT_STAGE_3],
                   "assets/textures/rewards/plant_stage_3.png");
    ok &= load_one(&g_tex.rewards[REWARD_PLANT_STAGE_4],
                   "assets/textures/rewards/plant_stage_4.png");
    ok &= load_one(&g_tex.rewards[REWARD_RUG],
                   "assets/textures/rewards/rug.png");
    ok &= load_one(&g_tex.rewards[REWARD_WALL_ART],
                   "assets/textures/rewards/wall_art.png");
    ok &= load_one(&g_tex.rewards[REWARD_STUDY_DESK],
                   "assets/textures/rewards/study_desk.png");
    ok &= load_one(&g_tex.rewards[REWARD_POT],
                   "assets/textures/rewards/pot.png");
    ok &= load_one(&g_tex.rewards[REWARD_FLOOR_LAMP],
                   "assets/textures/rewards/floor_lamp.png");
    ok &= load_one(&g_tex.rewards[REWARD_BOOKCASE],
                   "assets/textures/rewards/bookcase.png");
    ok &= load_one(&g_tex.rewards[REWARD_CHAIR],
                   "assets/textures/rewards/chair.png");
    ok &= load_one(&g_tex.rewards[REWARD_BED],
                   "assets/textures/rewards/bed.png");
    ok &= load_one(&g_tex.rewards[REWARD_ROUND_RUG],
                   "assets/textures/rewards/round_rug.png");
    g_tex.loaded = ok;
    return ok;
}

void textures_free_all() {
    texture_free(&g_tex.home_layer1);
    texture_free(&g_tex.home_layer2);
    texture_free(&g_tex.character);
    texture_free(&g_tex.cat);
    for (int i = 0; i < REWARD_TEXTURE_COUNT; i++) {
        texture_free(&g_tex.rewards[i]);
    }
    g_tex.loaded = false;
}
