//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "map_loader.h"
#include "generator_constants.h"

typedef const char* cstr;

static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";

static cstr terr_name (u8 cls) {
    if (cls == TERR_NONE[0]) return "NONE";
    if (cls == TERR_OCEAN[0]) return "OCEAN";
    if (cls == TERR_SEA[0]) return "SEA";
    if (cls == TERR_COASTAL[0]) return "COASTAL";
    if (cls == TERR_PLAINS[0]) return "PLAINS";
    if (cls == TERR_HILLS[0]) return "HILLS";
    if (cls == TERR_MOUNTAINS[0]) return "MOUNTAINS";
    return "?";
}

void print_terrain_breakdown (const MapTerrainData& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* d = map.data();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 cnt[8] = {};
    for (u32 i = 0; i < n; ++i) {
        const u8 cls = d[i];
        if (cls < 8u) {
            cnt[cls] += 1u;
        }
    }
    std::printf("MAP: %s\n", g_map_path);
    std::printf("SIZE: %u x %u  (%u tiles)\n", w, h, n);
    std::printf("--- TERRAIN BREAKDOWN ---\n");
    for (u8 cls = 0; cls <= TERR_MOUNTAINS[0]; ++cls) {
        const u32 c = cnt[cls];
        const double pct = n > 0 ? (100.0 * static_cast<double>(c) / static_cast<double>(n)) : 0.0;
        std::printf(" %s(%u): %u  (%.2f%%)\n", terr_name(cls), cls, c, pct);
    }
}

int main (int argc, char* argv[]) {
    if (argc > 1) {
        g_map_path = argv[1];
    }
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(g_map_path, map)) {
        std::printf("*** FAILED to load %s\n", g_map_path);
        return 1;
    }
    print_terrain_breakdown(map);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
