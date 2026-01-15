//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include "img_mng.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

inline double deg_to_rad (double degrees) {
    return degrees * M_PI / 180.0;
}

inline void rot_pt_inv (pt in, pt center, double cos_a, double sin_a, pt& out) {
    double dx = in.x - center.x, dy = in.y - center.y;
    out.x = center.x + dx * cos_a + dy * sin_a;
    out.y = center.y - dx * sin_a + dy * cos_a;
}

void bilinear_interpolate (u8* img_data, size img_size, int channels, pt src, u8* result, u8* def_color) {
    int x = static_cast<int>(std::round(src.x));
    int y = static_cast<int>(std::round(src.y));
    if (x < 0 || y < 0 || x >= img_size.width || y >= img_size.height) {
        std::memcpy(result, def_color, channels * sizeof(u8));
        return;
    }
    int pos = (y * img_size.width + x) * channels;
    std::memcpy(result, &img_data[pos], channels * sizeof(u8));
}

inline bool colors_match (u8* a, u8* b, int ch) {
    for (int i = 0; i < ch; i++) if (a[i] != b[i]) return false;
    return true;
}

//================================================================================================================================
//=> - Main public functions -
//================================================================================================================================

extern "C" {

void rotate_img (u8* img_in, u8* img_out, u8* def_color, size img_size, int channels, pt center, double degrees) {
    double angle_rad = deg_to_rad (degrees);
    double cos_a = std::cos(angle_rad);
    double sin_a = std::sin(angle_rad);
    
    for (int y = 0; y < img_size.height; y++) {
        for (int x = 0; x < img_size.width; x++) {
            pt src_pt;
            rot_pt_inv ({static_cast<double>(x), static_cast<double>(y)}, center, cos_a, sin_a, src_pt);
            int out_pos = (y * img_size.width + x) * channels;
            bilinear_interpolate (img_in, img_size, channels, src_pt, &img_out[out_pos], def_color);
        }
    }
}

void flood_fill (u8* img, size img_size, int channels, u8* lim_color, u8* fill_color, pt start) {
    int sx = static_cast<int>(start.x);
    int sy = static_cast<int>(start.y);
    if (sx < 0 || sx >= img_size.width || sy < 0 || sy >= img_size.height) {
        return;
    }
    int start_pos = (sy * img_size.width + sx) * channels;
    if (colors_match(&img[start_pos], lim_color, channels) || colors_match(&img[start_pos], fill_color, channels)) {
        return;
    }
    std::vector<int> stack;
    stack.push_back(sy * img_size.width + sx);
    while (!stack.empty()) {
        int idx = stack.back();
        stack.pop_back();
        int x = idx % img_size.width, y = idx / img_size.width;
        int pos = (y * img_size.width + x) * channels;
        if (x < 0 || x >= img_size.width || y < 0 || y >= img_size.height) {
            continue;
        }
        if (colors_match(&img[pos], lim_color, channels) || colors_match(&img[pos], fill_color, channels)) {
            continue;
        }
        std::memcpy(&img[pos], fill_color, channels * sizeof(u8));
        stack.push_back((y - 1) * img_size.width + x);
        stack.push_back((y + 1) * img_size.width + x);
        stack.push_back(y * img_size.width + (x - 1));
        stack.push_back(y * img_size.width + (x + 1));
    }
}

void find_margins (u8* img, size img_size, int channels, u8* bg_color, margins* out) {
    out->left = img_size.width;
    out->right = -1;
    out->top = img_size.height;
    out->bottom = -1;
    for (int y = 0; y < img_size.height; y++) {
        for (int x = 0; x < img_size.width; x++) {
            int pos = (y * img_size.width + x) * channels;
            if (!colors_match(&img[pos], bg_color, channels)) {
                if (x < out->left)   { out->left   = x; }
                if (x > out->right)  { out->right  = x; }
                if (y < out->top)    { out->top    = y; }
                if (y > out->bottom) { out->bottom = y; }
            }
        }
    }
    out->right = img_size.width - 1 - out->right;
    out->bottom = img_size.height - 1 - out->bottom;
}

void shift_img (u8* img_in, u8* img_out, size img_size, int channels, int dx, int dy, u8* def_color) {
    int row_size = img_size.width * channels;
    for (int y = 0; y < img_size.height; y++) {
        std::memcpy(&img_out[y * row_size], def_color, row_size);
    }
    int src_y0 = std::max(0, -dy);
    int src_y1 = std::min(img_size.height, img_size.height - dy);
    int src_x0 = std::max(0, -dx);
    int src_x1 = std::min(img_size.width, img_size.width - dx);
    int dst_y0 = std::max(0, dy);
    int dst_x0 = std::max(0, dx);
    int copy_h = src_y1 - src_y0;
    int copy_w = src_x1 - src_x0;
    if (copy_h > 0 && copy_w > 0) {
        int copy_row_size = copy_w * channels;
        for (int y = 0; y < copy_h; y++) {
            int src_row = (src_y0 + y) * row_size + src_x0 * channels;
            int dst_row = (dst_y0 + y) * row_size + dst_x0 * channels;
            std::memcpy(&img_out[dst_row], &img_in[src_row], copy_row_size);
        }
    }
}

void get_outline (u8* img_in, u8* img_out, size img_size, int channels, u8* bg_color) {
    u8 white[3] = {255, 255, 255};
    u8 black[3] = {0, 0, 0};
    int outline_channels = 3;
    for (int y = 0; y < img_size.height; y++) {
        for (int x = 0; x < img_size.width; x++) {
            int pos = (y * img_size.width + x) * channels;
            int out_pos = (y * img_size.width + x) * outline_channels;
            if (colors_match(&img_in[pos], bg_color, channels)) {
                bool has_adjacent_non_bg = false;
                int dx[] = {-1, 1, 0, 0};
                int dy[] = {0, 0, -1, 1};
                for (int d = 0; d < 4; d++) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (nx >= 0 && nx < img_size.width && ny >= 0 && ny < img_size.height) {
                        int npos = (ny * img_size.width + nx) * channels;
                        if (!colors_match(&img_in[npos], bg_color, channels)) {
                            has_adjacent_non_bg = true;
                            break;
                        }
                    }
                }
                if (has_adjacent_non_bg) {
                    std::memcpy(&img_out[out_pos], white, outline_channels * sizeof(u8));
                } else {
                    std::memcpy(&img_out[out_pos], black, outline_channels * sizeof(u8));
                }
            } else {
                std::memcpy(&img_out[out_pos], black, outline_channels * sizeof(u8));
            }
        }
    }
}

void apply_outline (u8* img_main, u8* img_outline, size img_size, int channels, u8* color_outline) {
    int outline_channels = 3;
    for (int y = 0; y < img_size.height; y++) {
        for (int x = 0; x < img_size.width; x++) {
            int outline_pos = (y * img_size.width + x) * outline_channels;
            if (colors_match(&img_outline[outline_pos], color_outline, outline_channels)) {
                int main_pos = (y * img_size.width + x) * channels;
                std::memcpy(&img_main[main_pos], color_outline, channels * sizeof(u8));
            }
        }
    }
}

}

//================================================================================================================================
//=> - End -
//================================================================================================================================
