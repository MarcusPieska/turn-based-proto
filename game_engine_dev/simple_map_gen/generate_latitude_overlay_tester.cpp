//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <time.h>

#include "game_primitives.h"
#include "generate_latitude_overlay.h"
#include "perlin_noise.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_out_path = "/home/w/Projects/simple-map-gen/latitude-overlay.ppm";
static const u16 g_w = 1000;
static const u16 g_h = 1000;

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_generate_latitude_overlay_basic () {
    const clock_t t_gen0 = clock();
    u8* pix = Generate_LatitudeOverlay::generate(g_w, g_h);
    const clock_t t_gen1 = clock();
    if (pix == nullptr) {
        std::printf("Generate_LatitudeOverlay failed to generate\n");
        return -1;
    }
    const clock_t t_save0 = clock();
    const bool ok = save_perlin_gray_pgm(g_out_path, pix, g_w, g_h);
    const clock_t t_save1 = clock();
    delete[] pix;
    const double gen_sec = static_cast<double>(t_gen1 - t_gen0) / static_cast<double>(CLOCKS_PER_SEC);
    const double save_sec = static_cast<double>(t_save1 - t_save0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("Generate_LatitudeOverlay generate time: %.6f s (%u x %u)\n", gen_sec, g_w, g_h);
    std::printf("Generate_LatitudeOverlay save time: %.6f s\n", save_sec);
    if (!ok) {
        std::printf("failed to save: %s\n", g_out_path);
        return -1;
    }
    std::printf("saved: %s\n", g_out_path);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main () {
    return test_generate_latitude_overlay_basic();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
