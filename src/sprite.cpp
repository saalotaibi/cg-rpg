#include "sprite.h"
#include "gl_compat.h"

// Note: stb_image is loaded with vertical flip = ON, so the texture's V=0 is at
// the bottom of the image. We compensate so callers can pass top-left source coords.
//
// Every sprite is positioned via glTranslatef and sized via glScalef applied to
// a unit quad at (0,0)..(1,1). Using the modelview matrix here means the GL
// transformation pipeline (translation + scaling) drives every textured draw
// in the game rather than precomputing screen-space vertices.
static void emit_quad(const Texture& tex,
                      float dx, float dy, float dw, float dh,
                      int sx, int sy, int sw, int sh) {
    float u0 = (float)sx / tex.w;
    float u1 = (float)(sx + sw) / tex.w;
    // sy is top-down pixel from top; convert to V (bottom-up after flip):
    float v0 = 1.0f - (float)(sy + sh) / tex.h;
    float v1 = 1.0f - (float)sy / tex.h;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(dx, dy, 0.0f);
    glScalef(dw, dh, 1.0f);

    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(u1, v0); glVertex2f(1.0f, 0.0f);
    glTexCoord2f(u1, v1); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(u0, v1); glVertex2f(0.0f, 1.0f);
    glEnd();

    glPopMatrix();
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
