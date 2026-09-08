//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "continent_size_indexer.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/lucky-seat";
static const u32 G_SEED = 101u;

static char g_terr[320];

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool build_paths () {
    char dir[256];
    if (std::snprintf(dir, sizeof(dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    return std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", dir) > 0;
}

static bool ensure_out_dir () {
    return ::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static bool save_rgb (cstr path, const Whiteboard_4B& wb) {
    if (path == nullptr || !wb.ok()) {
        return false;
    }
    const u16 w = wb.w();
    const u16 h = wb.h();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32* px = wb.get_iter_ptr();
    if (px == nullptr) {
        return false;
    }
    std::vector<u8> rgb(static_cast<size_t>(n) * 3u);
    for (u32 i = 0; i < n; ++i) {
        const u32 v = px[i];
        rgb[i * 3u + 0u] = static_cast<u8>((v >> 16) & 0xffu);
        rgb[i * 3u + 1u] = static_cast<u8>((v >> 8) & 0xffu);
        rgb[i * 3u + 2u] = static_cast<u8>(v & 0xffu);
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const bool ok = std::fwrite(rgb.data(), 1, rgb.size(), fp) == rgb.size();
    std::fclose(fp);
    return ok;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths() || !ensure_out_dir()) {
        std::printf("*** FAILED path/out\n");
        return 1;
    }
    MapTerrainData terr;
    if (!MapLoader::load_terrain_ppm(g_terr, terr)) {
        std::printf("*** FAILED load %s\n", g_terr);
        return 1;
    }
    const u16 w = terr.width();
    const u16 h = terr.height();
    const u8* cls = terr.data();
    if (cls == nullptr || w == 0u || h == 0u) {
        std::printf("*** FAILED empty terrain\n");
        return 1;
    }
    WhiteboardMng::init(w, h);
    Whiteboard_4B wb_rgb("continent_size_indexer_tester", "rgb", 0u);
    Whiteboard_2B wb_idx("continent_size_indexer_tester", "idx", 0u);
    if (!wb_rgb.ok() || !wb_idx.ok()) {
        std::printf("*** FAILED whiteboard\n");
        WhiteboardMng::terminate();
        return 1;
    }
    ContSizeList list = {};
    if (!ContinentSizeIndexer::index(cls, w, h, &list, wb_rgb, wb_idx)) {
        std::printf("*** FAILED ContinentSizeIndexer::index\n");
        WhiteboardMng::terminate();
        return 1;
    }
    char out_path[384];
    if (std::snprintf(out_path, sizeof(out_path), "%s/continent_size.ppm", G_OUT_DIR) <= 0) {
        WhiteboardMng::terminate();
        return 1;
    }
    if (!save_rgb(out_path, wb_rgb)) {
        std::printf("*** FAILED save %s\n", out_path);
        WhiteboardMng::terminate();
        return 1;
    }
    std::printf("top=%u\n", static_cast<unsigned>(list.m_n));
    for (u16 i = 0; i < list.m_n; ++i) {
        std::printf("  #%u id=%u tiles=%u\n",
            static_cast<unsigned>(i + 1u),
            static_cast<unsigned>(list.m_e[i].m_id),
            static_cast<unsigned>(list.m_e[i].m_tiles));
    }
    std::printf("wrote %s\n", out_path);
    WhiteboardMng::terminate();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
