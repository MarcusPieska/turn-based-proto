//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "continent_maker_pn.h"
#include "map_combiner.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    u32 seed = 0;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    ContinentMakerPnParams params_std;
    params_std.m_seed = seed;
    params_std.m_debug = false;
    ContinentMakerPn mk_std(params_std);
    if (!mk_std.is_valid()) {
        return 1;
    }
    ContinentMakerPnParams params_central;
    params_central.m_seed = seed;
    params_central.m_inner_grad_limit = 0.0f;
    params_central.m_debug = false;
    ContinentMakerPn mk_c(params_central);
    if (!mk_c.is_valid()) {
        return 2;
    }
    const u16 w = mk_std.width();
    const u16 h = mk_std.height();
    const u32 n3 = static_cast<u32>(w) * static_cast<u32>(h) * 3u;
    std::vector<u8> canvas(static_cast<size_t>(n3));
    std::memcpy(canvas.data(), mk_std.terrain_rgb(), static_cast<size_t>(n3));
    if (!MapCombiner::save_rgb_ppm("out_map_composed_std.ppm", mk_std.terrain_rgb(), w, h)) {
        return 3;
    }
    if (!MapCombiner::save_rgb_ppm("out_map_composed_central.ppm", mk_c.terrain_rgb(), mk_c.width(), mk_c.height())) {
        return 4;
    }
    const auto t0 = std::chrono::steady_clock::now();
    if (!MapCombiner::combine_terrain_into_base(canvas.data(), w, h, mk_c.terrain_rgb(), w, h, 0, 0)) {
        return 5;
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::printf("combine time: %.3f ms\n", ms);
    if (!MapCombiner::save_rgb_ppm("out_map_composed_terrain.ppm", canvas.data(), w, h)) {
        return 6;
    }
    std::printf(
        "wrote out_map_composed_std.ppm out_map_composed_central.ppm out_map_composed_terrain.ppm\n");
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
