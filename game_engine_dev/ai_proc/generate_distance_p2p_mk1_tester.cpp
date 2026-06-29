//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "generate_distance_p2p_mk1.h"
#include "cont_indexer.h"
#include "game_map_defs.h"
#include "generator_constants.h"
#include "map_loader.h"
#include "water_land_overlay.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr k_in_path = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr k_in_riv = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr k_out_path = "/home/w/Projects/simple-map-gen/distance-p2p-mk1.ppm";
static const u16 k_turn_none = 0xFFFFu;
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
static const bool k_dbg_walk = false;
static const u32 k_dbg_walk_max = 40u;

static const cstr k_ansi_rst = "\033[0m";
static const cstr k_ansi_grn = "\033[32m";
static const cstr k_ansi_red = "\033[31m";
static const cstr k_ansi_blu = "\033[94m";

struct P2PAuditRes {
    u32 n_val;
    u32 n_has_step;
    u32 n_no_step;
    u32 n_end;
    u32 n_rem1000;
    u32 n_rem1000_riv;
};

static bool audit_pass (const P2PAuditRes& a) {
    return a.n_no_step == 0u;
}

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

static u8 norm_gray (u16 d, u16 max_d) {
    if (d == k_turn_none) {
        return 0;
    }
    if (max_d == 0) {
        return 255;
    }
    return static_cast<u8>((static_cast<u32>(d) * 255u) / static_cast<u32>(max_d));
}

static i32 step_dec (u16 v) {
    return static_cast<i32>(v) - static_cast<i32>(PATH_MP_TURN);
}

static bool step_down (
    const u16* turn,
    const u16* step,
    u16 w,
    u16 h,
    u16& x,
    u16& y);

static void dbg_tile (
    cstr tag,
    u16 x,
    u16 y,
    const u16* turn,
    const u16* step,
    const u32* pred,
    const u8* rivers,
    u16 w) {
    const u32 i = tidx(w, x, y);
    std::printf(
        "  %s (%u,%u) turn=%u rem=%d pred=%u riv=%u\n",
        tag,
        static_cast<unsigned>(x),
        static_cast<unsigned>(y),
        static_cast<unsigned>(turn[i]),
        step_dec(step[i]),
        static_cast<unsigned>(pred[i]),
        static_cast<unsigned>(rivers[i]));
}

static void dbg_nbrs (
    u16 x,
    u16 y,
    const u16* turn,
    const u16* step,
    const u32* pred,
    const u8* rivers,
    u16 w,
    u16 h) {
    const u32 ci = tidx(w, x, y);
    const u16 ct = turn[ci];
    const i32 cs = step_dec(step[ci]);
    static const i16 dx4[] = {0, 1, 0, -1};
    static const i16 dy4[] = {-1, 0, 1, 0};
    std::printf("  nbrs from (%u,%u) cur turn=%u rem=%d:\n",
        static_cast<unsigned>(x),
        static_cast<unsigned>(y),
        static_cast<unsigned>(ct),
        cs);
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
        const u32 ni = tidx(w, tx, ty);
        const u16 nt = turn[ni];
        const i32 ns = step_dec(step[ni]);
        cstr why = "ok";
        if (nt == k_turn_none) {
            why = "none";
        } else if (nt > ct) {
            why = "turn_high";
        } else if (nt == ct && ns <= cs) {
            why = "rem_low";
        }
        std::printf(
            "    (%u,%u) turn=%u rem=%d pred=%u riv=%u -> %s\n",
            static_cast<unsigned>(tx),
            static_cast<unsigned>(ty),
            static_cast<unsigned>(nt),
            ns,
            static_cast<unsigned>(pred[ni]),
            static_cast<unsigned>(rivers[ni]),
            why);
    }
}

static bool has_valid_step (
    u16 x,
    u16 y,
    const u16* turn,
    const u16* step,
    u16 w,
    u16 h) {
    const u32 ci = tidx(w, x, y);
    const u16 ct = turn[ci];
    const i32 cs = step_dec(step[ci]);
    if (ct == k_turn_none || ct == 0u) {
        return false;
    }
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
        const u32 ni = tidx(w, tx, ty);
        const u16 nt = turn[ni];
        const i32 ns = step_dec(step[ni]);
        if (nt == k_turn_none || nt > ct || (nt == ct && ns <= cs)) {
            continue;
        }
        return true;
    }
    return false;
}

static P2PAuditRes audit_overlay (
    const u16* turn,
    const u16* step,
    const u8* rivers,
    u16 w,
    u16 h) {
    P2PAuditRes res = {};
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = tidx(w, x, y);
            if (turn[i] == k_turn_none) {
                continue;
            }
            res.n_val++;
            if (turn[i] == 0u) {
                res.n_end++;
                continue;
            }
            if (has_valid_step(x, y, turn, step, w, h)) {
                res.n_has_step++;
            } else {
                res.n_no_step++;
            }
            if (step_dec(step[i]) == 1000) {
                res.n_rem1000++;
                if (rivers[i] != 0u) {
                    res.n_rem1000_riv++;
                }
            }
        }
    }
    return res;
}

static void print_test_outcome (
    u16 w,
    u16 h,
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    u16 max_turn,
    u32 path_len,
    u32 walk_turns,
    bool traced,
    const P2PAuditRes& audit,
    double ovl_sec,
    double walk_sec,
    cstr out_path) {
    const bool pass = audit_pass(audit) && traced;
    std::printf("test p2p mk1: %s\n", pass ? "PASS" : "FAIL");
    std::printf("  map %ux%u src (%u,%u) dst (%u,%u) max_turn %u\n",
        static_cast<unsigned>(w),
        static_cast<unsigned>(h),
        static_cast<unsigned>(src_x),
        static_cast<unsigned>(src_y),
        static_cast<unsigned>(dst_x),
        static_cast<unsigned>(dst_y),
        static_cast<unsigned>(max_turn));
    std::printf(
        "  overlay val=%u %shas_step=%u%s %sno_step=%u%s end=%u\n",
        audit.n_val,
        k_ansi_grn,
        audit.n_has_step,
        k_ansi_rst,
        audit.n_no_step == 0u ? k_ansi_grn : k_ansi_red,
        audit.n_no_step,
        k_ansi_rst,
        audit.n_end);
    std::printf("  rem=1000 %u on_river %u\n",
        audit.n_rem1000,
        audit.n_rem1000_riv);
    std::printf("  path len %u traced %s\n",
        path_len,
        traced ? "yes" : "no");
    std::printf("  %swalk turns %u%s\n",
        k_ansi_blu,
        walk_turns,
        k_ansi_rst);
    std::printf("  %soverlay time %.6f s%s\n",
        k_ansi_blu,
        ovl_sec,
        k_ansi_rst);
    std::printf("  %swalk time %.6f s%s\n",
        k_ansi_blu,
        walk_sec,
        k_ansi_rst);
    std::printf("  saved %s\n", out_path);
}

static void dbg_pred_chain (
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    const u32* pred,
    u16 w,
    u32 tile_n) {
    const u32 dst_i = tidx(w, dst_x, dst_y);
    u32 cur = tidx(w, src_x, src_y);
    u8* vis = new u8[tile_n];
    for (u32 i = 0; i < tile_n; ++i) {
        vis[i] = 0;
    }
    std::printf("dbg pred chain src->dst:\n");
    for (u32 n = 0; n < 24u; ++n) {
        const u16 x = static_cast<u16>(cur % static_cast<u32>(w));
        const u16 y = static_cast<u16>(cur / static_cast<u32>(w));
        std::printf("  %u: (%u,%u) pred=%u\n",
            n,
            static_cast<unsigned>(x),
            static_cast<unsigned>(y),
            static_cast<unsigned>(pred[cur]));
        if (cur == dst_i) {
            std::printf("  pred reached dst at hop %u\n", n);
            delete[] vis;
            return;
        }
        if (vis[cur] != 0) {
            std::printf("  pred cycle at (%u,%u) hop %u\n",
                static_cast<unsigned>(x),
                static_cast<unsigned>(y),
                n);
            delete[] vis;
            return;
        }
        vis[cur] = 1;
        const u32 nxt = pred[cur];
        if (nxt == cur || nxt >= tile_n) {
            std::printf("  pred dead at (%u,%u) nxt=%u\n",
                static_cast<unsigned>(x),
                static_cast<unsigned>(y),
                static_cast<unsigned>(nxt));
            delete[] vis;
            return;
        }
        cur = nxt;
    }
    std::printf("  pred truncated at 24 hops cur=(%u,%u)\n",
        static_cast<unsigned>(cur % static_cast<u32>(w)),
        static_cast<unsigned>(cur / static_cast<u32>(w)));
    delete[] vis;
}

static u8 path_mark (const u8* rivers, u16 w, u16 x, u16 y) {
    return rivers[tidx(w, x, y)] != 0u ? 2u : 1u;
}

static bool trace_path (
    const u16* turn,
    const u16* step,
    const u32* pred,
    const u8* rivers,
    u16 w,
    u16 h,
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    u8* path_m,
    u32& path_len,
    u32& walk_turns,
    bool& traced) {
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    u16 px = src_x;
    u16 py = src_y;
    path_len = 0;
    walk_turns = 0;
    traced = false;
    bool on_riv = false;
    u16 prev_t = turn[tidx(w, px, py)];
    for (u32 i = 0; i < tile_n; ++i) {
        path_m[i] = 0;
    }
    path_m[tidx(w, px, py)] = path_mark(rivers, w, px, py);
    if (k_dbg_walk) {
        std::printf("dbg walk src->dst:\n");
        dbg_tile("start", px, py, turn, step, pred, rivers, w);
    }
    for (u32 guard = 0; guard < tile_n; ++guard) {
        const u32 ci = tidx(w, px, py);
        if (turn[ci] == 0u) {
            traced = (px == dst_x && py == dst_y);
            if (k_dbg_walk) {
                dbg_tile(traced ? "done" : "turn0_wrong", px, py, turn, step, pred, rivers, w);
            }
            break;
        }
        const bool riv_here = rivers[ci] != 0u;
        if (!on_riv && riv_here) {
            on_riv = true;
            if (k_dbg_walk) {
                std::printf("dbg enter river at (%u,%u) step %u\n",
                    static_cast<unsigned>(px),
                    static_cast<unsigned>(py),
                    static_cast<unsigned>(path_len));
                dbg_nbrs(px, py, turn, step, pred, rivers, w, h);
            }
        }
        if (k_dbg_walk && (path_len < k_dbg_walk_max || riv_here)) {
            std::printf("dbg step %u at (%u,%u) riv=%u\n",
                static_cast<unsigned>(path_len),
                static_cast<unsigned>(px),
                static_cast<unsigned>(py),
                static_cast<unsigned>(rivers[ci]));
        }
        if (!step_down(turn, step, w, h, px, py)) {
            if (k_dbg_walk) {
                std::printf("dbg STALL at (%u,%u) after %u steps on_riv=%u\n",
                    static_cast<unsigned>(px),
                    static_cast<unsigned>(py),
                    static_cast<unsigned>(path_len),
                    on_riv ? 1u : 0u);
                dbg_nbrs(px, py, turn, step, pred, rivers, w, h);
            }
            break;
        }
        const u16 cur_t = turn[tidx(w, px, py)];
        if (cur_t < prev_t) {
            walk_turns += static_cast<u32>(prev_t - cur_t);
        }
        prev_t = cur_t;
        path_m[tidx(w, px, py)] = path_mark(rivers, w, px, py);
        path_len++;
    }
    return traced;
}

static bool step_down (
    const u16* turn,
    const u16* step,
    u16 w,
    u16 h,
    u16& x,
    u16& y) {
    const u32 ci = tidx(w, x, y);
    const u16 ct = turn[ci];
    const i32 cs = step_dec(step[ci]);
    if (ct == k_turn_none || ct == 0u) {
        return false;
    }
    u16 bt = ct;
    i32 bs = cs;
    u16 bx = x;
    u16 by = y;
    bool found = false;
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
        const u32 ni = tidx(w, tx, ty);
        const u16 nt = turn[ni];
        const i32 ns = step_dec(step[ni]);
        if (nt == k_turn_none || nt > ct || (nt == ct && ns <= cs)) {
            continue;
        }
        if (!found || nt < bt || (nt == bt && ns > bs)) {
            bt = nt;
            bs = ns;
            bx = tx;
            by = ty;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    x = bx;
    y = by;
    return true;
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

static bool build_img (
    const u8* terrain,
    const u16* turn,
    const u8* path_m,
    u16 w,
    u16 h,
    u16 max_d,
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    u8* rgb) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        const u8 t = terrain[i];
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (is_water(t)) {
            r = k_wtr_r;
            g = k_wtr_g;
            b = k_wtr_b;
        } else if (turn[i] != k_turn_none) {
            const u8 gry = norm_gray(turn[i], max_d);
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
    if (path_m != nullptr) {
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
    }
    paint_pt(rgb, w, h, src_x, src_y, k_pt_r, k_pt_g, k_pt_b);
    paint_pt(rgb, w, h, dst_x, dst_y, k_pt_r, k_pt_g, k_pt_b);
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

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
    u16* turn = new u16[tile_n];
    u16* step = new u16[tile_n];
    u32* pred = new u32[tile_n];
    u16* scr[Generate_DistanceP2P_Mk1::k_scr_n];
    for (u32 s = 0; s < Generate_DistanceP2P_Mk1::k_scr_n; ++s) {
        scr[s] = new u16[tile_n];
    }
    u16 p2p_max = 0;
    const clock_t t0 = clock();
    const bool ok = Generate_DistanceP2P_Mk1::generate(
        terrain, rivers, w, h, src_x, src_y, dst_x, dst_y, turn, step, pred, scr, &p2p_max);
    const double ovl_sec = static_cast<double>(clock() - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok) {
        std::printf("*** FAILED p2p generate (no path)\n");
        return 1;
    }
    if (k_dbg_walk) {
        std::printf("dbg overlay src turn=%u rem=%d dst turn=%u rem=%d max=%u\n",
            static_cast<unsigned>(turn[tidx(w, src_x, src_y)]),
            step_dec(step[tidx(w, src_x, src_y)]),
            static_cast<unsigned>(turn[tidx(w, dst_x, dst_y)]),
            step_dec(step[tidx(w, dst_x, dst_y)]),
            static_cast<unsigned>(p2p_max));
        dbg_pred_chain(src_x, src_y, dst_x, dst_y, pred, w, tile_n);
    }
    const P2PAuditRes audit = audit_overlay(turn, step, rivers, w, h);
    u8* path_m = new u8[tile_n];
    u32 path_len = 0;
    u32 walk_turns = 0;
    bool traced = false;
    const clock_t t1 = clock();
    trace_path(turn, step, pred, rivers, w, h, src_x, src_y, dst_x, dst_y, path_m, path_len, walk_turns, traced);
    const double walk_sec = static_cast<double>(clock() - t1) / static_cast<double>(CLOCKS_PER_SEC);
    u8* rgb = new u8[static_cast<size_t>(tile_n) * 3u];
    build_img(terrain, turn, path_m, w, h, p2p_max, src_x, src_y, dst_x, dst_y, rgb);
    if (!save_rgb_ppm(k_out_path, rgb, w, h)) {
        std::printf("*** FAILED save %s\n", k_out_path);
        delete[] path_m;
        return 1;
    }
    print_test_outcome(w, h, src_x, src_y, dst_x, dst_y, p2p_max, path_len, walk_turns, traced, audit, ovl_sec, walk_sec, k_out_path);
    const bool test_ok = audit_pass(audit) && traced;
    delete[] rgb;
    delete[] path_m;
    delete[] rivers;
    delete[] turn;
    delete[] step;
    delete[] pred;
    delete[] land_dep;
    for (u32 s = 0; s < Generate_DistanceP2P_Mk1::k_scr_n; ++s) {
        delete[] scr[s];
    }
    if (!test_ok) {
        std::printf("*** FAILED generate_distance_p2p_mk1\n");
        return 1;
    }
    std::printf("*** PASSED generate_distance_p2p_mk1\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
