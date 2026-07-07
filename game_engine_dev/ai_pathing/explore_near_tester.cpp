//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstdarg>
#include <time.h>
#include <sys/stat.h>

#include "explore_near.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "map_terrain_validate.h"
#include "map_bit_overlay.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr EN_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr EN_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr EN_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr EN_OUT_DIR = "/home/w/Projects/simple-map-gen/explore_near_results";
static const u16 EN_SX = 499u;
static const u16 EN_SY = 499u;
static const u16 EN_TURNS = PATH_MP_TURN;
static const u16 EN_SIGHT = 3u;
static const u32 EN_MOVE_CAP = static_cast<u32>(EN_TURNS) * 2u;

static const cstr k_ansi_rst = "\033[0m";
static const cstr k_ansi_grn = "\033[32m";
static const cstr k_ansi_red = "\033[31m";
static const cstr k_ansi_blu = "\033[94m";

static void print_cls_size (size_t n) {
    if (n < 1000u) {
        std::printf("%sTotal size: %zuB%s\n", k_ansi_blu, n, k_ansi_rst);
    } else {
        std::printf("%sTotal size: %.2fKB%s\n", k_ansi_blu, static_cast<double>(n) / 1024.0, k_ansi_rst);
    }
}

static void t_fail (cstr fmt, ...) {
    va_list ap;
    std::printf("%s", k_ansi_red);
    va_start(ap, fmt);
    std::vprintf(fmt, ap);
    va_end(ap);
    std::printf("%s", k_ansi_rst);
}

static void t_pass (cstr fmt, ...) {
    va_list ap;
    std::printf("%s", k_ansi_grn);
    va_start(ap, fmt);
    std::vprintf(fmt, ap);
    va_end(ap);
    std::printf("%s", k_ansi_rst);
}

static double move_elapsed_us (const timespec& t0, const timespec& t1) {
    const double sec = static_cast<double>(t1.tv_sec - t0.tv_sec);
    const double nsec = static_cast<double>(t1.tv_nsec - t0.tv_nsec);
    return sec * 1000000.0 + nsec / 1000.0;
}

static void record_move (double* us, u32 cap, u32& n, const timespec& t0, const timespec& t1) {
    if (n >= cap) {
        return;
    }
    us[n++] = move_elapsed_us(t0, t1);
}

static void print_move_timing (const double* us, u32 n) {
    if (n == 0u) {
        std::printf("move avg n/a\n");
        return;
    }
    double sum = 0.0;
    for (u32 i = 0u; i < n; ++i) {
        sum += us[i];
    }
    const double avg = sum / static_cast<double>(n);
    std::printf("move avg %.2f us (%u samples)\n", avg, static_cast<unsigned>(n));
    u32 show = 10u;
    if (show > n) {
        show = n;
    }
    double* cp = new double[n];
    if (cp == nullptr) {
        return;
    }
    for (u32 i = 0u; i < n; ++i) {
        cp[i] = us[i];
    }
    std::printf("  worst %u:", static_cast<unsigned>(show));
    for (u32 s = 0u; s < show; ++s) {
        u32 bi = s;
        for (u32 i = s + 1u; i < n; ++i) {
            if (cp[i] > cp[bi]) {
                bi = i;
            }
        }
        const double v = cp[bi];
        cp[bi] = cp[s];
        cp[s] = v;
        std::printf(" %.0f", v);
    }
    std::printf(" us\n");
    delete[] cp;
}

class VisOverlay {
public:
    explicit VisOverlay (u16 w, u16 h) :
        m_w(w),
        m_h(h),
        m_v(nullptr) {
        const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
        m_v = new u32[n];
        for (u32 i = 0u; i < n; ++i) {
            m_v[i] = 0u;
        }
    }
    ~VisOverlay () {
        delete[] m_v;
    }
    void inc (u16 x, u16 y) {
        if (x >= m_w || y >= m_h || m_v == nullptr) {
            return;
        }
        const u32 i = static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x);
        ++m_v[i];
    }
    void stats (u32& tiles, u32& tot) const {
        tiles = 0u;
        tot = 0u;
        if (m_v == nullptr) {
            return;
        }
        const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
        for (u32 i = 0u; i < n; ++i) {
            if (m_v[i] == 0u) {
                continue;
            }
            ++tiles;
            tot += m_v[i];
        }
    }

private:
    u16 m_w;
    u16 m_h;
    u32* m_v;
};

static void print_visit_stats (const VisOverlay& vis) {
    u32 vis_tiles = 0u;
    u32 vis_tot = 0u;
    vis.stats(vis_tiles, vis_tot);
    if (vis_tiles > 0u) {
        const double vis_avg = static_cast<double>(vis_tot) / static_cast<double>(vis_tiles);
        std::printf("visits: %u tiles %u total avg %.2f\n",
            static_cast<unsigned>(vis_tiles),
            static_cast<unsigned>(vis_tot),
            vis_avg);
    } else {
        std::printf("visits: 0 tiles 0 total avg 0.00\n");
    }
}

static bool is_spawn_terr (u8 t) {
    if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0]) {
        return false;
    }
    return true;
}

static u32 cheb_dist (u16 x, u16 y, u16 cx, u16 cy) {
    const u32 dx = (x > cx) ? static_cast<u32>(x - cx) : static_cast<u32>(cx - x);
    const u32 dy = (y > cy) ? static_cast<u32>(y - cy) : static_cast<u32>(cy - y);
    return (dx > dy) ? dx : dy;
}

static bool find_land_center (const GameArraySimple& map, u16& ox, u16& oy) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u16 cx = w / 2u;
    const u16 cy = h / 2u;
    const u32 max_d = static_cast<u32>(cx > cy ? cx : cy) + 1u;
    for (u32 d = 0u; d <= max_d; ++d) {
        for (u16 y = 0u; y < h; ++y) {
            for (u16 x = 0u; x < w; ++x) {
                if (cheb_dist(x, y, cx, cy) != d) {
                    continue;
                }
                if (!is_spawn_terr(map.get_terrain(x, y))) {
                    continue;
                }
                ox = x;
                oy = y;
                return true;
            }
        }
    }
    return false;
}

static bool ensure_dir (cstr path) {
    if (mkdir(path, 0755) == 0) {
        return true;
    }
    return errno == EEXIST;
}

static u32 cnt_exp (const MapBitOverlay& ov) {
    u32 c = 0u;
    for (u16 y = 0u; y < ov.height(); ++y) {
        for (u16 x = 0u; x < ov.width(); ++x) {
            c += ov.get(x, y);
        }
    }
    return c;
}

static void mark_pt (u8* rgb, u16 w, u16 h, u16 px, u16 py, u8 r, u8 g, u8 b) {
    if (px >= w || py >= h) {
        return;
    }
    const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
    rgb[i * 3u + 0] = r;
    rgb[i * 3u + 1] = g;
    rgb[i * 3u + 2] = b;
}

static void mark_cross (u8* rgb, u16 w, u16 h, u16 px, u16 py, u8 r, u8 g, u8 b) {
    for (i16 dy = -2; dy <= 2; ++dy) {
        for (i16 dx = -2; dx <= 2; ++dx) {
            if ((dx < 0 ? -dx : dx) > 1 && (dy < 0 ? -dy : dy) > 1) {
                continue;
            }
            const i32 xi = static_cast<i32>(px) + dx;
            const i32 yi = static_cast<i32>(py) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            mark_pt(rgb, w, h, static_cast<u16>(xi), static_cast<u16>(yi), r, g, b);
        }
    }
}

static ExOpt mk_opt (bool on, bool recenter) {
    ExOpt o = {};
    if (!on) {
        o.mode = EX_OPT_IGNORE;
    } else if (recenter) {
        o.mode = EX_OPT_RECENTER;
    } else {
        o.mode = EX_OPT_PURSUE;
    }
    o.param = 0u;
    return o;
}

static void opt_label (const ExOpt& coast, const ExOpt& mtn, const ExOpt& riv, char* buf, size_t n) {
    std::snprintf(buf, n, "c%u_m%u_r%u",
        static_cast<unsigned>(coast.mode),
        static_cast<unsigned>(mtn.mode),
        static_cast<unsigned>(riv.mode));
}

static bool save_result_ppm (
    const GameArraySimple& map,
    const MapBitOverlay& ov,
    u16 hx,
    u16 hy,
    u16 ux,
    u16 uy,
    cstr path) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tn = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(tn) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0u; i < tn; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (ov.get(x, y) != 0u) {
            MapTerrainValidate::rgb_from_class(map.get_terrain(x, y), &r, &g, &b);
            if (map.get_river(x, y) != 0u) {
                r = 0;
                g = 128;
                b = 255;
            }
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    mark_cross(rgb, w, h, hx, hy, 255, 255, 255);
    mark_pt(rgb, w, h, ux, uy, 255, 255, 0);
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(tn) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    const bool verbose = (argc >= 2 && argv[1] != nullptr && std::atoi(argv[1]) >= 2);
    print_cls_size(sizeof(ExploreNear));
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, EN_IN_TERR, EN_IN_CLIM, EN_IN_RIV)) {
        t_fail("*** FAILED load map\n");
        return 1;
    }
    if (!ensure_dir(EN_OUT_DIR)) {
        t_fail("*** FAILED mkdir %s\n", EN_OUT_DIR);
        return 1;
    }
    u16 sx = EN_SX;
    u16 sy = EN_SY;
    if (!is_spawn_terr(map.get_terrain(sx, sy))) {
        if (!find_land_center(map, sx, sy)) {
            t_fail("*** FAILED find spawn\n");
            return 1;
        }
    }
    u32 pass_n = 0u;
    for (u32 mask = 0u; mask < 8u; ++mask) {
        for (u32 rec = 0u; rec < 2u; ++rec) {
            const bool recenter = (rec != 0u);
            ExOpt coast = mk_opt((mask & 1u) != 0u, recenter);
            ExOpt mtn = mk_opt((mask & 2u) != 0u, recenter);
            ExOpt riv = mk_opt((mask & 4u) != 0u, recenter);
            riv.param = 4u;
            MapBitOverlay ov(map.width(), map.height());
            ExploreNear ai(
                map,
                ov,
                sx,
                sy,
                EN_SIGHT,
                0u,
                WN_BIAS_NONE,
                coast,
                mtn,
                riv);
            double* move_us = new double[EN_MOVE_CAP];
            u32 move_n = 0u;
            if (move_us == nullptr) {
                t_fail("*** FAILED alloc move timing\n");
                return 1;
            }
            VisOverlay vis(map.width(), map.height());
            vis.inc(sx, sy);
            u16 turn = 0u;
            while (turn < EN_TURNS && !ai.done()) {
                timespec t0 = {};
                timespec t1 = {};
                clock_gettime(CLOCK_MONOTONIC, &t0);
                ai.move(1u);
                clock_gettime(CLOCK_MONOTONIC, &t1);
                record_move(move_us, EN_MOVE_CAP, move_n, t0, t1);
                vis.inc(ai.x(), ai.y());
                ++turn;
            }
            char tag[32];
            char path[160];
            opt_label(coast, mtn, riv, tag, sizeof(tag));
            std::snprintf(path, sizeof(path), "%s/explore_near_%s.ppm", EN_OUT_DIR, tag);
            if (!save_result_ppm(map, ov, sx, sy, ai.x(), ai.y(), path)) {
                t_fail("*** FAILED save %s\n", path);
                delete[] move_us;
                return 1;
            }
            std::printf("[%s]\n", tag);
            std::printf("  turns %u explored %u near %u path %u coast %u mtn %u riv %u rivers %u end (%u,%u) st %u done %u\n",
                static_cast<unsigned>(turn),
                static_cast<unsigned>(cnt_exp(ov)),
                static_cast<unsigned>(ai.near_n()),
                static_cast<unsigned>(ai.path_n()),
                static_cast<unsigned>(ai.coast_n()),
                static_cast<unsigned>(ai.mtn_n()),
                static_cast<unsigned>(ai.riv_n()),
                static_cast<unsigned>(ai.riv_done()),
                static_cast<unsigned>(ai.x()),
                static_cast<unsigned>(ai.y()),
                static_cast<unsigned>(ai.st()),
                static_cast<unsigned>(ai.done() ? 1u : 0u));
            print_move_timing(move_us, move_n);
            print_visit_stats(vis);
            std::printf("\n");
            delete[] move_us;
            ++pass_n;
        }
    }
    if (verbose) {
        std::printf("explore_near: %ux%u sight %u home (%u,%u) runs 16\n",
            static_cast<unsigned>(map.width()),
            static_cast<unsigned>(map.height()),
            static_cast<unsigned>(EN_SIGHT),
            static_cast<unsigned>(sx),
            static_cast<unsigned>(sy));
    }
    std::printf("runs %u output: %s/\n", static_cast<unsigned>(pass_n), EN_OUT_DIR);
    if (verbose) {
        t_pass("*** PASSED explore_near\n");
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
