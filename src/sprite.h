#pragma once

#include "texture.h"

// Draw a sub-rectangle of `tex` (in texel pixel coords, top-left origin)
// to a screen-space rectangle (bottom-left origin, like ortho 2D).
//
// dx, dy = bottom-left corner on screen
// dw, dh = destination size on screen (in pixels)
// sx, sy, sw, sh = source rect in texture pixels (sy measured from top of texture)
void sprite_draw(const Texture& tex,
                 float dx, float dy, float dw, float dh,
                 int sx, int sy, int sw, int sh);

// Draw centered at (cx, cy)
void sprite_draw_centered(const Texture& tex,
                          float cx, float cy, float dw, float dh,
                          int sx, int sy, int sw, int sh);

// Tinted version (multiplies texel by RGB color)
void sprite_draw_tinted(const Texture& tex,
                        float dx, float dy, float dw, float dh,
                        int sx, int sy, int sw, int sh,
                        float r, float g, float b, float a);
