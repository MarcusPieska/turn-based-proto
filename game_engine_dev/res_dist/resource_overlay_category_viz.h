//================================================================================================================================
//= - Include guards -
//================================================================================================================================

#ifndef RESOURCE_OVERLAY_CATEGORY_VIZ_H
#define RESOURCE_OVERLAY_CATEGORY_VIZ_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "res_statics.h"
#include "resource_placement.h"
#include "resource_static_key.h"
#include "res_dist_static_key.h"
#include "res_type_static_key.h"
#include "game_map_defs.h"
#include "map_terrain_validate.h"

//================================================================================================================================
//= - Shared category overlay viz helpers -
//================================================================================================================================

static const char* g_cat_viz_dir = "/home/w/Projects/simple-map-gen/p1-seed-101";
static const char* g_cat_viz_lib = "../data_io/runtime_static_loader_lib.so";
static const char* g_cat_viz_data = "../";
static u32 g_cat_viz_seed = 101;
static u8 g_cat_viz_dot_small = 0;

static const u8 k_cat_viz_mark_cap = 16;

static ResPlcVizPrm cat_viz_prm () {
    return g_cat_viz_dot_small != 0 ? res_plc_viz_prm_small() : res_plc_viz_prm_def();
}

static bool cat_viz_join (cstr dir, cstr file, char* out, size_t cap) {
    if (dir == nullptr || file == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    const size_t n = std::strlen(dir);
    if (n > 0 && dir[n - 1] == '/') {
        return std::snprintf(out, cap, "%s%s", dir, file) > 0;
    }
    return std::snprintf(out, cap, "%s/%s", dir, file) > 0;
}

static bool cat_viz_skip_ws_cmt (FILE* fp) {
    for (;;) {
        const int c = std::fgetc(fp);
        if (c == EOF) {
            return false;
        }
        if (c == '#') {
            for (;;) {
                const int d = std::fgetc(fp);
                if (d == EOF) {
                    return false;
                }
                if (d == '\n') {
                    break;
                }
            }
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            continue;
        }
        std::ungetc(c, fp);
        return true;
    }
}

static u8* cat_viz_load_ppm (cstr path, u16* out_w, u16* out_h) {
    if (path == nullptr || out_w == nullptr || out_h == nullptr) {
        return nullptr;
    }
    FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return nullptr;
    }
    char mag[3] = {};
    if (std::fread(mag, 1, 2, fp) != 2 || mag[0] != 'P' || mag[1] != '6') {
        std::fclose(fp);
        return nullptr;
    }
    if (!cat_viz_skip_ws_cmt(fp)) {
        std::fclose(fp);
        return nullptr;
    }
    unsigned wi = 0;
    unsigned hi = 0;
    unsigned maxv = 0;
    if (std::fscanf(fp, "%u", &wi) != 1 || !cat_viz_skip_ws_cmt(fp) || std::fscanf(fp, "%u", &hi) != 1
        || !cat_viz_skip_ws_cmt(fp) || std::fscanf(fp, "%u", &maxv) != 1 || maxv != 255u) {
        std::fclose(fp);
        return nullptr;
    }
    const int nl = std::fgetc(fp);
    if (nl != '\n' && nl != ' ' && nl != '\t' && nl != '\r') {
        std::fclose(fp);
        return nullptr;
    }
    if (wi == 0 || hi == 0 || wi > 65535u || hi > 65535u) {
        std::fclose(fp);
        return nullptr;
    }
    const u32 n = wi * hi;
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        std::fclose(fp);
        return nullptr;
    }
    if (std::fread(rgb, 1, static_cast<size_t>(n) * 3u, fp) != static_cast<size_t>(n) * 3u) {
        delete[] rgb;
        std::fclose(fp);
        return nullptr;
    }
    std::fclose(fp);
    *out_w = static_cast<u16>(wi);
    *out_h = static_cast<u16>(hi);
    return rgb;
}

static u16* cat_viz_decode_res (const u8* rgb, u32 n) {
    u16* ov = new u16[n];
    if (ov == nullptr || rgb == nullptr) {
        delete[] ov;
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        const u8 r = rgb[i * 3u + 0];
        const u8 g = rgb[i * 3u + 1];
        const u8 b = rgb[i * 3u + 2];
        if (r == g && g == b) {
            ov[i] = static_cast<u16>(r);
        } else {
            ov[i] = U16_KEY_NULL;
        }
    }
    return ov;
}

static u8* cat_viz_decode_terr (const u8* rgb, u32 n) {
    u8* terr = new u8[n];
    if (terr == nullptr || rgb == nullptr) {
        delete[] terr;
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        bool ok = false;
        terr[i] = MapTerrainValidate::class_from_rgb(rgb[i * 3u], rgb[i * 3u + 1], rgb[i * 3u + 2], &ok);
        if (!ok) {
            terr[i] = TERR_PLAINS[0];
        }
    }
    return terr;
}

static u8* cat_viz_decode_clim (const u8* rgb, u32 n) {
    u8* clim = new u8[n];
    if (clim == nullptr || rgb == nullptr) {
        delete[] clim;
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        bool ok = false;
        clim[i] = climate_from_rgb(rgb[i * 3u], rgb[i * 3u + 1], rgb[i * 3u + 2], &ok);
        if (!ok) {
            clim[i] = CLIMATE_NONE;
        }
    }
    return clim;
}

static u16 cat_viz_find_type (const RuntimeStatics& s, cstr type_nm) {
    const ResTypeStaticData& rt = s.res_type();
    const u16 n = rt.get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = rt.get_name(ResTypeStaticDataKey::from_raw(i));
        if (nm != nullptr && type_nm != nullptr && std::strcmp(nm, type_nm) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static i32 cat_viz_run (cstr type_nm, cstr out_fname) {
    ResStatics rs;
    if (!rs.load(g_cat_viz_lib, g_cat_viz_data)) {
        std::printf("failed to load statics\n");
        return -1;
    }
    char res_path[384];
    char terr_path[384];
    char clim_path[384];
    if (!cat_viz_join(g_cat_viz_dir, "resources.ppm", res_path, sizeof(res_path))
        || !cat_viz_join(g_cat_viz_dir, "terrain.ppm", terr_path, sizeof(terr_path))
        || !cat_viz_join(g_cat_viz_dir, "climate.ppm", clim_path, sizeof(clim_path))) {
        std::printf("failed to build input paths from %s\n", g_cat_viz_dir);
        rs.unload();
        return -1;
    }
    u16 w = 0;
    u16 h = 0;
    u8* res_rgb = cat_viz_load_ppm(res_path, &w, &h);
    if (res_rgb == nullptr) {
        std::printf("failed to load: %s\n", res_path);
        rs.unload();
        return -1;
    }
    u16 tw = 0;
    u16 th = 0;
    u8* terr_rgb = cat_viz_load_ppm(terr_path, &tw, &th);
    if (terr_rgb == nullptr || tw != w || th != h) {
        std::printf("failed to load terrain (or size mismatch): %s\n", terr_path);
        delete[] res_rgb;
        delete[] terr_rgb;
        rs.unload();
        return -1;
    }
    u16 cw = 0;
    u16 ch = 0;
    u8* clim_rgb = cat_viz_load_ppm(clim_path, &cw, &ch);
    if (clim_rgb == nullptr || cw != w || ch != h) {
        std::printf("failed to load climate (or size mismatch): %s\n", clim_path);
        delete[] res_rgb;
        delete[] terr_rgb;
        delete[] clim_rgb;
        rs.unload();
        return -1;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u16* res_ov = cat_viz_decode_res(res_rgb, npx);
    u8* terr = cat_viz_decode_terr(terr_rgb, npx);
    u8* clim = cat_viz_decode_clim(clim_rgb, npx);
    delete[] res_rgb;
    delete[] terr_rgb;
    delete[] clim_rgb;
    u8* river = new u8[npx];
    u8* feat = new u8[npx];
    u8* marks = new u8[npx];
    if (res_ov == nullptr || terr == nullptr || clim == nullptr || river == nullptr || feat == nullptr
        || marks == nullptr) {
        delete[] res_ov;
        delete[] terr;
        delete[] clim;
        delete[] river;
        delete[] feat;
        delete[] marks;
        rs.unload();
        return -1;
    }
    for (u32 i = 0; i < npx; ++i) {
        river[i] = 0;
        feat[i] = 0;
        marks[i] = 0;
    }
    const RuntimeStatics& st = rs.s();
    const u16 ty = cat_viz_find_type(st, type_nm);
    if (ty == U16_KEY_NULL) {
        std::printf("%s res_type not found\n", type_nm != nullptr ? type_nm : "?");
        delete[] res_ov;
        delete[] terr;
        delete[] clim;
        delete[] river;
        delete[] feat;
        delete[] marks;
        rs.unload();
        return -1;
    }
    u16 ids[k_cat_viz_mark_cap];
    u32 cnts[k_cat_viz_mark_cap];
    u8 id_n = 0;
    const u16 res_n = st.resource().get_item_count();
    const ResourceStaticData& rs_data = st.resource();
    for (u16 i = 0; i < res_n; ++i) {
        const ResourceStaticDataStruct& it = rs_data.get_item(ResourceStaticDataKey::from_raw(i));
        if (it.type != ty) {
            continue;
        }
        if (id_n >= k_cat_viz_mark_cap) {
            std::printf("too many %s resources for color table\n", type_nm);
            break;
        }
        ids[id_n] = i;
        cnts[id_n] = 0;
        id_n = static_cast<u8>(id_n + 1u);
    }
    if (id_n == 0) {
        std::printf("no %s resources found\n", type_nm);
        delete[] res_ov;
        delete[] terr;
        delete[] clim;
        delete[] river;
        delete[] feat;
        delete[] marks;
        rs.unload();
        return -1;
    }
    u32 placed = 0;
    for (u32 i = 0; i < npx; ++i) {
        const u16 ri = res_ov[i];
        if (ri == U16_KEY_NULL) {
            continue;
        }
        for (u8 k = 0; k < id_n; ++k) {
            if (ids[k] != ri) {
                continue;
            }
            marks[i] = static_cast<u8>(k + 1u);
            cnts[k] = cnts[k] + 1u;
            placed = placed + 1u;
            break;
        }
    }
    ResPlcMapCtx ctx = {};
    ctx.m_w = w;
    ctx.m_h = h;
    ctx.m_terrain = terr;
    ctx.m_climate = clim;
    ctx.m_river = river;
    ctx.m_overlay = feat;
    char out_path[384];
    const clock_t t0 = clock();
    if (!ResPlcViz::make_out_path(g_cat_viz_seed, out_fname, out_path, sizeof(out_path))
        || !ResPlcViz::save_pair_img(out_path, ctx, marks, npx, cat_viz_prm())) {
        std::printf("failed to save %s viz\n", type_nm);
        delete[] res_ov;
        delete[] terr;
        delete[] clim;
        delete[] river;
        delete[] feat;
        delete[] marks;
        rs.unload();
        return -1;
    }
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("loaded dir: %s (%ux%u)\n", g_cat_viz_dir, (unsigned)w, (unsigned)h);
    for (u8 k = 0; k < id_n; ++k) {
        cstr nm = rs_data.get_name(ResourceStaticDataKey::from_raw(ids[k]));
        u8 rr = 0;
        u8 gg = 0;
        u8 bb = 0;
        ResPlcViz::rule_rgb(static_cast<u8>(k + 1u), &rr, &gg, &bb);
        std::printf("  mark %u rgb=(%u,%u,%u) %s idx=%u placed=%u\n",
            (unsigned)(k + 1u), (unsigned)rr, (unsigned)gg, (unsigned)bb,
            nm != nullptr ? nm : "unknown", (unsigned)ids[k], cnts[k]);
    }
    std::printf("saved: %s (type=%s resources=%u placed=%u)\n", out_path, type_nm, (unsigned)id_n, placed);
    std::printf("resource_overlay_%s_viz done in %.3f s (dot=%s)\n",
        type_nm, sec, g_cat_viz_dot_small != 0 ? "1px" : "large");
    delete[] res_ov;
    delete[] terr;
    delete[] clim;
    delete[] river;
    delete[] feat;
    delete[] marks;
    rs.unload();
    return 0;
}

#endif // RESOURCE_OVERLAY_CATEGORY_VIZ_H

//================================================================================================================================
//= - End of file -
//================================================================================================================================
