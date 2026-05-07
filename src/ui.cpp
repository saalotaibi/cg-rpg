#include "ui.h"
#include "font.h"
#include "gl_compat.h"
#include <cmath>

static void quad(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x,     y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
}

void ui_draw_title(int win_w, int win_h, float t) {
    glBegin(GL_QUADS);
    glColor3f(0.10f, 0.12f, 0.25f); glVertex2f(0,     0);
    glColor3f(0.10f, 0.12f, 0.25f); glVertex2f(win_w, 0);
    glColor3f(0.30f, 0.20f, 0.40f); glVertex2f(win_w, win_h);
    glColor3f(0.30f, 0.20f, 0.40f); glVertex2f(0,     win_h);
    glEnd();
    (void)quad;

    float bob = sinf(t * 1.5f) * 6;
    const char* title = "COZY ROOM RPG";
    float scale = 8;
    float w = font_width(scale, title);
    font_draw((win_w - w) / 2 + 4, win_h * 0.65f - 4 + bob, scale, title, 0.05f, 0.05f, 0.10f);
    font_draw((win_w - w) / 2,     win_h * 0.65f + bob,     scale, title, 1.0f,  0.85f, 0.45f);

    const char* sub = "A COZY PRODUCTIVITY COMPANION";
    float sub_scale = 3;
    float sw = font_width(sub_scale, sub);
    font_draw((win_w - sw) / 2, win_h * 0.55f, sub_scale, sub, 0.85f, 0.75f, 0.8f);

    if ((int)(t * 2) % 2 == 0) {
        const char* prompt = "PRESS SPACE FOR ONBOARDING";
        float ps = 4;
        float pw = font_width(ps, prompt);
        font_draw((win_w - pw) / 2, win_h * 0.32f, ps, prompt, 1.0f, 1.0f, 0.6f);
    }

    const char* hint = "WASD MOVE   P START POMODORO   1-9 CHECK ITEM   ESC QUIT";
    float hs = 2;
    font_draw((win_w - font_width(hs, hint)) / 2, win_h * 0.18f, hs, hint, 0.7f, 0.7f, 0.75f);

    const char* hint2 = "ONBOARDING ALWAYS RUNS   R RESET PROFILE";
    float hs2 = 1.6f;
    font_draw((win_w - font_width(hs2, hint2)) / 2, win_h * 0.13f, hs2, hint2,
              0.65f, 0.65f, 0.70f);
}
