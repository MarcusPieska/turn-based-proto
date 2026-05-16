//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>

#include "perlin_noise.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    PerlinImgParams params;
    u32 seed = 0;
    const size_t nbytes =
        static_cast<size_t>(params.m_w) * static_cast<size_t>(params.m_h);
    u8* pix = new u8[nbytes];
    f32 freq = 0.5f;
    const f32 freq_step_mul = 1.62f;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 7; ++i) {
        params.m_frequency = freq;
        char path[88];
        std::snprintf(path, sizeof(path), "out_perlin_%02d.pgm", i + 1);
        bool ok = render_perlin_gray_u8(pix, params, seed);
        if (!ok) {
            std::printf("render_perlin_gray_u8 failed step %d\n", i + 1);
            delete[] pix;
            return 1;
        }
        ok = save_perlin_gray_pgm(path, pix, params.m_w, params.m_h);
        if (!ok) {
            std::printf("save_perlin_gray_pgm failed %s\n", path);
            delete[] pix;
            return 2;
        }
        std::printf("wrote %s freq=%g lac=%g\n", path, (f64)params.m_frequency, (f64)params.m_lacunarity);
        freq *= freq_step_mul;
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::printf("map build time: %.3f ms\n", ms);
    delete[] pix;
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
