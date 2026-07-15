//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "factory_game_array_simple.h"

#include "game_map_defs.h"
#include "map_loader.h"
#include "map_terrain_data.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - PPM helpers -
//================================================================================================================================

static bool rd_ppm_rgb (cstr path, u16* out_w, u16* out_h, u8** out_rgb) {
    if (path == nullptr || out_w == nullptr || out_h == nullptr || out_rgb == nullptr) {
        return false;
    }
    *out_w = 0;
    *out_h = 0;
    *out_rgb = nullptr;
    FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    char magic[3] = {};
    if (std::fscanf(fp, "%2s", magic) != 1 || magic[0] != 'P' || magic[1] != '6') {
        std::fclose(fp);
        return false;
    }
    int c = std::fgetc(fp);
    while (c == '#') {
        while (c != '\n' && c != EOF) {
            c = std::fgetc(fp);
        }
        c = std::fgetc(fp);
    }
    ungetc(c, fp);
    unsigned wi = 0;
    unsigned hi = 0;
    unsigned maxv = 0;
    if (std::fscanf(fp, "%u %u %u", &wi, &hi, &maxv) != 3 || maxv != 255u) {
        std::fclose(fp);
        return false;
    }
    c = std::fgetc(fp);
    if (c == EOF) {
        std::fclose(fp);
        return false;
    }
    if (wi == 0 || hi == 0 || wi > 65535u || hi > 65535u) {
        std::fclose(fp);
        return false;
    }
    const u16 w = static_cast<u16>(wi);
    const u16 h = static_cast<u16>(hi);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    u8* rgb = new u8[nbytes];
    if (std::fread(rgb, 1, nbytes, fp) != nbytes) {
        delete[] rgb;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    *out_w = w;
    *out_h = h;
    *out_rgb = rgb;
    return true;
}

static bool load_clim_ppm (cstr path, u16 ew, u16 eh, u8** out_cls) {
    u16 w = 0;
    u16 h = 0;
    u8* rgb = nullptr;
    if (!rd_ppm_rgb(path, &w, &h, &rgb)) {
        return false;
    }
    if (w != ew || h != eh) {
        delete[] rgb;
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* cls = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        const u8* px = rgb + i * 3u;
        bool matched = false;
        cls[i] = climate_from_rgb(px[0], px[1], px[2], &matched);
        if (!matched) {
            delete[] cls;
            delete[] rgb;
            return false;
        }
    }
    delete[] rgb;
    *out_cls = cls;
    return true;
}

static bool load_riv_ppm (cstr path, u16 ew, u16 eh, u8** out_riv) {
    u16 w = 0;
    u16 h = 0;
    u8* rgb = nullptr;
    if (!rd_ppm_rgb(path, &w, &h, &rgb)) {
        return false;
    }
    if (w != ew || h != eh) {
        delete[] rgb;
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* riv = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        const u8* px = rgb + i * 3u;
        riv[i] = (px[0] != 0 || px[1] != 0 || px[2] != 0) ? 1 : 0;
    }
    delete[] rgb;
    *out_riv = riv;
    return true;
}

static bool is_res_bg_px (u8 r, u8 g, u8 b) {
    if (r == 80 && g == 160 && b == 80) {
        return true;
    }
    if (r == 64 && g == 128 && b == 192) {
        return true;
    }
    return false;
}

static u16 decode_res_px (u8 r, u8 g, u8 b) {
    if (is_res_bg_px(r, g, b)) {
        return U16_KEY_NULL;
    }
    if (r != g || g != b) {
        return U16_KEY_NULL;
    }
    return static_cast<u16>(r);
}

static bool load_res_ppm (cstr path, u16 ew, u16 eh, u16** out_res) {
    if (path == nullptr) {
        return false;
    }
    u16 w = 0;
    u16 h = 0;
    u8* rgb = nullptr;
    if (!rd_ppm_rgb(path, &w, &h, &rgb)) {
        return false;
    }
    if (w != ew || h != eh) {
        delete[] rgb;
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* res = new u16[n];
    for (u32 i = 0; i < n; ++i) {
        const u8* px = rgb + i * 3u;
        res[i] = decode_res_px(px[0], px[1], px[2]);
    }
    delete[] rgb;
    *out_res = res;
    return true;
}

static bool load_ov_ppm (cstr path, u16 ew, u16 eh, u8** out_ov) {
    u16 w = 0;
    u16 h = 0;
    u8* rgb = nullptr;
    if (!rd_ppm_rgb(path, &w, &h, &rgb)) {
        return false;
    }
    if (w != ew || h != eh) {
        delete[] rgb;
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* ov = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        const u8* px = rgb + i * 3u;
        bool matched = false;
        ov[i] = overlay_class_from_rgb(px[0], px[1], px[2], &matched);
        if (!matched) {
            delete[] ov;
            delete[] rgb;
            return false;
        }
    }
    delete[] rgb;
    *out_ov = ov;
    return true;
}

//================================================================================================================================
//=> - Factory_GameArraySimple -
//================================================================================================================================

bool Factory_GameArraySimple::load_map_gen_data (
    GameArraySimple* out,
    cstr terr_path,
    cstr clim_path,
    cstr riv_path,
    cstr ov_path) 
{
    if (out == nullptr) {
        return false;
    }
    out->clear();
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(terr_path, map)) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terr = map.data();
    if (terr == nullptr || w == 0 || h == 0) {
        return false;
    }
    u8* clim = nullptr;
    u8* riv = nullptr;
    if (!load_clim_ppm(clim_path, w, h, &clim) || !load_riv_ppm(riv_path, w, h, &riv)) {
        delete[] riv;
        delete[] clim;
        return false;
    }
    u8* ov = nullptr;
    if (ov_path != nullptr && !load_ov_ppm(ov_path, w, h, &ov)) {
        delete[] riv;
        delete[] clim;
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    GameTileSimple* tiles = new GameTileSimple[n];
    for (u32 i = 0; i < n; ++i) {
        GameTileSimple* t = &tiles[i];
        t->m_unit_hd = U16_KEY_NULL;
        t->m_add_idx = U16_KEY_NULL;
        t->m_res = U16_KEY_NULL;
        t->m_terr = terr[i];
        t->m_clim = clim[i];
        t->m_ov = ov != nullptr ? ov[i] : OVERLAY_NONE;
        t->m_riv = riv[i];
        t->m_add_typ = 0;
    }
    delete[] ov;
    delete[] riv;
    delete[] clim;
    out->m_w = w;
    out->m_h = h;
    out->m_tiles = tiles;
    return true;
}

bool Factory_GameArraySimple::load_res_dist_data (GameArraySimple* out, cstr res_path) {
    if (out == nullptr || res_path == nullptr) {
        return false;
    }
    const u16 w = out->m_w;
    const u16 h = out->m_h;
    if (out->m_tiles == nullptr || w == 0 || h == 0) {
        return false;
    }
    u16* res = nullptr;
    if (!load_res_ppm(res_path, w, h, &res)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        out->m_tiles[i].m_res = res[i];
    }
    delete[] res;
    return true;
}

bool Factory_GameArraySimple::load_from_rslt (GameArraySimple* out, const MakeMapRslt& rslt) {
    if (out == nullptr || !rslt.m_ok || rslt.m_terrain == nullptr || rslt.m_climate == nullptr
        || rslt.m_rivers == nullptr || rslt.m_overlay == nullptr || rslt.m_resources == nullptr) {
        return false;
    }
    const u16 w = rslt.m_w;
    const u16 h = rslt.m_h;
    if (w == 0 || h == 0) {
        return false;
    }
    out->clear();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    GameTileSimple* tiles = new GameTileSimple[n];
    for (u32 i = 0; i < n; ++i) {
        GameTileSimple* t = &tiles[i];
        t->m_unit_hd = U16_KEY_NULL;
        t->m_add_idx = U16_KEY_NULL;
        t->m_res = rslt.m_resources[i];
        t->m_terr = rslt.m_terrain[i];
        t->m_clim = rslt.m_climate[i];
        t->m_ov = rslt.m_overlay[i];
        t->m_riv = rslt.m_rivers[i];
        t->m_add_typ = 0;
    }
    out->m_w = w;
    out->m_h = h;
    out->m_tiles = tiles;
    return true;
}

bool Factory_GameArraySimple::init_test_grid (GameArraySimple* out, u16 w, u16 h) {
    if (out == nullptr || w == 0 || h == 0) {
        return false;
    }
    out->clear();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    GameTileSimple* tiles = new GameTileSimple[n];
    for (u32 i = 0; i < n; ++i) {
        tiles[i] = {};
        tiles[i].m_unit_hd = U16_KEY_NULL;
        tiles[i].m_add_idx = U16_KEY_NULL;
        tiles[i].m_res = UINT16_MAX;
    }
    out->m_w = w;
    out->m_h = h;
    out->m_tiles = tiles;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
