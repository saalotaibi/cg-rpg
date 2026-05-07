#pragma once

// The cat companion. Same coordinate convention as Player (relative to the
// home backdrop). Rendering and AI live in agent-owned files.
struct Pet {
    float x, y;
    int   facing;        // 0=down, 1=left, 2=right, 3=up
    float walk_phase;
    bool  visible;
};

void pet_init(Pet& p);
