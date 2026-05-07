#include "pet.h"

void pet_init(Pet& p) {
    p.x = 300;
    p.y = 300;
    p.facing = 0;
    p.walk_phase = 0.0f;
    p.visible = true;
}
