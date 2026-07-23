//================================================================================================================================
//= - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "res_statics.h"
#include "resource_placement.h"
#include "resource_static_key.h"
#include "res_dist_static_key.h"
#include "game_map_defs.h"
#include "map_terrain_validate.h"

static const char* g_res_ov_ppm = "/home/w/Projects/simple-map-gen/p1-seed-101";
static const char* g_lib_path = "../data_io/runtime_static_loader_lib.so";
static const char* g_data_path = "../";
static u32 g_out_seed = 101;
static u8 g_dot_small = 0;
static const u32 g_base_n = 100u;

static const char* k_ansi_rst = "\033[0m";
static const char* k_ansi_red = "\033[91m";
static const char* k_ansi_grn = "\033[92m";
static const char* k_ansi_cyn = "\033[96m";

//================================================================================================================================
//= - Helpers -
//================================================================================================================================

static ResPlcVizPrm viz_prm () {
    return g_dot_small != 0 ? res_plc_viz_prm_small() : res_plc_viz_prm_def();
}

static cstr placed_ansi (u32 placed, u32 target) {
    if (placed < target) {
        return k_ansi_red;
    }
    if (placed > target) {
        return k_ansi_grn;
    }
    return k_ansi_cyn;
}

static bool join_path (cstr dir, cstr file, char* out, size_t cap) {
    if (dir == nullptr || file == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    const size_t n = std::strlen(dir);
    if (n > 0 && dir[n - 1] == '/') {
        return std::snprintf(out, cap, "%s%s", dir, file) > 0;
    }
    return std::snprintf(out, cap, "%s/%s", dir, file) > 0;
}

static bool skip_ws_cmt (FILE* fp) {
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

static u8* load_ppm_rgb (cstr path, u16* out_w, u16* out_h) {
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
    if (!skip_ws_cmt(fp)) {
        std::fclose(fp);
        return nullptr;
    }
    unsigned wi = 0;
    unsigned hi = 0;
    unsigned maxv = 0;
    if (std::fscanf(fp, "%u", &wi) != 1 || !skip_ws_cmt(fp) || std::fscanf(fp, "%u", &hi) != 1
        || !skip_ws_cmt(fp) || std::fscanf(fp, "%u", &maxv) != 1 || maxv != 255u) {
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

static u16* decode_res_ov (const u8* rgb, u32 n) {
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

static u8* decode_terr (const u8* rgb, u32 n) {
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

static u8* decode_clim (const u8* rgb, u32 n) {
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

//================================================================================================================================
//= - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    if (argc >= 2) {
        g_dot_small = static_cast<u8>(std::strtoul(argv[1], nullptr, 10));
    }
    ResStatics rs;
    if (!rs.load(g_lib_path, g_data_path)) {
        std::printf("failed to load statics\n");
        return -1;
    }
    char res_path[384];
    char terr_path[384];
    char clim_path[384];
    if (!join_path(g_res_ov_ppm, "resources.ppm", res_path, sizeof(res_path))
        || !join_path(g_res_ov_ppm, "terrain.ppm", terr_path, sizeof(terr_path))
        || !join_path(g_res_ov_ppm, "climate.ppm", clim_path, sizeof(clim_path))) {
        std::printf("failed to build input paths from %s\n", g_res_ov_ppm);
        rs.unload();
        return -1;
    }
    u16 w = 0;
    u16 h = 0;
    u8* res_rgb = load_ppm_rgb(res_path, &w, &h);
    if (res_rgb == nullptr) {
        std::printf("failed to load: %s\n", res_path);
        rs.unload();
        return -1;
    }
    u16 tw = 0;
    u16 th = 0;
    u8* terr_rgb = load_ppm_rgb(terr_path, &tw, &th);
    if (terr_rgb == nullptr || tw != w || th != h) {
        std::printf("failed to load terrain (or size mismatch): %s\n", terr_path);
        delete[] res_rgb;
        delete[] terr_rgb;
        rs.unload();
        return -1;
    }
    u16 cw = 0;
    u16 ch = 0;
    u8* clim_rgb = load_ppm_rgb(clim_path, &cw, &ch);
    if (clim_rgb == nullptr || cw != w || ch != h) {
        std::printf("failed to load climate (or size mismatch): %s\n", clim_path);
        delete[] res_rgb;
        delete[] terr_rgb;
        delete[] clim_rgb;
        rs.unload();
        return -1;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u16* res_ov = decode_res_ov(res_rgb, npx);
    u8* terr = decode_terr(terr_rgb, npx);
    u8* clim = decode_clim(clim_rgb, npx);
    delete[] res_rgb;
    delete[] terr_rgb;
    delete[] clim_rgb;
    u8* river = new u8[npx];
    u8* feat = new u8[npx];
    u8* poss = new u8[npx];
    u8* act = new u8[npx];
    if (res_ov == nullptr || terr == nullptr || clim == nullptr || river == nullptr || feat == nullptr
        || poss == nullptr || act == nullptr) {
        delete[] res_ov;
        delete[] terr;
        delete[] clim;
        delete[] river;
        delete[] feat;
        delete[] poss;
        delete[] act;
        rs.unload();
        return -1;
    }
    for (u32 i = 0; i < npx; ++i) {
        river[i] = 0;
        feat[i] = 0;
    }
    u32 land_n = 0;
    u32 water_n = 0;
    for (u32 i = 0; i < npx; ++i) {
        if (overlay_is_water_terr(terr[i])) {
            water_n = water_n + 1u;
        } else {
            land_n = land_n + 1u;
        }
    }
    std::printf("loaded dir: %s (%ux%u) land=%u water=%u\n",
        g_res_ov_ppm, (unsigned)w, (unsigned)h, land_n, water_n);
    ResPlcMapCtx ctx = {};
    ctx.m_w = w;
    ctx.m_h = h;
    ctx.m_terrain = terr;
    ctx.m_climate = clim;
    ctx.m_river = river;
    ctx.m_overlay = feat;
    const ResPlcVizPrm vprm = viz_prm();
    const RuntimeStatics& st = rs.s();
    const u16 res_n = st.resource().get_item_count();
    u32 img_n = 0;
    u32 tot_pl = 0;
    u32 tot_land = 0;
    u32 tot_water = 0;
    char fname[96];
    char out_path[384];
    const clock_t t0 = clock();
    for (u16 ri = 0; ri < res_n; ++ri) {
        const ResourceStaticDataStruct& r = st.resource().get_item(ResourceStaticDataKey::from_raw(ri));
        if (r.res_dist_idx >= st.res_dist().get_item_count()) {
            continue;
        }
        if (st.res_dist().get_item(ResDistStaticDataKey::from_raw(r.res_dist_idx)).has_plc == 0) {
            continue;
        }
        cstr rnm = st.resource().get_name(ResourceStaticDataKey::from_raw(ri));
        const u32 hit = ResPlcMatch::mark_all_rules(ctx, st, ri, poss, npx);
        const u32 target = g_base_n * static_cast<u32>(rs.res_plc(ri).m_res_wt);
        u32 placed = 0;
        u32 pl_land = 0;
        u32 pl_water = 0;
        for (u32 i = 0; i < npx; ++i) {
            if (res_ov[i] == ri) {
                act[i] = 1;
                placed = placed + 1u;
                if (overlay_is_water_terr(terr[i])) {
                    pl_water = pl_water + 1u;
                } else {
                    pl_land = pl_land + 1u;
                }
            } else {
                act[i] = 0;
            }
        }
        std::snprintf(fname, sizeof(fname), "%03u_%s.ppm", (unsigned)ri, rnm != nullptr ? rnm : "unknown");
        if (!ResPlcViz::make_out_path(g_out_seed, fname, out_path, sizeof(out_path))
            || !ResPlcViz::save_pair_ov_img(out_path, ctx, poss, act, npx, vprm)) {
            std::printf("overlay viz failed for %s\n", rnm != nullptr ? rnm : "unknown");
            delete[] res_ov;
            delete[] terr;
            delete[] clim;
            delete[] river;
            delete[] feat;
            delete[] poss;
            delete[] act;
            rs.unload();
            return -1;
        }
        const double land_pct = (land_n > 0u) ? (100.0 * static_cast<double>(pl_land) / static_cast<double>(land_n)) : 0.0;
        const double water_pct = (water_n > 0u) ? (100.0 * static_cast<double>(pl_water) / static_cast<double>(water_n)) : 0.0;
        std::printf("saved: %s (possible=%u %splaced=%u%s target=%u land=%u (%.3f%% of land) water=%u (%.3f%% of water))\n",
            out_path, hit, placed_ansi(placed, target), placed, k_ansi_rst, target,
            pl_land, land_pct, pl_water, water_pct);
        tot_pl = tot_pl + placed;
        tot_land = tot_land + pl_land;
        tot_water = tot_water + pl_water;
        img_n = img_n + 1u;
    }
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const double tot_land_pct = (land_n > 0u) ? (100.0 * static_cast<double>(tot_land) / static_cast<double>(land_n)) : 0.0;
    const double tot_water_pct = (water_n > 0u) ? (100.0 * static_cast<double>(tot_water) / static_cast<double>(water_n)) : 0.0;
    std::printf("resource_overlay_viz images: %u (%ux%u)\n", img_n, (unsigned)w, (unsigned)h);
    std::printf("totals: placed=%u land=%u (%.3f%% of land) water=%u (%.3f%% of water)\n",
        tot_pl, tot_land, tot_land_pct, tot_water, tot_water_pct);
    std::printf("resource_overlay_viz_tester done: %u resources in %.3f s (dot=%s)\n",
        img_n, sec, g_dot_small != 0 ? "1px" : "large");
    delete[] res_ov;
    delete[] terr;
    delete[] clim;
    delete[] river;
    delete[] feat;
    delete[] poss;
    delete[] act;
    rs.unload();
    return 0;
}

//================================================================================================================================
//= - End of file -
//================================================================================================================================
