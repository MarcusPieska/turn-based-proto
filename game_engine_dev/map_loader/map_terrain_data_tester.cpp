//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "map_loader.h"

typedef const char* cstr;

static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";

static void base_name (cstr path, char* out, u16 out_sz) {
    if (path == nullptr || out == nullptr || out_sz == 0) {
        return;
    }
    cstr slash = path;
    for (cstr p = path; *p != '\0'; ++p) {
        if (*p == '/') {
            slash = p + 1;
        }
    }
    std::snprintf(out, out_sz, "%s", slash);
}

static void make_del_path (cstr src_path, char* out, u16 out_sz) {
    char base[256];
    base_name(src_path, base, sizeof(base));
    std::snprintf(out, out_sz, "DEL_%s", base);
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
    char out_path[320];
    make_del_path(g_map_path, out_path, sizeof(out_path));
    if (!map.save_terrain_ppm(out_path)) {
        std::printf("*** FAILED to save %s\n", out_path);
        return 1;
    }
    std::printf("LOADED: %s\n", g_map_path);
    std::printf("SIZE:   %u x %u\n", map.width(), map.height());
    std::printf("SAVED:  %s\n", out_path);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
