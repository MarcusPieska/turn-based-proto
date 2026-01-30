//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <SDL2/SDL.h>

#include "col_morph.h"

//================================================================================================================================
//=> - Shading macros -
//================================================================================================================================

#define SHADE_MULT_MIN 0.9f
#define SHADE_MULT_MAX 1.1f
#define SHADE_MAX_DIFF 31
#define PLAIN_FACTOR_MAX_HEIGHT_DIFF 10

//================================================================================================================================
//=> - Morphing y-values -
//================================================================================================================================

typedef struct y_vals {
    int old_y_low, old_y_high, new_y_low, new_y_high;
} y_vals;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static inline bool is_black (u8* img, int pitch, int x, int y) {
    int idx = y * pitch + x * 4;
    return img[idx + 0] == 0 && img[idx + 1] == 0 && img[idx + 2] == 0;
}

static inline bool is_magenta (u8* img, int pitch, int x, int y) {
    int idx = y * pitch + x * 4;
    return img[idx + 0] == 255 && img[idx + 1] == 0 && img[idx + 2] == 255;
}

static inline void find_old_col_bounds (u8* img, size img_size, int pitch, int x, int& y_low, int& y_high) {
    y_low = -1;
    y_high = -1;
    for (int y = img_size.height - 1; y >= 0; y--) {
        if (!is_magenta(img, pitch, x, y)) {
            y_low = y; 
            break;
        }
    }
    for (int y = 0; y < img_size.height; y++) {
        if (!is_magenta(img, pitch, x, y)) {
            y_high = y;
            break;
        }
    }
}

static inline void find_new_col_bounds (u8* img, size img_size, int pitch, int x, int& y_low, int& y_high) {
    y_low = 0;
    y_high = img_size.height - 1;
    for (int y = img_size.height - 1; y >= 0; y--) {
        if (is_black(img, pitch, x, y)) {
            y_low = y; 
            break;
        }
    }
    for (int y = 0; y < img_size.height; y++) {
        if (is_black(img, pitch, x, y)) {
            y_high = y;
            break;
        }
    }
}

static inline void fit_old_col_to_new_col (u8* img1, u8* img2, int ch, int pitch1, int pitch2, int x, y_vals yv, float shade) {
    if (yv.old_y_low < 0 || yv.old_y_high < 0 || yv.new_y_low < 0 || yv.new_y_high < 0) {
        return;
    }
    
    int old_height = yv.old_y_low - yv.old_y_high + 1;
    int new_height = yv.new_y_low - yv.new_y_high + 1;
    
    if (old_height <= 0 || new_height <= 0) {
        return;
    }
    
    const int FIXED_SHIFT = 16;
    const int FIXED_ONE = 1 << FIXED_SHIFT;
    
    int old_height_minus_1 = old_height - 1;
    int new_height_minus_1 = new_height - 1;
    
    int scale_fixed;
    if (new_height_minus_1 == 0) {
        scale_fixed = 0;  
    } else {
        scale_fixed = (old_height_minus_1 * FIXED_ONE) / new_height_minus_1;
    }
    
    for (int dst_y = yv.new_y_high; dst_y <= yv.new_y_low; dst_y++) {
        int offset = dst_y - yv.new_y_high;
        int src_y_fixed;
        if (new_height_minus_1 == 0) {
            src_y_fixed = yv.old_y_high * FIXED_ONE; 
        } else {
            src_y_fixed = yv.old_y_high * FIXED_ONE + offset * scale_fixed;
        }
        
        int src_y0 = src_y_fixed >> FIXED_SHIFT;
        int src_y1 = src_y0 + 1;
        int fy_fixed = src_y_fixed & (FIXED_ONE - 1); 
        
        src_y0 = std::max(yv.old_y_high, std::min(yv.old_y_low, src_y0));
        src_y1 = std::max(yv.old_y_high, std::min(yv.old_y_low, src_y1));
        
        int dst_idx = dst_y * pitch2 + x * 4;
        int src_idx0 = src_y0 * pitch1 + x * 4;
        int src_idx1 = src_y1 * pitch1 + x * 4;
        
        for (int c = 0; c < ch && c < 3; c++) {
            int val0 = img1[src_idx0 + c];
            int val1 = img1[src_idx1 + c];
            int val = (val0 * (FIXED_ONE - fy_fixed) + val1 * fy_fixed + (FIXED_ONE >> 1)) >> FIXED_SHIFT;
            int shaded_val = (int)(val * shade + 0.5f);
            img2[dst_idx + c] = (u8)std::max(0, std::min(255, shaded_val));
        }
        if (ch >= 4) {
            int alpha0 = img1[src_idx0 + 3];
            int alpha1 = img1[src_idx1 + 3];
            int alpha = (alpha0 * (FIXED_ONE - fy_fixed) + alpha1 * fy_fixed + (FIXED_ONE >> 1)) >> FIXED_SHIFT;
            img2[dst_idx + 3] = (u8)alpha;
        }
    }
}

static inline void draw_line_thick (u8* img, size img_size, int ch, int pitch, pt p1, pt p2, int thickness) {
    int dx = abs(p2.x - p1.x);
    int dy = abs(p2.y - p1.y);
    int sx = (p1.x < p2.x) ? 1 : -1;
    int sy = (p1.y < p2.y) ? 1 : -1;
    int err = dx - dy;
    
    int x = p1.x;
    int y = p1.y;
    
    int half_thick = thickness / 2;
    while (true) {
        for (int dy_off = -half_thick; dy_off <= half_thick; dy_off++) {
            for (int dx_off = -half_thick; dx_off <= half_thick; dx_off++) {
                int px = x + dx_off;
                int py = y + dy_off;
                
                if (px >= 0 && px < img_size.width && py >= 0 && py < img_size.height) {
                    int idx = py * pitch + px * 4;
                    for (int c = 0; c < ch && c < 3; c++) {
                        img[idx + c] = 0; 
                    }
                    if (ch >= 4) {
                        img[idx + 3] = 255;
                    }
                }
            }
        }
        
        if (x == p2.x && y == p2.y) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

static inline void adjust_pts_to_bottom (pt &top, pt &right, pt &bottom, pt &left, int &height) {
    int min_y = std::min(right.y, std::min(left.y, std::min(top.y, bottom.y)));
    int max_y = std::max(right.y, std::max(left.y, std::max(top.y, bottom.y)));
    top.y = top.y - min_y;
    right.y = right.y - min_y;
    bottom.y = bottom.y - min_y;
    left.y = left.y - min_y;
    height = max_y - min_y;
}

//================================================================================================================================
//=> - Functions -
//================================================================================================================================

SDL_Texture* morph_tile (SDL_Renderer* rend, SDL_Texture* src_tex, size src_size, int ch, tile_pts pts, deltas d, int& new_h) {
    SDL_Texture* dst_tex;
    pt new_top = {pts.top.x, pts.top.y + d.top_dy};
    pt new_right = {pts.right.x, pts.right.y + d.right_dy};
    pt new_bottom = {pts.bottom.x, pts.bottom.y + d.bottom_dy};
    pt new_left = {pts.left.x, pts.left.y + d.left_dy};
    size dst_size = {pts.right.x - pts.left.x, 0};
    adjust_pts_to_bottom(new_top, new_right, new_bottom, new_left, dst_size.height);
    new_h = dst_size.height;
    dst_tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, dst_size.width, dst_size.height);
    SDL_SetTextureBlendMode(dst_tex, SDL_BLENDMODE_BLEND);

    void* src_pixels;
    void* dst_pixels;
    int src_pitch, dst_pitch;
    SDL_LockTexture(src_tex, nullptr, &src_pixels, &src_pitch);
    SDL_LockTexture(dst_tex, nullptr, &dst_pixels, &dst_pitch);
    u8* src_img = (u8*)src_pixels;
    u8* dst_img = (u8*)dst_pixels;
    for (int y = 0; y < dst_size.height; y++) {
        for (int x = 0; x < dst_size.width; x++) {
            int idx = y * dst_pitch + x * 4;
            dst_img[idx + 0] = 255;
            dst_img[idx + 1] = 0;
            dst_img[idx + 2] = 255;
            dst_img[idx + 3] = 0;
        }
    }
    
    float y_diff = (float)(new_left.y - new_right.y);
    float max_diff = (float)SHADE_MAX_DIFF;
    float normalized_diff = (max_diff > 0.0f) ? (y_diff / max_diff) : 0.0f;
    normalized_diff = std::max(-1.0f, std::min(1.0f, normalized_diff));
    
    float shade = SHADE_MULT_MIN + (normalized_diff + 1.0f) * 0.5f * (SHADE_MULT_MAX - SHADE_MULT_MIN);
    float shade_effect = shade - 1.0f;
    int original_height = abs(pts.bottom.y - pts.top.y);
    int morphed_height = abs(new_bottom.y - new_top.y);
    int height_delta = abs(morphed_height - original_height);
    float plain_factor = std::min(1.0f, (float)height_delta / (float)PLAIN_FACTOR_MAX_HEIGHT_DIFF);
    shade = 1.0f + shade_effect * plain_factor;
    
    const int thickness = 3;
    draw_line_thick(dst_img, dst_size, ch, dst_pitch, new_top, new_right, thickness);
    draw_line_thick(dst_img, dst_size, ch, dst_pitch, new_right, new_bottom, thickness);
    draw_line_thick(dst_img, dst_size, ch, dst_pitch, new_bottom, new_left, thickness);
    draw_line_thick(dst_img, dst_size, ch, dst_pitch, new_left, new_top, thickness);

    int min_x = std::max(0, new_left.x);
    int max_x = std::min(dst_size.width - 1, new_right.x);
    y_vals yv;
    for (int x = min_x; x <= max_x; x++) {
        find_old_col_bounds(src_img, src_size, src_pitch, x, yv.old_y_low, yv.old_y_high);
        find_new_col_bounds(dst_img, dst_size, dst_pitch, x, yv.new_y_low, yv.new_y_high);
        fit_old_col_to_new_col(src_img, dst_img, ch, src_pitch, dst_pitch, x, yv, shade);
    }
    
    SDL_UnlockTexture(src_tex);
    SDL_UnlockTexture(dst_tex);
    return dst_tex;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
