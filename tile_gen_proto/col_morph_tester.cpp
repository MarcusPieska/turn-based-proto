//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <chrono>
#include "col_morph.cpp"

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

static void setup_test_image(u8* img, int w, int h, int channels) {
    for (int i = 0; i < w * h * channels; i += channels) {
        if (channels >= 1) img[i + 0] = 255; // R
        if (channels >= 2) img[i + 1] = 0;   // G
        if (channels >= 3) img[i + 2] = 255; // B
        if (channels >= 4) img[i + 3] = 255; // A
    }
    int tile_h = h / 2;
    for (int y = tile_h; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * channels;
            int val = ((x + y) % 256);
            if (channels >= 1) img[idx + 0] = val;
            if (channels >= 2) img[idx + 1] = val;
            if (channels >= 3) img[idx + 2] = val;
            if (channels >= 4) img[idx + 3] = 255;
        }
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main() {
    const int tile_w = 101;
    const int tile_h = 51;
    const int morph_h = tile_h * 2;
    const int channels = 3;
    u8* src_img = new u8[morph_h * tile_w * channels];
    u8* dst_img = new u8[morph_h * tile_w * channels];
    setup_test_image (src_img, tile_w, morph_h, channels);
    int half_w = tile_w / 2;
    int half_h = tile_h / 2;
    int morph_center_y = tile_h + half_h;
    
    pt top_pt = {tile_w / 2, morph_center_y - half_h};
    pt right_pt = {tile_w / 2 + half_w, morph_center_y};
    pt bottom_pt = {tile_w / 2, morph_center_y + half_h};
    pt left_pt = {tile_w / 2 - half_w, morph_center_y};
    
    std::srand(std::time(nullptr));
    const int num_iterations = 10000;
    printf("Running %d morphing operations...\n", num_iterations);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_iterations; i++) {
        int top_dy = std::rand() % 40 - 20;  
        int right_dy = std::rand() % 20 - 10; 
        int left_dy = std::rand() % 20 - 10; 
        int bottom_dy = 0;
        deltas d = {top_dy, right_dy, bottom_dy, left_dy};
        morph_tile(src_img, tile_w, morph_h, channels, top_pt, right_pt, bottom_pt, left_pt, d, dst_img);
    }
    
    auto stop_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time);
    
    double elapsed_seconds = elapsed.count() / 1000000.0;
    double time_per_tile = elapsed_seconds / num_iterations;
    
    printf("*** Total time taken for %d morphing operations: %.4f seconds\n", num_iterations, elapsed_seconds);
    printf("    Time per tile: %.4f ms\n", time_per_tile * 1000.0);
    printf("    Tiles per second: %.2f\n", 1.0 / time_per_tile);
    
    delete[] src_img;
    delete[] dst_img;
    
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
