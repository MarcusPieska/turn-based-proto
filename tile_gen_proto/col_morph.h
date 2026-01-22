//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef COL_MORPH_H
#define COL_MORPH_H

typedef unsigned char u8;

typedef struct pt {
    int x, y;
} pt;

typedef struct deltas {
    int top_dy, right_dy, bottom_dy, left_dy;
} deltas;

#ifdef __cplusplus
extern "C" {
#endif

void morph_tile (u8* src_img, int src_w, int src_h, int channels, pt top, pt right, pt bottom, pt left, deltas d, u8* dst_img);

#ifdef __cplusplus
}
#endif

//================================================================================================================================
//=> - End -
//================================================================================================================================

#endif
