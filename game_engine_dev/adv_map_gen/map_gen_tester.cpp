//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "map_config.h"
#include "map_gen_loader.h"
#include "map_terrain_data.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool copy_owned (u8** dst, const u8* src, u32 n) {
    if (dst == nullptr || src == nullptr || n == 0u) {
        return false;
    }
    *dst = new u8[n];
    if (*dst == nullptr) {
        return false;
    }
    std::memcpy(*dst, src, static_cast<size_t>(n));
    return true;
}

static bool copy_owned_u16 (u16** dst, const u16* src, u32 n) {
    if (dst == nullptr || src == nullptr || n == 0u) {
        return false;
    }
    *dst = new u16[n];
    if (*dst == nullptr) {
        return false;
    }
    std::memcpy(*dst, src, static_cast<size_t>(n) * sizeof(u16));
    return true;
}

static bool save_owned_copy (const MakeMapRslt& r, const char* terr_path) {
    if (!r.m_ok || r.m_terrain == nullptr) {
        return false;
    }
    MapTerrainData map;
    if (!map.assign_copy(r.m_w, r.m_h, r.m_terrain)) {
        return false;
    }
    return map.save_terrain_ppm(terr_path);
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    u32 seed = 101u;
    u16 w = 1000u;
    u16 h = 1000u;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    if (argc >= 4) {
        w = static_cast<u16>(std::strtoul(argv[2], nullptr, 10));
        h = static_cast<u16>(std::strtoul(argv[3], nullptr, 10));
    }
    MapGenLoader loader;
    if (!loader.load("./map_gen.so")) {
        std::fprintf(stderr, "map_gen_tester: failed to load map_gen.so\n");
        return 1;
    }
    MapGenReq req = {};
    req.m_seed = seed;
    req.m_type = MAP_CONTINENTAL;
    req.m_w = w;
    req.m_h = h;
    req.m_cfg = map_config_def();
    req.m_statics = nullptr;
    MakeMapRslt so_rslt = loader.generate(req);
    if (!so_rslt.m_ok) {
        std::fprintf(stderr, "map_gen_tester: generate failed\n");
        loader.unload();
        return 1;
    }
    const u32 npx = static_cast<u32>(so_rslt.m_w) * static_cast<u32>(so_rslt.m_h);
    MakeMapRslt owned = {};
    owned.m_ok = false;
    if (!copy_owned(&owned.m_terrain, so_rslt.m_terrain, npx)
        || !copy_owned(&owned.m_climate, so_rslt.m_climate, npx)
        || !copy_owned(&owned.m_rivers, so_rslt.m_rivers, npx)
        || !copy_owned(&owned.m_overlay, so_rslt.m_overlay, npx)
        || !copy_owned_u16(&owned.m_resources, so_rslt.m_resources, npx)) {
        loader.free_rslt(&so_rslt);
        loader.unload();
        std::fprintf(stderr, "map_gen_tester: copy failed\n");
        return 1;
    }
    owned.m_ok = true;
    owned.m_w = so_rslt.m_w;
    owned.m_h = so_rslt.m_h;
    loader.free_rslt(&so_rslt);
    loader.unload();
    char path[256];
    std::snprintf(path, sizeof(path), "/tmp/map_gen_tester_%u_terrain.ppm", seed);
    if (!save_owned_copy(owned, path)) {
        std::fprintf(stderr, "map_gen_tester: save failed\n");
        return 1;
    }
    std::printf("map_gen_tester ok seed=%u size=%ux%u terrain=%s\n", seed, owned.m_w, owned.m_h, path);
    delete[] owned.m_terrain;
    delete[] owned.m_climate;
    delete[] owned.m_rivers;
    delete[] owned.m_overlay;
    delete[] owned.m_resources;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================