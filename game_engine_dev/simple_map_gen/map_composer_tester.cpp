//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "continent_maker_pn.h"
#include "map_combiner.h"
#include "map_composer.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    u32 seed_base = 0;
    u32 seed_overlay = 0;
    if (argc >= 2) {
        seed_base = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    if (argc >= 3) {
        seed_overlay = static_cast<u32>(std::strtoul(argv[2], nullptr, 10));
    } else {
        seed_overlay = seed_base;
    }
    ComposeCentralWithArchipelagoArgs ca;
    ca.m_base_params.m_seed = seed_base;
    ca.m_base_params.m_debug = false;
    ca.m_base_params.m_inner_grad_limit = 0.0f;
    ca.m_overlay_params.m_seed = seed_overlay;
    ca.m_overlay_params.m_debug = false;
    const u16 w = ca.m_base_params.m_width;
    const u16 h = ca.m_base_params.m_height;
    const u16 ow = ca.m_overlay_params.m_width;
    const u16 oh = ca.m_overlay_params.m_height;
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<u8> out_rgb;
    std::vector<u8> base_rgb;
    std::vector<u8> overlay_rgb;
    if (!MapComposer::compose_central_with_archipelago(ca, out_rgb, &base_rgb, &overlay_rgb)) {
        return 1;
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::printf("compose_central_with_archipelago: %.3f ms\n", ms);
    if (!MapCombiner::save_rgb_ppm("out_map_composed_central.ppm", base_rgb.data(), w, h)) {
        return 2;
    }
    if (!MapCombiner::save_rgb_ppm("out_map_composed_std.ppm", overlay_rgb.data(), ow, oh)) {
        return 3;
    }
    if (!MapCombiner::save_rgb_ppm("out_map_composed_terrain.ppm", out_rgb.data(), w, h)) {
        return 4;
    }
    std::printf(
        "wrote out_map_composed_central.ppm out_map_composed_std.ppm out_map_composed_terrain.ppm\n");
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
