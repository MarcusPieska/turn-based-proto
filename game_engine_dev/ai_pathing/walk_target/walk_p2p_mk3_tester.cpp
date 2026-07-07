//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "generate_distance_p2p_mk3.h"
#include "walk_p2p_mk3.h"
#include "map_bit_array_overlay.h"
#include "cont_indexer.h"
#include "game_map_defs.h"
#include "game_map_defs.h"
#include "generator_constants.h"
#include "map_loader.h"
#include "runtime_trace_dbg.h"
#include "water_land_overlay.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr k_in_path = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr k_in_riv = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr k_out_path = "/home/w/Projects/simple-map-gen/distance-p2p-mk3-walk.ppm";
static const cstr k_trace_path = "/home/w/Projects/simple-map-gen/distance-p2p-mk3-walk.trace";
static const u16 k_turn_none = 0xFFFFu;
static const u32 k_pred_none = 0xFFFFFFFFu;
static const u32 k_turn_sent = Generate_DistanceP2P_Mk3::k_turn_sent;
static const u32 k_step_top = Generate_DistanceP2P_Mk3::k_ovl_mod - 1u;
static const u8 k_wtr_b = 30;
static const u8 k_wtr_g = 110;
static const u8 k_wtr_r = 220;
static const u8 k_pt_r = 255;
static const u8 k_pt_g = 0;
static const u8 k_pt_b = 0;
static const u8 k_path_r = 255;
static const u8 k_path_g = 220;
static const u8 k_path_b = 0;
static const u8 k_path_riv_r = 40;
static const u8 k_path_riv_g = 120;
static const u8 k_path_riv_b = 255;
static const u8 k_inland_r = 34;
static const u8 k_inland_g = 112;
static const u8 k_inland_b = 48;

static const cstr k_ansi_rst = "\033[0m";
static const cstr k_ansi_blu = "\033[94m";

struct WalkSimRes {
    u32 path_len;
    u32 game_turns;
    bool traced;
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_water (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool is_walk (u8 t) {
    return t != TERR_MOUNTAINS[0] && !is_water(t);
}

static bool pt_in_region (
    const ContIndexer& idx,
    u32 rid,
    u16 x,
    u16 y) {
    const std::vector<ContRegionRec>& regs = idx.regions();
    if (rid >= regs.size()) {
        return false;
    }
    const ContRegionRec& r = regs[rid];
    const u8* rgb = idx.count_map_rgb();
    if (rgb == nullptr) {
        return false;
    }
    const u32 i = tidx(idx.width(), x, y) * 3u;
    return rgb[i + 0u] == r.m_r && rgb[i + 1u] == r.m_g && rgb[i + 2u] == r.m_b;
}

static u32 pick_largest_rid (const ContIndexer& idx) {
    u32 best = 0;
    u32 best_area = 0;
    const std::vector<ContRegionRec>& regs = idx.regions();
    for (u32 i = 0; i < regs.size(); ++i) {
        if (regs[i].m_area_px > best_area) {
            best_area = regs[i].m_area_px;
            best = i;
        }
    }
    return best;
}

static bool borders_water (
    const u8* terrain,
    u16 w,
    u16 h,
    u16 x,
    u16 y) {
    static const i16 dx4[] = {0, 1, 0, -1};
    static const i16 dy4[] = {-1, 0, 1, 0};
    for (u32 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + dx4[k];
        const i32 ny = static_cast<i32>(y) + dy4[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (tx >= w || ty >= h) {
            continue;
        }
        if (is_water(terrain[tidx(w, tx, ty)])) {
            return true;
        }
    }
    return false;
}

static bool pick_src_dst (
    const u8* terrain,
    const ContIndexer& idx,
    u32 rid,
    u16 w,
    u16 h,
    u16* land_dep,
    u16& src_x,
    u16& src_y,
    u16& dst_x,
    u16& dst_y) {
    src_x = 0;
    src_y = 0;
    dst_x = 0;
    dst_y = 0;
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < tile_n; ++i) {
        land_dep[i] = k_turn_none;
    }
    u32* q = new u32[tile_n];
    u32 qn = 0;
    bool have_dst = false;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!pt_in_region(idx, rid, x, y)) {
                continue;
            }
            if (!is_walk(terrain[tidx(w, x, y)])) {
                continue;
            }
            if (!borders_water(terrain, w, h, x, y)) {
                continue;
            }
            const u32 i = tidx(w, x, y);
            land_dep[i] = 0;
            q[qn++] = i;
            if (!have_dst) {
                dst_x = x;
                dst_y = y;
                have_dst = true;
            }
        }
    }
    if (!have_dst) {
        delete[] q;
        return false;
    }
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 cur = land_dep[i];
        if (cur >= 65534u) {
            continue;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const u16 nxt = static_cast<u16>(cur + 1u);
        static const i16 dx4[] = {0, 1, 0, -1};
        static const i16 dy4[] = {-1, 0, 1, 0};
        for (u32 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(px) + dx4[k];
            const i32 ny = static_cast<i32>(py) + dy4[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 tx = static_cast<u16>(nx);
            const u16 ty = static_cast<u16>(ny);
            if (tx >= w || ty >= h) {
                continue;
            }
            if (!pt_in_region(idx, rid, tx, ty)) {
                continue;
            }
            const u32 ni = tidx(w, tx, ty);
            if (!is_walk(terrain[ni]) || land_dep[ni] != k_turn_none) {
                continue;
            }
            land_dep[ni] = nxt;
            q[qn++] = ni;
        }
    }
    delete[] q;
    u16 best = 0;
    bool have_src = false;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 d = land_dep[tidx(w, x, y)];
            if (d == k_turn_none || d == 0u) {
                continue;
            }
            if (!have_src || d > best) {
                best = d;
                src_x = x;
                src_y = y;
                have_src = true;
            }
        }
    }
    return have_src;
}

static bool ovl_reach (const u32* pred, u16 w, u16 x, u16 y) {
    return pred[tidx(w, x, y)] != k_pred_none;
}

static u8 path_mark (const u8* rivers, u16 w, u16 x, u16 y) {
    return rivers[tidx(w, x, y)] != 0u ? 2u : 1u;
}

static WalkSimRes sim_walk (
    const WalkP2P_Mk3& wk,
    const MapBitArrayOverlay& turn_o,
    const MapBitArrayOverlay& step_o,
    const u32* pred,
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy,
    u8* path_m) {
    WalkSimRes res = {};
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    u16 px = sx;
    u16 py = sy;
    i32 mp = static_cast<i32>(PATH_MP_TURN);
    u32 turn = 1u;
    for (u32 i = 0; i < tile_n; ++i) {
        path_m[i] = 0;
    }
    path_m[tidx(w, px, py)] = path_mark(rivers, w, px, py);
    u32 guard = 0;
    while (px != dx || py != dy) {
        if (++guard > tile_n) {
            TRACE_PATH_FAILURE(("mk3 walk guard"));
            break;
        }
        while (px != dx || py != dy) {
            const WalkP2P_Mk3::StepRes st = wk.peek_step(
                turn_o, step_o, pred, terrain, rivers, w, h, px, py);
            if (!st.have) {
                TRACE_P2P_MK3_WALK_STALL((
                    px, py, static_cast<u16>(mp > 0 ? mp : 0), turn));
                TRACE_PATH_FAILURE(("mk3 walk stall"));
                res.game_turns = turn;
                return res;
            }
            const i32 cost = static_cast<i32>(st.cost);
            if (mp < cost) {
                break;
            }
            mp -= cost;
            TRACE_P2P_MK3_WALK_STEP((
                px, py, st.nx, st.ny, st.cost,
                static_cast<u16>(mp > 0 ? mp : 0), turn));
            px = st.nx;
            py = st.ny;
            path_m[tidx(w, px, py)] = path_mark(rivers, w, px, py);
            res.path_len++;
            if (mp <= 0) {
                break;
            }
        }
        if (px == dx && py == dy) {
            res.traced = true;
            TRACE_P2P_MK3_WALK_DONE((dx, dy, res.path_len, turn));
            break;
        }
        turn++;
        mp += static_cast<i32>(PATH_MP_TURN);
    }
    res.game_turns = turn;
    return res;
}

static bool load_riv (cstr path, u16 ew, u16 eh, u8** out_riv) {
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
    if (c == EOF || wi == 0 || hi == 0 || wi != ew || hi != eh) {
        std::fclose(fp);
        return false;
    }
    const u32 tn = static_cast<u32>(ew) * static_cast<u32>(eh);
    u8* rgb = new u8[tn * 3u];
    if (std::fread(rgb, 1, static_cast<size_t>(tn) * 3u, fp) != static_cast<size_t>(tn) * 3u) {
        delete[] rgb;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    u8* riv = new u8[tn];
    for (u32 i = 0; i < tn; ++i) {
        const u8* px = rgb + i * 3u;
        riv[i] = (px[0] != 0 || px[1] != 0 || px[2] != 0) ? 1 : 0;
    }
    delete[] rgb;
    *out_riv = riv;
    return true;
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void paint_pt (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    static const i16 rad = 4;
    for (i16 dy = -rad; dy <= rad; ++dy) {
        for (i16 dx = -rad; dx <= rad; ++dx) {
            const i32 nx = static_cast<i32>(x) + dx;
            const i32 ny = static_cast<i32>(y) + dy;
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 tx = static_cast<u16>(nx);
            const u16 ty = static_cast<u16>(ny);
            if (tx >= w || ty >= h) {
                continue;
            }
            if (dx * dx + dy * dy > rad * rad) {
                continue;
            }
            const u32 o = tidx(w, tx, ty) * 3u;
            rgb[o + 0u] = r;
            rgb[o + 1u] = g;
            rgb[o + 2u] = b;
        }
    }
}

static u8 norm_gray_u4 (u32 d) {
    if (d >= k_turn_sent) {
        return 0;
    }
    return static_cast<u8>((d * 255u) / k_step_top);
}

static bool build_img (
    const u8* terrain,
    const MapBitArrayOverlay& turn_o,
    const u32* pred,
    const u8* path_m,
    u16 w,
    u16 h,
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    u8* rgb) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = tidx(w, x, y);
            const u8 t = terrain[i];
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            if (is_water(t)) {
                r = k_wtr_r;
                g = k_wtr_g;
                b = k_wtr_b;
            } else if (ovl_reach(pred, w, x, y)) {
                const u8 gry = norm_gray_u4(turn_o.get(x, y));
                r = gry;
                g = gry;
                b = gry;
            } else {
                r = k_inland_r;
                g = k_inland_g;
                b = k_inland_b;
            }
            rgb[i * 3u + 0] = r;
            rgb[i * 3u + 1] = g;
            rgb[i * 3u + 2] = b;
        }
    }
    for (u32 i = 0; i < n; ++i) {
        const u8 pm = path_m[i];
        if (pm == 0) {
            continue;
        }
        if (pm == 2u) {
            rgb[i * 3u + 0] = k_path_riv_r;
            rgb[i * 3u + 1] = k_path_riv_g;
            rgb[i * 3u + 2] = k_path_riv_b;
        } else {
            rgb[i * 3u + 0] = k_path_r;
            rgb[i * 3u + 1] = k_path_g;
            rgb[i * 3u + 2] = k_path_b;
        }
    }
    paint_pt(rgb, w, h, src_x, src_y, k_pt_r, k_pt_g, k_pt_b);
    paint_pt(rgb, w, h, dst_x, dst_y, k_pt_r, k_pt_g, k_pt_b);
    return true;
}

static void print_outcome (
    u16 w,
    u16 h,
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    u16 max_turn,
    const WalkSimRes& res,
    double gen_sec,
    double walk_sec,
    cstr out_path) {
    const bool pass = res.traced;
    std::printf("test walk p2p mk3: %s\n", pass ? "PASS" : "FAIL");
    std::printf("  map %ux%u src (%u,%u) dst (%u,%u) max_turn %u\n",
        static_cast<unsigned>(w),
        static_cast<unsigned>(h),
        static_cast<unsigned>(src_x),
        static_cast<unsigned>(src_y),
        static_cast<unsigned>(dst_x),
        static_cast<unsigned>(dst_y),
        static_cast<unsigned>(max_turn));
    std::printf("  path len %u traced %s\n",
        res.path_len,
        res.traced ? "yes" : "no");
    std::printf("  %swalk turns %u%s\n",
        k_ansi_blu,
        res.game_turns,
        k_ansi_rst);
    std::printf("  %sgenerate time %.6f s%s\n",
        k_ansi_blu,
        gen_sec,
        k_ansi_rst);
    std::printf("  %swalk time %.6f s%s\n",
        k_ansi_blu,
        walk_sec,
        k_ansi_rst);
    std::printf("  saved %s\n", out_path);
}

int main () {
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(k_in_path, map)) {
        std::printf("*** FAILED load %s\n", k_in_path);
        return 1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terrain = map.data();
    MapArrayTerrain arr;
    if (!arr.assign_copy(w, h, terrain)) {
        std::printf("*** FAILED terrain copy\n");
        return 1;
    }
    WaterLandOverlay wl(arr);
    if (!wl.is_valid()) {
        std::printf("*** FAILED water-land overlay\n");
        return 1;
    }
    ContIndexer cidx(wl);
    if (!cidx.is_valid() || cidx.region_count() == 0u) {
        std::printf("*** FAILED cont indexer\n");
        return 1;
    }
    const u32 rid = pick_largest_rid(cidx);
    const ContRegionRec& reg = cidx.regions()[rid];
    std::printf("largest landmass rid=%u area=%u seed=(%u,%u)\n",
        rid,
        static_cast<unsigned>(reg.m_area_px),
        static_cast<unsigned>(reg.m_sx),
        static_cast<unsigned>(reg.m_sy));
    u8* rivers = nullptr;
    if (!load_riv(k_in_riv, w, h, &rivers)) {
        std::printf("*** FAILED load %s\n", k_in_riv);
        return 1;
    }
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* land_dep = new u16[tile_n];
    u16 src_x = 0;
    u16 src_y = 0;
    u16 dst_x = 0;
    u16 dst_y = 0;
    if (!pick_src_dst(terrain, cidx, rid, w, h, land_dep, src_x, src_y, dst_x, dst_y)) {
        std::printf("*** FAILED pick src/dst on landmass\n");
        delete[] land_dep;
        return 1;
    }
    std::printf("picked src (%u,%u) land_depth %u dst (%u,%u)\n",
        static_cast<unsigned>(src_x), static_cast<unsigned>(src_y),
        static_cast<unsigned>(land_dep[tidx(w, src_x, src_y)]),
        static_cast<unsigned>(dst_x), static_cast<unsigned>(dst_y));
    MapBitArrayOverlay turn_o(w, h, Generate_DistanceP2P_Mk3::k_turn_bpv);
    MapBitArrayOverlay step_o(w, h, Generate_DistanceP2P_Mk3::k_step_bpv);
    u32* pred = new u32[tile_n];
    u16* scr[Generate_DistanceP2P_Mk3::k_scr_n];
    for (u32 s = 0; s < Generate_DistanceP2P_Mk3::k_scr_n; ++s) {
        scr[s] = new u16[tile_n];
    }
    u16 p2p_max = 0;
    const clock_t t0 = clock();
    const bool ok = Generate_DistanceP2P_Mk3::generate(
        terrain, rivers, w, h, src_x, src_y, dst_x, dst_y, &turn_o, &step_o, pred, scr, &p2p_max);
    const double gen_sec = static_cast<double>(clock() - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok) {
        std::printf("*** FAILED p2p generate (no path)\n");
        return 1;
    }
    u8* path_m = new u8[tile_n];
    TRACE_SETUP((k_trace_path));
    WalkP2P_Mk3 walker;
    WalkSimRes res = {};
    const clock_t t1 = clock();
    res = sim_walk(
        walker, turn_o, step_o, pred, terrain, rivers, w, h,
        src_x, src_y, dst_x, dst_y, path_m);
    const double walk_sec = static_cast<double>(clock() - t1) / static_cast<double>(CLOCKS_PER_SEC);
    u8* rgb = new u8[static_cast<size_t>(tile_n) * 3u];
    build_img(terrain, turn_o, pred, path_m, w, h, src_x, src_y, dst_x, dst_y, rgb);
    if (!save_rgb_ppm(k_out_path, rgb, w, h)) {
        std::printf("*** FAILED save %s\n", k_out_path);
        delete[] path_m;
        return 1;
    }
    print_outcome(w, h, src_x, src_y, dst_x, dst_y, p2p_max, res, gen_sec, walk_sec, k_out_path);
    delete[] rgb;
    delete[] path_m;
    delete[] rivers;
    delete[] pred;
    delete[] land_dep;
    for (u32 s = 0; s < Generate_DistanceP2P_Mk3::k_scr_n; ++s) {
        delete[] scr[s];
    }
    if (!res.traced) {
        TRACE_PATH_FAILURE(("mk3 walk trace failed"));
        std::printf("  trace %s\n", k_trace_path);
        std::printf("*** FAILED walk_p2p_mk3\n");
        return 1;
    }
    std::printf("*** PASSED walk_p2p_mk3\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
