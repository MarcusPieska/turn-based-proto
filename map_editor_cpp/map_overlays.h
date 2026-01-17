//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_OVERLAYS_H
#define MAP_OVERLAYS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char u8;

typedef struct pt {
    double x, y;
} pt;

typedef struct size {
    int width, height;
} size;

typedef struct margins {
    int left, right, top, bottom;
} margins;

void distance_overlay_brute_force (u8* img, size img_size, int channels, u8* bg_color, int* dist_out);
void distance_overlay (u8* img, size img_size, int channels, u8* bg_color, int* dist_out);
void distance_overlay_block (u8* img, size img_size, int channels, u8* bg_color, u8* block_color, int* dist_out);
void distance_mask (int* dist_in, size img_size, int lower_limit, int upper_limit, u8* mask_out);
void wander_up (int* dist_in, size img_size, pt start, pt* end_out);
void wander_down (int* dist_in, size img_size, pt start, int min_limit, pt* end_out);
bool wander_up_radius (int* dist_in, size img_size, pt start, int radius, pt* end_out);
bool wander_down_radius (int* dist_in, size img_size, pt start, int radius, int min_limit, pt* end_out);

#ifdef __cplusplus
}
#endif

#endif // MAP_OVERLAYS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
