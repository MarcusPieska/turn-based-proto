//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include "map_to_map.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

inline double rgb_to_hue (u8 r, u8 g, u8 b) {
    double r_norm = r / 255.0;
    double g_norm = g / 255.0;
    double b_norm = b / 255.0;
    double max_val = std::max({r_norm, g_norm, b_norm});
    double min_val = std::min({r_norm, g_norm, b_norm});
    double delta = max_val - min_val;
    if (delta < 0.0001) {
        return 0.0;
    }
    double hue = 0.0;
    if (max_val == r_norm) {
        hue = 60.0 * (((g_norm - b_norm) / delta));
    } else if (max_val == g_norm) {
        hue = 60.0 * (2.0 + ((b_norm - r_norm) / delta));
    } else {
        hue = 60.0 * (4.0 + ((r_norm - g_norm) / delta));
    }
    if (hue < 0.0) {
        hue += 360.0;
    }
    return hue * 255.0 / 360.0;
}

inline double rgb_to_brightness (u8 r, u8 g, u8 b) {
    double r_norm = r / 255.0;
    double g_norm = g / 255.0;
    double b_norm = b / 255.0;
    double max_val = std::max({r_norm, g_norm, b_norm});
    return max_val * 255.0;
}

inline double rgb_to_saturation (u8 r, u8 g, u8 b) {
    double r_norm = r / 255.0;
    double g_norm = g / 255.0;
    double b_norm = b / 255.0;
    double max_val = std::max({r_norm, g_norm, b_norm});
    double min_val = std::min({r_norm, g_norm, b_norm});
    double delta = max_val - min_val;
    if (max_val < 0.0001) {
        return 0.0;
    }
    return (delta / max_val) * 255.0;
}

inline bool hue_in_range (double pixel_hue, double target_hue, int hue_range) {
    double diff = std::abs(pixel_hue - target_hue);
    if (diff > 127.5) {
        diff = 255.0 - diff;
    }
    return diff <= static_cast<double>(hue_range);
}

inline bool brightness_in_range (double pixel_brightness, double target_brightness, int brightness_range) {
    double diff = std::abs(pixel_brightness - target_brightness);
    return diff <= static_cast<double>(brightness_range);
}

inline bool saturation_in_range (double pixel_saturation, double target_saturation, int saturation_range) {
    double diff = std::abs(pixel_saturation - target_saturation);
    return diff <= static_cast<double>(saturation_range);
}

//================================================================================================================================
//=> - Main public functions -
//================================================================================================================================

extern "C" {

void reduce_colors (u8* img, size img_size, int channels, u8* old_color, u8* new_color, color range, u8* pixel_map) {
    if (channels < 3) {
        return;
    }
    double target_hue = rgb_to_hue(old_color[0], old_color[1], old_color[2]);
    double target_brightness = rgb_to_brightness(old_color[0], old_color[1], old_color[2]);
    double target_saturation = rgb_to_saturation(old_color[0], old_color[1], old_color[2]);
    for (int y = 0; y < img_size.height; y++) {
        for (int x = 0; x < img_size.width; x++) {
            int pos = (y * img_size.width + x) * channels;
            u8 r = img[pos];
            u8 g = img[pos + 1];
            u8 b = img[pos + 2];
            double pixel_hue = rgb_to_hue(r, g, b);
            double pixel_brightness = rgb_to_brightness(r, g, b);
            double pixel_saturation = rgb_to_saturation(r, g, b);
            if (hue_in_range(pixel_hue, target_hue, range.h) &&
                brightness_in_range(pixel_brightness, target_brightness, range.b) &&
                saturation_in_range(pixel_saturation, target_saturation, range.s)) {
                std::memcpy(&img[pos], new_color, channels * sizeof(u8));
                pixel_map[y * img_size.width + x] = 1;
            } else {
                pixel_map[y * img_size.width + x] = 0;
            }
        }
    }
}

}

//================================================================================================================================
//=> - End -
//================================================================================================================================

