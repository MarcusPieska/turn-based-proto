//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef COL_MORPH_H
#define COL_MORPH_H

#include <SDL2/SDL.h>

typedef unsigned char u8;

typedef struct size {
    int width, height;
} size;

typedef struct pt {
    int x, y;
} pt;

typedef struct tile_pts {
    pt top, right, bottom, left;
} tile_pts;

typedef struct deltas {
    int top_dy, right_dy, bottom_dy, left_dy;
} deltas;

SDL_Texture* morph_tile (SDL_Renderer* rend, SDL_Texture* src_tex, size src_size, int ch, tile_pts pts, deltas d, int& new_h);

#endif

//================================================================================================================================
//=> - End -
//================================================================================================================================
