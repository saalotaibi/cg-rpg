#pragma once

// The player avatar. Position is in pixel coordinates relative to the
// home backdrop's top-left (Layout::home_x, Layout::home_y).
//
// player_init() is the only function this header guarantees. Rendering
// lives in home.cpp (Agent A) and AI / movement lives in ai.cpp (Agent C).
struct Player {
    float x, y;
    int   facing;        // 0=down, 1=left, 2=right, 3=up
    float walk_phase;
    bool  moving;        // true if the avatar moved this frame
};

void player_init(Player& p);
