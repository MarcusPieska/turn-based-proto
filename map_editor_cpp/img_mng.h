//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef IMG_MNG_H
#define IMG_MNG_H

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

void rotate_img (u8* img_in, u8* img_out, u8* def_color, size img_size, int channels, pt center, double degrees);
void flood_fill (u8* img, size img_size, int channels, u8* lim_color, u8* fill_color, pt start);
void find_margins (u8* img, size img_size, int channels, u8* bg_color, margins* out);
void shift_img (u8* img_in, u8* img_out, size img_size, int channels, int dx, int dy, u8* def_color);
void get_outline (u8* img_in, u8* img_out, size img_size, int channels, u8* bg_color);
void apply_outline (u8* img_main, u8* img_outline, size img_size, int channels, u8* color_outline);

#ifdef __cplusplus
}
#endif

#endif // IMG_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
