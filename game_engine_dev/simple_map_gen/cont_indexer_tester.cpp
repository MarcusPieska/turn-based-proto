//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "cont_indexer.h"
#include "continent_maker_pn.h"
#include "water_land_overlay.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    ContinentMakerPnParams params;
    params.m_seed = 0;
    if (argc >= 2) {
        params.m_seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    params.m_debug = false;
    ContinentMakerPn mk(params);
    if (!mk.is_valid()) {
        return 1;
    }
    WaterLandOverlay wl(mk);
    if (!wl.is_valid()) {
        return 2;
    }
    const auto t0 = std::chrono::steady_clock::now();
    ContIndexer idx(wl);
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::printf("index map build time: %.3f ms\n", ms);
    if (!idx.is_valid()) {
        return 3;
    }
    std::printf("regions (continents + islands): %u\n", idx.region_count());
    idx.print_regions_by_area_desc();
    if (!idx.save_count_map_ppm("out_map_cont_index.ppm")) {
        return 4;
    }
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
