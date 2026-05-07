#include "player.h"

void player_init(Player& p) {
    p.x = 200;
    p.y = 200;
    p.facing = 0;
    p.walk_phase = 0.0f;
    p.moving = false;
}
