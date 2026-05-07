#include "font.h"
#include "gl_compat.h"
#include <cctype>
#include <cstdint>
#include <cstring>

// 5x7 bitmap font. Each glyph = 7 rows, each row = 5 bits (bit 4 = leftmost).
// Only uppercase + digits + select punctuation; lowercase is mapped to uppercase.
// 0 = no glyph.

static const uint8_t G_A[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
static const uint8_t G_B[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
static const uint8_t G_C[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
static const uint8_t G_D[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
static const uint8_t G_E[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
static const uint8_t G_F[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
static const uint8_t G_G[7] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E};
static const uint8_t G_H[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
static const uint8_t G_I[7] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
static const uint8_t G_J[7] = {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C};
static const uint8_t G_K[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
static const uint8_t G_L[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
static const uint8_t G_M[7] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
static const uint8_t G_N[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
static const uint8_t G_O[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t G_P[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
static const uint8_t G_Q[7] = {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
static const uint8_t G_R[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
static const uint8_t G_S[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
static const uint8_t G_T[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
static const uint8_t G_U[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t G_V[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
static const uint8_t G_W[7] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11};
static const uint8_t G_X[7] = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
static const uint8_t G_Y[7] = {0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04};
static const uint8_t G_Z[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};

static const uint8_t G_0[7] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
static const uint8_t G_1[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
static const uint8_t G_2[7] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
static const uint8_t G_3[7] = {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E};
static const uint8_t G_4[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
static const uint8_t G_5[7] = {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E};
static const uint8_t G_6[7] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
static const uint8_t G_7[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
static const uint8_t G_8[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
static const uint8_t G_9[7] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C};

static const uint8_t G_DOT[7]   = {0,0,0,0,0,0,0x04};
static const uint8_t G_COL[7]   = {0,0x04,0,0,0,0x04,0};
static const uint8_t G_EXC[7]   = {0x04,0x04,0x04,0x04,0,0,0x04};
static const uint8_t G_QUE[7]   = {0x0E,0x11,0x01,0x02,0x04,0,0x04};
static const uint8_t G_DASH[7]  = {0,0,0,0x0E,0,0,0};
static const uint8_t G_SLASH[7] = {0x01,0x01,0x02,0x04,0x08,0x10,0x10};
static const uint8_t G_LB[7]    = {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E};
static const uint8_t G_RB[7]    = {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E};
static const uint8_t G_PCT[7]   = {0x18,0x19,0x02,0x04,0x08,0x13,0x03};
static const uint8_t G_COM[7]   = {0,0,0,0,0,0x0C,0x04};
static const uint8_t G_APO[7]   = {0x04,0x04,0,0,0,0,0};
static const uint8_t G_PLUS[7]  = {0,0x04,0x04,0x1F,0x04,0x04,0};
static const uint8_t G_HASH[7]  = {0x0A,0x0A,0x1F,0x0A,0x1F,0x0A,0x0A};

static const uint8_t* glyph_for(char c) {
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    switch (c) {
        case 'A': return G_A; case 'B': return G_B; case 'C': return G_C;
        case 'D': return G_D; case 'E': return G_E; case 'F': return G_F;
        case 'G': return G_G; case 'H': return G_H; case 'I': return G_I;
        case 'J': return G_J; case 'K': return G_K; case 'L': return G_L;
        case 'M': return G_M; case 'N': return G_N; case 'O': return G_O;
        case 'P': return G_P; case 'Q': return G_Q; case 'R': return G_R;
        case 'S': return G_S; case 'T': return G_T; case 'U': return G_U;
        case 'V': return G_V; case 'W': return G_W; case 'X': return G_X;
        case 'Y': return G_Y; case 'Z': return G_Z;
        case '0': return G_0; case '1': return G_1; case '2': return G_2;
        case '3': return G_3; case '4': return G_4; case '5': return G_5;
        case '6': return G_6; case '7': return G_7; case '8': return G_8;
        case '9': return G_9;
        case '.': return G_DOT; case ':': return G_COL; case '!': return G_EXC;
        case '?': return G_QUE; case '-': return G_DASH; case '/': return G_SLASH;
        case '[': return G_LB;  case ']': return G_RB;   case '%': return G_PCT;
        case ',': return G_COM; case '\'': return G_APO; case '+': return G_PLUS;
        case '#': return G_HASH;
    }
    return nullptr;
}

static void draw_pixel(float x, float y, float s) {
    glBegin(GL_QUADS);
    glVertex2f(x,     y);
    glVertex2f(x + s, y);
    glVertex2f(x + s, y + s);
    glVertex2f(x,     y + s);
    glEnd();
}

void font_draw(float x, float y, float scale, const char* text,
               float r, float g, float b) {
    glColor3f(r, g, b);
    float cx = x;
    for (const char* p = text; *p; p++) {
        if (*p == ' ') { cx += 6 * scale; continue; }
        const uint8_t* g_ptr = glyph_for(*p);
        if (!g_ptr) { cx += 6 * scale; continue; }
        for (int row = 0; row < 7; row++) {
            uint8_t bits = g_ptr[row];
            for (int col = 0; col < 5; col++) {
                if (bits & (1 << (4 - col))) {
                    float px = cx + col * scale;
                    float py = y + (6 - row) * scale; // flip vertically: row 0 at top
                    draw_pixel(px, py, scale);
                }
            }
        }
        cx += 6 * scale;
    }
}

float font_width(float scale, const char* text) {
    return strlen(text) * 6 * scale;
}
