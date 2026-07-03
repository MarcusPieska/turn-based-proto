//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cerrno>

#include <sys/stat.h>
#include <sys/types.h>

#include "river_explore_mk2.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "map_terrain_validate.h"
#include "runtime_trace_dbg.h"
#include "test_support.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr RE_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr RE_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr RE_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr RE_OUT = "/home/w/Projects/simple-map-gen/river_explore_mk2_result.ppm";
static const cstr RE_SYS_MAP = "/home/w/Projects/simple-map-gen/river_explore_mk2_sysmap.ppm";
static const cstr RE_BAD_MAP = "/home/w/Projects/simple-map-gen/river_explore_mk2_badmap.ppm";
static const cstr RE_DISC_MAP = "/home/w/Projects/simple-map-gen/river_explore_mk2_disc.ppm";
static const cstr RE_FRAMES = "/home/w/Projects/simple-map-gen/river_explore_mk2_frames";
static const cstr RE_TRACE = "river_explore_mk2_test.trace";
static const u16 RE_TURNS = PATH_MP_TURN;
static const u16 RE_MOVES = 3u;
static const u16 RE_SIGHT = 3u;
static const u8 RE_PH_DONE = 3u;
static const u16 RE_PROG_EVERY = 50u;
static const u32 RE_SYS_MAX = 1000u;
static const u32 RE_MK_MAX = 8192u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool re_verbose (int argc, char** argv) {
    if (argc < 2 || argv[1] == nullptr) {
        return false;
    }
    return std::atoi(argv[1]) >= 2;
}

static bool re_sys_frames (int argc, char** argv) {
    if (argc < 2 || argv[1] == nullptr) {
        return false;
    }
    return std::atoi(argv[1]) >= 3;
}

static bool ensure_dir (cstr path) {
    if (mkdir(path, 0755) == 0) {
        return true;
    }
    return errno == EEXIST;
}

static double clk_ms (clock_t t0, clock_t t1) {
    return static_cast<double>(t1 - t0) * 1000.0 / static_cast<double>(CLOCKS_PER_SEC);
}

static const char* ph_name (u8 ph) {
    if (ph == 0) {
        return "off";
    }
    if (ph == 1) {
        return "greedy";
    }
    if (ph == 2) {
        return "back";
    }
    if (ph == 3) {
        return "done";
    }
    return "?";
}

static void print_sys_prog (
    bool verbose,
    u32 sys_i,
    u16 sys_turn,
    const RiverExploreMk2& ai,
    u32 riv_done,
    u32 riv_tot) {
    if (!verbose) {
        return;
    }
    std::printf("  system %u turn %u pos (%u,%u) phase %s river %u/%u\n",
        static_cast<unsigned>(sys_i),
        static_cast<unsigned>(sys_turn),
        static_cast<unsigned>(ai.x()),
        static_cast<unsigned>(ai.y()),
        ph_name(ai.phase()),
        static_cast<unsigned>(riv_done),
        static_cast<unsigned>(riv_tot));
    std::fflush(stdout);
}

static u32 cnt_explored (const MapBitOverlay& ov) {
    u32 n = 0;
    for (u16 y = 0; y < ov.height(); ++y) {
        for (u16 x = 0; x < ov.width(); ++x) {
            n += ov.get(x, y);
        }
    }
    return n;
}

class VisOverlay {
public:
    explicit VisOverlay (u16 w, u16 h) :
        m_w(w),
        m_h(h),
        m_v(nullptr) {
        const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
        m_v = new u32[n];
        for (u32 i = 0; i < n; ++i) {
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
        for (u32 i = 0; i < n; ++i) {
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

static void mark_pt (u8* rgb, u16 w, u16 h, u16 px, u16 py, u8 r, u8 g, u8 b) { 
    if (px >= w || py >= h) {
        return;
    }
    const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
    rgb[i * 3u + 0] = r;
    rgb[i * 3u + 1] = g;
    rgb[i * 3u + 2] = b;
}

static void pull_marks (
    const RiverExploreMk2& ai,
    u16* ge_x,
    u16* ge_y,
    u32& ge_n,
    u16* gs_x,
    u16* gs_y,
    u32& gs_n) {
    for (u16 i = 0; i < ai.gre_end_n(); ++i) {
        if (ge_n < RE_MK_MAX) {
            ge_x[ge_n] = ai.gre_end_x(i);
            ge_y[ge_n] = ai.gre_end_y(i);
            ++ge_n;
        }
    }
    for (u16 i = 0; i < ai.gre_start_n(); ++i) {
        if (gs_n < RE_MK_MAX) {
            gs_x[gs_n] = ai.gre_start_x(i);
            gs_y[gs_n] = ai.gre_start_y(i);
            ++gs_n;
        }
    }
}

static bool save_result_ppm (
    const GameArraySimple& map,
    const ExploredRiverOverlay& riv_exp,
    const u16* sys_x,
    const u16* sys_y,
    u32 sys_n,
    const u16* ge_x,
    const u16* ge_y,
    u32 ge_n,
    const u16* gs_x,
    const u16* gs_y,
    u32 gs_n,
    cstr path) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (map.get_river(x, y) != 0u) {
            r = 0;
            g = 0;
            b = 255;
            if (riv_exp.get(x, y) != 0u) {
                r = 255;
                g = 255;
                b = 255;
            }
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    for (u32 i = 0; i < ge_n; ++i) {
        mark_pt(rgb, w, h, ge_x[i], ge_y[i], 128, 128, 128);
    }
    for (u32 i = 0; i < gs_n; ++i) {
        mark_pt(rgb, w, h, gs_x[i], gs_y[i], 0, 255, 0);
    }
    for (u32 si = 0; si < sys_n; ++si) {
        mark_pt(rgb, w, h, sys_x[si], sys_y[si], 255, 0, 0);
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

static bool save_disc_ppm (
    const GameArraySimple& map,
    const MapBitOverlay& exp,
    cstr path) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (exp.get(x, y) != 0u) {
            MapTerrainValidate::rgb_from_class(map.get_terrain(x, y), &r, &g, &b);
            if (map.get_river(x, y) != 0u) {
                r = 0;
                g = 0;
                b = 255;
            }
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

static bool save_sys_frame (
    const GameArraySimple& map,
    const ExploredRiverOverlay& riv_exp,
    const u16* sys_x,
    const u16* sys_y,
    u32 sys_n,
    const u16* ge_x,
    const u16* ge_y,
    u32 ge_n,
    const u16* gs_x,
    const u16* gs_y,
    u32 gs_n,
    u32 sys_i) {
    char path[128];
    const int n = std::snprintf(path, sizeof(path), "%s/%04u.ppm",
        RE_FRAMES, static_cast<unsigned>(sys_i));
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(path)) {
        return false;
    }
    return save_result_ppm(map, riv_exp, sys_x, sys_y, sys_n,
        ge_x, ge_y, ge_n, gs_x, gs_y, gs_n, path);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    const bool verbose = re_verbose(argc, argv);
    const bool sys_frames = re_sys_frames(argc, argv);
    GameArraySimple map;
    if (!Factory_GameArraySimple::load(&map, RE_IN_TERR, RE_IN_CLIM, RE_IN_RIV)) {
        std::printf("*** FAILED load map\n");
        return 1;
    }
    TRACE_SETUP((RE_TRACE));
    if (sys_frames && !ensure_dir(RE_FRAMES)) {
        std::printf("*** FAILED mkdir %s\n", RE_FRAMES);
        return 1;
    }
    ExploredRiverOverlay riv_exp(map.width(), map.height());
    RiverSystemFinder finder(map, riv_exp);
    MapBitOverlay exp(map.width(), map.height());
    VisOverlay vis(map.width(), map.height());
    const clock_t t0 = clock();
    u16 ux = 0;
    u16 uy = 0;
    u16 sys_x[RE_SYS_MAX];
    u16 sys_y[RE_SYS_MAX];
    double sys_ms[RE_SYS_MAX];
    u32 sys_n = 0;
    u16 turn = 0;
    u16 ge_x[RE_MK_MAX];
    u16 ge_y[RE_MK_MAX];
    u16 gs_x[RE_MK_MAX];
    u16 gs_y[RE_MK_MAX];
    u32 ge_n = 0;
    u32 gs_n = 0;
    if (verbose) {
        std::printf("river_explore_mk2: %ux%u turns %u moves %u sight %u\n",
            static_cast<unsigned>(map.width()),
            static_cast<unsigned>(map.height()),
            static_cast<unsigned>(RE_TURNS),
            static_cast<unsigned>(RE_MOVES),
            static_cast<unsigned>(RE_SIGHT));
        std::printf("river tiles %u sys_max %u\n",
            static_cast<unsigned>(finder.riv_tot()),
            static_cast<unsigned>(RE_SYS_MAX));
    }
    while (sys_n < RE_SYS_MAX && finder.find_next(ux, uy)) {
        sys_x[sys_n] = ux;
        sys_y[sys_n] = uy;
        const u32 sys_i = sys_n + 1u;
        const u32 riv_bef = riv_exp.cnt();
        const u32 riv_tot = finder.riv_tot();
        if (verbose) {
            std::printf("--- system %u ---\n", static_cast<unsigned>(sys_i));
            std::printf("  entry (%u,%u) river %u/%u\n",
                static_cast<unsigned>(ux),
                static_cast<unsigned>(uy),
                static_cast<unsigned>(riv_bef),
                static_cast<unsigned>(riv_tot));
            std::fflush(stdout);
        }
        const clock_t sys_t0 = clock();
        RiverExploreMk2 ai(map, exp, ux, uy, RE_SIGHT, 0u);
        exp.set(ux, uy);
        vis.inc(ux, uy);
        TRACE_EXPLORE_DISCOVER((ux, uy, 0u));
        u16 sys_turn = 0;
        u8 last_ph = ai.phase();
        for (u16 t = 0; t < RE_TURNS && ai.phase() != RE_PH_DONE; ++t) {
            ++turn;
            ++sys_turn;
            TRACE_NEW_TURN((turn));
            for (u16 m = 0; m < RE_MOVES && ai.phase() != RE_PH_DONE; ++m) {
                ai.move(1u);
                vis.inc(ai.x(), ai.y());
            }
            const u8 ph = ai.phase();
            const bool ph_chg = ph != last_ph;
            last_ph = ph;
            if (verbose && (sys_turn == 1u || ph_chg
                || (sys_turn % RE_PROG_EVERY) == 0u)) {
                print_sys_prog(verbose, sys_i, sys_turn, ai, riv_exp.cnt(), riv_tot);
            }
        }
        sys_ms[sys_n] = clk_ms(sys_t0, clock());
        riv_exp.pull(map, exp);
        pull_marks(ai, ge_x, ge_y, ge_n, gs_x, gs_y, gs_n);
        const u32 riv_aft = riv_exp.cnt();
        if (verbose) {
            std::printf("  done turns %u end (%u,%u) phase %s river +%u total %u/%u\n",
                static_cast<unsigned>(sys_turn),
                static_cast<unsigned>(ai.x()),
                static_cast<unsigned>(ai.y()),
                ph_name(ai.phase()),
                static_cast<unsigned>(riv_aft - riv_bef),
                static_cast<unsigned>(riv_aft),
                static_cast<unsigned>(riv_tot));
            std::fflush(stdout);
        }
        ++sys_n;
        if (sys_frames && !save_sys_frame(map, riv_exp, sys_x, sys_y, sys_n,
                ge_x, ge_y, ge_n, gs_x, gs_y, gs_n, sys_i)) {
            std::printf("*** FAILED save frame %u\n", static_cast<unsigned>(sys_i));
            return 1;
        }
    }
    const clock_t t1 = clock();
    const double run_sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (sys_n == 0u) {
        std::printf("*** FAILED find river start\n");
        return 1;
    }
    if (!save_result_ppm(map, riv_exp, sys_x, sys_y, sys_n,
            ge_x, ge_y, ge_n, gs_x, gs_y, gs_n, RE_OUT)) {
        std::printf("*** FAILED save %s\n", RE_OUT);
        return 1;
    }
    if (!save_disc_ppm(map, exp, RE_DISC_MAP)) {
        std::printf("*** FAILED save %s\n", RE_DISC_MAP);
        return 1;
    }
    for (u32 si = 0; si < sys_n; ++si) {
        std::printf("system %u: %.1f ms\n",
            static_cast<unsigned>(si + 1u), sys_ms[si]);
    }
    double sys_tot = 0.0;
    double sys_min = sys_ms[0];
    double sys_max = sys_ms[0];
    for (u32 si = 0; si < sys_n; ++si) {
        sys_tot += sys_ms[si];
        if (sys_ms[si] < sys_min) {
            sys_min = sys_ms[si];
        }
        if (sys_ms[si] > sys_max) {
            sys_max = sys_ms[si];
        }
    }
    const double sys_avg = sys_tot / static_cast<double>(sys_n);
    std::printf("time summary: %u systems total %.1f ms avg %.1f ms min %.1f ms max %.1f ms\n",
        static_cast<unsigned>(sys_n),
        sys_tot, sys_avg, sys_min, sys_max);
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
    if (!re_riv_audit_run(RE_OUT, RE_SYS_MAP, RE_BAD_MAP)) {
        return 1;
    }
    if (verbose) {
        std::printf("systems %u/%u river explored %u/%u map explored %u gre_end %u gre_start %u\n",
            static_cast<unsigned>(sys_n),
            static_cast<unsigned>(RE_SYS_MAX),
            static_cast<unsigned>(riv_exp.cnt()),
            static_cast<unsigned>(finder.riv_tot()),
            cnt_explored(exp),
            static_cast<unsigned>(ge_n),
            static_cast<unsigned>(gs_n));
        std::printf("run time: %.6f s\n", run_sec);
        std::printf("result: %s\n", RE_OUT);
        std::printf("disc: %s\n", RE_DISC_MAP);
        if (sys_frames) {
            std::printf("frames: %s\n", RE_FRAMES);
        }
        std::printf("trace: %s\n", RE_TRACE);
        std::printf("*** PASSED river_explore_mk2\n");
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
