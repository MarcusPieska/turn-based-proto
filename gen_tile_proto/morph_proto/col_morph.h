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

void morph_tile (SDL_Texture* src_tex, SDL_Texture* dst_tex, size src_size, int channels, tile_pts pts, deltas d);

#endif

//================================================================================================================================
//=> - End -
//================================================================================================================================
