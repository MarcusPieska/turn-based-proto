//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "hlp_print_tile_owner.h"
#include "game_array_simple.h"
#include "hlp_civ_nm.h"
#include "hlp_parse_seed.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

static const u32 k_tiles_magic = 0x534c4954u;
static const u32 k_io_ver_min = 1u;

static const char* G_RT_LIB_A = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA_A = "../../";
static const char* G_RT_LIB_B = "../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA_B = "../";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool load_tiles (cstr path, GameTileSimple** out_tiles, u16* out_w, u16* out_h) {
    if (path == nullptr || out_tiles == nullptr || out_w == nullptr || out_h == nullptr) {
        return false;
    }
    *out_tiles = nullptr;
    *out_w = 0;
    *out_h = 0;
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 w = 0;
    u16 h = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&w, sizeof(w), 1, fp) != 1
        || std::fread(&h, sizeof(h), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (magic != k_tiles_magic || ver < k_io_ver_min || w == 0 || h == 0) {
        std::fclose(fp);
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    GameTileSimple* tiles = new GameTileSimple[n];
    if (std::fread(tiles, sizeof(GameTileSimple), n, fp) != n) {
        delete[] tiles;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    *out_tiles = tiles;
    *out_w = w;
    *out_h = h;
    return true;
}

static void count_own (const GameTileSimple* tiles, u32 n, u32* counts) {
    for (u32 i = 0; i < 256u; ++i) {
        counts[i] = 0;
    }
    for (u32 i = 0; i < n; ++i) {
        const u8 own = static_cast<u8>(tiles[i].m_civ_owner);
        counts[own] = counts[own] + 1u;
    }
}

static void print_own (const u32* counts, const RuntimeStatics& st) {
    for (u32 i = 0; i < 256u; ++i) {
        if (counts[i] == 0) {
            continue;
        }
        if (i == static_cast<u32>(U8_KEY_NULL)) {
            std::printf("owner=none tiles=%u\n", counts[i]);
        } else {
            std::printf("owner=%s tiles=%u\n", Helper_CivNm::nm(st, static_cast<u16>(i)), counts[i]);
        }
    }
}

//================================================================================================================================
//=> - Helper_PrintTileOwner -
//================================================================================================================================

bool Helper_PrintTileOwner::run (u32 turn) {
    if (!Helper_ParseSeed::load()) {
        std::printf("Helper_PrintTileOwner: parse_seed load failed\n");
        return false;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB_A, G_RT_DATA_A) && !loader.load(G_RT_LIB_B, G_RT_DATA_B)) {
        std::printf("Helper_PrintTileOwner: statics load failed\n");
        return false;
    }
    const RuntimeStatics& st = loader.statics();
    char path[384];
    if (!Helper_ParseSeed::map_path(turn, path, sizeof(path))) {
        return false;
    }
    GameTileSimple* tiles = nullptr;
    u16 w = 0;
    u16 h = 0;
    if (!load_tiles(path, &tiles, &w, &h)) {
        std::printf("Helper_PrintTileOwner: load failed: %s\n", path);
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 counts[256];
    count_own(tiles, n, counts);
    std::printf("seed=%u players=%u turn=%04u path=%s\n", Helper_ParseSeed::seed(), Helper_ParseSeed::players(), turn, path);
    print_own(counts, st);
    delete[] tiles;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
