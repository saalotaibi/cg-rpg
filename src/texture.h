#pragma once

struct Texture {
    unsigned int id;
    int w, h;
};

bool texture_load(Texture* out, const char* path);
void texture_free(Texture* t);
