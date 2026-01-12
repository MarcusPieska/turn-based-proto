//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef IMG_MNG_H
#define IMG_MNG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char u8;

typedef struct size {
    int width, height;
} size;

typedef struct color {
    u8 h, s, b;
} color;

void reduce_colors (u8* img, size img_size, int channels, u8* old_color, u8* new_color, color range, u8* pixel_map);

#ifdef __cplusplus
}
#endif

#endif // IMG_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
