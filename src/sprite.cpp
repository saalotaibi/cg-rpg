#include "sprite.h"
#include <GL/glew.h>
#include <GL/glut.h>

// Note: stb_image is loaded with vertical flip = ON, so the texture's V=0 is at
// the bottom of the image. We compensate so callers can pass top-left source coords.
static void emit_quad(const Texture& tex,
                      float dx, float dy, float dw, float dh,
                      int sx, int sy, int sw, int sh) {
    float u0 = (float)sx / tex.w;
    float u1 = (float)(sx + sw) / tex.w;
    // sy is top-down pixel from top; convert to V (bottom-up after flip):
    float v0 = 1.0f - (float)(sy + sh) / tex.h;
    float v1 = 1.0f - (float)sy / tex.h;

    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(dx,      dy);
    glTexCoord2f(u1, v0); glVertex2f(dx + dw, dy);
    glTexCoord2f(u1, v1); glVertex2f(dx + dw, dy + dh);
    glTexCoord2f(u0, v1); glVertex2f(dx,      dy + dh);
    glEnd();
}

void sprite_draw(const Texture& tex,
                 float dx, float dy, float dw, float dh,
                 int sx, int sy, int sw, int sh) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glColor4f(1, 1, 1, 1);
    emit_quad(tex, dx, dy, dw, dh, sx, sy, sw, sh);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void sprite_draw_centered(const Texture& tex,
                          float cx, float cy, float dw, float dh,
                          int sx, int sy, int sw, int sh) {
    sprite_draw(tex, cx - dw / 2, cy - dh / 2, dw, dh, sx, sy, sw, sh);
}

void sprite_draw_tinted(const Texture& tex,
                        float dx, float dy, float dw, float dh,
                        int sx, int sy, int sw, int sh,
                        float r, float g, float b, float a) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glColor4f(r, g, b, a);
    emit_quad(tex, dx, dy, dw, dh, sx, sy, sw, sh);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor4f(1, 1, 1, 1);
}
