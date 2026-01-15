//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include <queue>
#include "map_overlays.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static bool colors_match (u8* a, u8* b, int ch) {
    for (int i = 0; i < ch; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

//================================================================================================================================
//=> - Main public functions -
//================================================================================================================================

extern "C" {

void distance_overlay_brute_force (u8* img, size img_size, int channels, u8* bg_color, int* dist_out) {
    for (int i = 0; i < img_size.width * img_size.height; i++) {
        dist_out[i] = -1;
    }
    for (int y = 0; y < img_size.height; y++) {
        for (int x = 0; x < img_size.width; x++) {
            int pos = (y * img_size.width + x) * channels;
            if (colors_match(&img[pos], bg_color, channels)) {
                dist_out[y * img_size.width + x] = 0;
            }
        }
    }
    int curr_dist = 1;
    while (1) {
        int count = 0;
        for (int y = 0; y < img_size.height; y++) {
            for (int x = 0; x < img_size.width; x++) {
                if (dist_out[y * img_size.width + x] == curr_dist - 1) {
                    int dx[] = {-1, 1, 0, 0};
                    int dy[] = {0, 0, -1, 1};
                    for (int d = 0; d < 4; d++) {
                        int nx = x + dx[d];
                        int ny = y + dy[d];
                        if (nx >= 0 && nx < img_size.width && ny >= 0 && ny < img_size.height) {
                            int npos = (ny * img_size.width + nx) * channels;
                            if (!colors_match(&img[npos], bg_color, channels) && dist_out[ny * img_size.width + nx] == -1) {
                                dist_out[ny * img_size.width + nx] = curr_dist;
                                count++;
                            }
                        }
                    }
                }
            }
        }
        if (count == 0) break;
        curr_dist++;
    }
}

void distance_overlay (u8* img, size img_size, int channels, u8* bg_color, int* dist_out) {
    for (int i = 0; i < img_size.width * img_size.height; i++) {
        dist_out[i] = -1;
    }
    std::queue<std::pair<int, int> > q;
    for (int y = 0; y < img_size.height; y++) {
        for (int x = 0; x < img_size.width; x++) {
            int pos = (y * img_size.width + x) * channels;
            if (colors_match(&img[pos], bg_color, channels)) {
                dist_out[y * img_size.width + x] = 0;
                int dx[] = {-1, 1, 0, 0};
                int dy[] = {0, 0, -1, 1};
                for (int d = 0; d < 4; d++) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (nx >= 0 && nx < img_size.width && ny >= 0 && ny < img_size.height) {
                        int npos = (ny * img_size.width + nx) * channels;
                        if (!colors_match(&img[npos], bg_color, channels) && dist_out[ny * img_size.width + nx] == -1) {
                            dist_out[ny * img_size.width + nx] = 1;
                            q.push(std::make_pair(nx, ny));
                        }
                    }
                }
            }
        }
    }
    while (!q.empty()) {
        int x = q.front().first;
        int y = q.front().second;
        q.pop();
        int curr_dist = dist_out[y * img_size.width + x];
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx >= 0 && nx < img_size.width && ny >= 0 && ny < img_size.height) {
                int npos = (ny * img_size.width + nx) * channels;
                if (!colors_match(&img[npos], bg_color, channels) && dist_out[ny * img_size.width + nx] == -1) {
                    dist_out[ny * img_size.width + nx] = curr_dist + 1;
                    q.push(std::make_pair(nx, ny));
                }
            }
        }
    }
}

void distance_mask (int* dist_in, size img_size, int lower_limit, int upper_limit, u8* mask_out) {
    for (int i = 0; i < img_size.width * img_size.height; i++) {
        if (dist_in[i] >= lower_limit && dist_in[i] <= upper_limit) {
            mask_out[i] = 255;
        } else {
            mask_out[i] = 0;
        }
    }
}

void wander_up (int* dist_in, size img_size, pt start, pt* end_out) {
    int x = (int)start.x;
    int y = (int)start.y;
    for (int step = 0; step < 1000; step++) {
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        int best_x = x, best_y = y;
        int best_val = dist_in[y * img_size.width + x];
        bool found = false;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx >= 0 && nx < img_size.width && ny >= 0 && ny < img_size.height) {
                int val = dist_in[ny * img_size.width + nx];
                if (val > best_val) {
                    best_val = val;
                    best_x = nx;
                    best_y = ny;
                    found = true;
                }
            }
        }
        if (!found) break;
        x = best_x;
        y = best_y;
    }
    end_out->x = (double)x;
    end_out->y = (double)y;
}

void wander_down (int* dist_in, size img_size, pt start, int min_limit, pt* end_out) {
    int x = (int)start.x;
    int y = (int)start.y;
    int curr_val = dist_in[y * img_size.width + x];
    if (curr_val <= min_limit) {
        end_out->x = (double)x;
        end_out->y = (double)y;
        return;
    }
    for (int step = 0; step < 1000; step++) {
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        int best_x = x, best_y = y;
        int best_val = 2147483647;
        bool found = false;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx >= 0 && nx < img_size.width && ny >= 0 && ny < img_size.height) {
                int val = dist_in[ny * img_size.width + nx];
                if (val >= min_limit && val < curr_val && val < best_val) {
                    best_val = val;
                    best_x = nx;
                    best_y = ny;
                    found = true;
                }
            }
        }
        if (!found) break;
        x = best_x;
        y = best_y;
        curr_val = dist_in[y * img_size.width + x];
        if (curr_val <= min_limit) break;
    }
    end_out->x = (double)x;
    end_out->y = (double)y;
}

bool wander_up_radius (int* dist_in, size img_size, pt start, int radius, pt* end_out) {
    int sx = (int)start.x;
    int sy = (int)start.y;
    int best_x = sx, best_y = sy;
    int best_val = dist_in[sy * img_size.width + sx];
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > radius * radius) continue;
            int nx = sx + dx;
            int ny = sy + dy;
            if (nx >= 0 && nx < img_size.width && ny >= 0 && ny < img_size.height) {
                int val = dist_in[ny * img_size.width + nx];
                if (val > best_val) {
                    best_val = val;
                    best_x = nx;
                    best_y = ny;
                }
            }
        }
    }
    end_out->x = (double)best_x;
    end_out->y = (double)best_y;
    return (best_x != sx || best_y != sy);
}

bool wander_down_radius (int* dist_in, size img_size, pt start, int radius, int min_limit, pt* end_out) {
    int sx = (int)start.x;
    int sy = (int)start.y;
    int start_val = dist_in[sy * img_size.width + sx];
    int best_x = sx, best_y = sy;
    int best_val = 2147483647;
    bool found = false;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy > radius * radius) continue;
            int nx = sx + dx;
            int ny = sy + dy;
            if (nx >= 0 && nx < img_size.width && ny >= 0 && ny < img_size.height) {
                int val = dist_in[ny * img_size.width + nx];
                if (val >= min_limit && val < best_val) {
                    best_val = val;
                    best_x = nx;
                    best_y = ny;
                    found = true;
                }
            }
        }
    }
    if (!found) {
        end_out->x = (double)sx;
        end_out->y = (double)sy;
        return false;
    }
    end_out->x = (double)best_x;
    end_out->y = (double)best_y;
    return (best_x != sx || best_y != sy);
}

}

//================================================================================================================================
//=> - End -
//================================================================================================================================
