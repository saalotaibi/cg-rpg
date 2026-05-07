#pragma once

// Pixel bitmap font: 5x7 glyphs rendered as filled quads.
// Stardew-style chunky retro look, no external deps.

void font_draw(float x, float y, float scale, const char* text,
               float r, float g, float b);
float font_width(float scale, const char* text);
