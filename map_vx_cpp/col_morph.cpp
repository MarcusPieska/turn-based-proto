//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>
#include <cmath>
#include <algorithm>

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static inline bool is_black (u8* img, int idx) {
    return img[idx + 0] == 0 && img[idx + 1] == 0 && img[idx + 2] == 0;
}

static inline bool is_transparent (u8* img, int idx) {
    return img[idx + 3] == 0;
}

static inline void find_old_column_bounds (u8* img, int w, int h, int x, int& y_low, int& y_high) {
    y_low = -1;
    y_high = -1;
    for (int y = h - 1; y >= 0; y--) {
        int idx = (y * w + x) * 4;
        if (!is_transparent(img, idx)) {
            y_low = y; 
            break;
        }
    }
    for (int y = 0; y < h; y++) {
        int idx = (y * w + x) * 4;
        if (!is_transparent(img, idx)) {
            y_high = y;
            break;
        }
    }
}

static inline void find_new_column_bounds (u8* img, int w, int h, int x, int& y_low, int& y_high) {
    y_low = -1;
    y_high = -1;
    for (int y = h - 1; y >= 0; y--) {
        int idx = (y * w + x) * 4;
        if (is_black(img, idx)) {
            y_low = y; 
            break;
        }
    }
    for (int y = 0; y < h; y++) {
        int idx = (y * w + x) * 4;
        if (is_black(img, idx)) {
            y_high = y;
            break;
        }
    }
}

static inline void fit_old_column_to_new_column (u8* src_img, int src_w, int src_h, u8* dst_img, int x, int y_low1, int y_high1, int y_low2, int y_high2) {
    if (y_low1 < 0 || y_high1 < 0 || y_low2 < 0 || y_high2 < 0) return;
    int old_height = y_low1 - y_high1 + 1;
    int new_height = y_low2 - y_high2 + 1;
    if (old_height <= 0 || new_height <= 0) return;
    const int FIXED_SHIFT = 16;
    const int FIXED_ONE = 1 << FIXED_SHIFT;
    int old_height_minus_1 = old_height - 1;
    int new_height_minus_1 = new_height - 1;
    int scale_fixed = (new_height_minus_1 == 0) ? 0 : (old_height_minus_1 * FIXED_ONE) / new_height_minus_1;
    for (int dst_y = y_high2; dst_y <= y_low2; dst_y++) {
        int offset = dst_y - y_high2;
        int src_y_fixed = (new_height_minus_1 == 0) ? y_high1 * FIXED_ONE : y_high1 * FIXED_ONE + offset * scale_fixed;
        int src_y0 = src_y_fixed >> FIXED_SHIFT;
        int src_y1 = src_y0 + 1;
        int fy_fixed = src_y_fixed & (FIXED_ONE - 1);
        src_y0 = std::max(y_high1, std::min(y_low1, src_y0));
        src_y1 = std::max(y_high1, std::min(y_low1, src_y1));
        int dst_idx = (dst_y * src_w + x) * 4;
        int src_idx0 = (src_y0 * src_w + x) * 4;
        int src_idx1 = (src_y1 * src_w + x) * 4;
        for (int c = 0; c < 4; c++) {
            int val = (src_img[src_idx0 + c] * (FIXED_ONE - fy_fixed) + src_img[src_idx1 + c] * fy_fixed + (FIXED_ONE >> 1)) >> FIXED_SHIFT;
            dst_img[dst_idx + c] = (u8)val;
        }
    }
}

static inline void draw_line_thick (u8* img, int w, int h, pt p1, pt p2, int thickness) {
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
                if (px >= 0 && px < w && py >= 0 && py < h) {
                    int idx = (py * w + px) * 4;
                    img[idx + 0] = 0;
                    img[idx + 1] = 0;
                    img[idx + 2] = 0;
                    img[idx + 3] = 255;
                }
            }
        }
        if (x == p2.x && y == p2.y) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

//================================================================================================================================
//=> - C functions -
//================================================================================================================================

void morph_tile (u8* src_img, int src_w, int src_h, pt top, pt right, pt bottom, pt left, deltas d, u8* dst_img) {
    int total_pixels = src_w * src_h;
    for (int i = 0; i < total_pixels; i++) {
        dst_img[i * 4 + 0] = 255;
        dst_img[i * 4 + 1] = 0;
        dst_img[i * 4 + 2] = 255;
        dst_img[i * 4 + 3] = 0;
    }
    pt new_top = {top.x, top.y + d.top_dy};
    pt new_right = {right.x, right.y + d.right_dy};
    pt new_bottom = {bottom.x, bottom.y + d.bottom_dy};
    pt new_left = {left.x, left.y + d.left_dy};
    draw_line_thick(dst_img, src_w, src_h, new_top, new_right, 1);
    draw_line_thick(dst_img, src_w, src_h, new_right, new_bottom, 1);
    draw_line_thick(dst_img, src_w, src_h, new_bottom, new_left, 1);
    draw_line_thick(dst_img, src_w, src_h, new_left, new_top, 1);
    int min_x = std::max(0, new_left.x);
    int max_x = std::min(src_w - 1, new_right.x);
    int y_low1, y_high1, y_low2, y_high2;
    for (int x = min_x; x <= max_x; x++) {
        find_old_column_bounds (src_img, src_w, src_h, x, y_low1, y_high1);
        find_new_column_bounds (dst_img, src_w, src_h, x, y_low2, y_high2);
        fit_old_column_to_new_column (src_img, src_w, src_h, dst_img, x, y_low1, y_high1, y_low2, y_high2);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
