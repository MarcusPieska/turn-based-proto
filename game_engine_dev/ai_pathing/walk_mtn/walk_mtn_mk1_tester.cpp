//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstdarg>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "walk_mtn_mk1.h"
#include "factory_game_array_simple.h"
#include "map_bit_overlay.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr WM_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr WM_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr WM_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr WM_OUT = "/home/w/Projects/simple-map-gen/walk_mtn_mk1_result.ppm";
static const cstr WM_FRAMES = "/home/w/Projects/simple-map-gen/walk_mtn_mk1_frames";
static const u16 WM_TURNS = PATH_MP_TURN;
static const u16 WM_SIGHT = 3u;
static const u16 WM_UNIT2_TURN = 21u;
static const u32 WM_MOVE_CAP = static_cast<u32>(WM_TURNS) * 2u;

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

static void print_move_timing (const double* us, u32 n, u16 turns) {
    std::printf("turns played %u moves timed %u\n",
        static_cast<unsigned>(turns), static_cast<unsigned>(n));
    if (n == 0u) {
        return;
    }
    double sum = 0.0;
    for (u32 i = 0u; i < n; ++i) {
        sum += us[i];
    }
    const double avg = sum / static_cast<double>(n);
    std::printf("move avg %.2f us\n", avg);
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
    std::printf("slowest %u moves (us):\n", show);
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
        std::printf("  %u: %.2f\n", s + 1u, v);
    }
    delete[] cp;
}


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

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool find_spawn (
    const GameArraySimple& map,
    MapBitOverlay& exp,
    u16 sight,
    u16& ox,
    u16& oy) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u16 cx = w / 2u;
    const u16 cy = h / 2u;
    u16 best_st = 0u;
    u16 best_x = 0u;
    u16 best_y = 0u;
    u32 best_d2 = 0xffffffffu;
    for (u16 y = 0u; y < h; ++y) {
        for (u16 x = 0u; x < w; ++x) {
            const u16 st = WalkMtnMk1::can_ai_start_from(map, exp, x, y, sight);
            if (st == 0u) {
                continue;
            }
            if (st == 1u) {
                ox = x;
                oy = y;
                return true;
            }
            const i32 dx = static_cast<i32>(x) - static_cast<i32>(cx);
            const i32 dy = static_cast<i32>(y) - static_cast<i32>(cy);
            const u32 d2 = static_cast<u32>(dx * dx + dy * dy);
            if (best_st == 0u || d2 < best_d2) {
                best_st = st;
                best_x = x;
                best_y = y;
                best_d2 = d2;
            }
        }
    }
    if (best_st == 0u) {
        return false;
    }
    ox = best_x;
    oy = best_y;
    return true;
}

static bool ensure_dir (cstr path) {
    if (mkdir(path, 0755) == 0) {
        return true;
    }
    return errno == EEXIST;
}

static bool load_terr_rgb (cstr path, u16 ew, u16 eh, u8** out_rgb) {
    u16 w = 0;
    u16 h = 0;
    u8* rgb = nullptr;
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
    if (c == EOF || wi == 0u || hi == 0u || wi > 65535u || hi > 65535u) {
        std::fclose(fp);
        return false;
    }
    w = static_cast<u16>(wi);
    h = static_cast<u16>(hi);
    if (w != ew || h != eh) {
        std::fclose(fp);
        return false;
    }
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    rgb = new u8[nbytes];
    if (rgb == nullptr) {
        std::fclose(fp);
        return false;
    }
    if (std::fread(rgb, 1, nbytes, fp) != nbytes) {
        delete[] rgb;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    *out_rgb = rgb;
    return true;
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

static void frame_path (char* path, size_t n, cstr dir, u16 turn) {
    std::snprintf(path, n, "%s/%05u.ppm", dir, static_cast<unsigned>(turn));
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

static void grad_rgb (u8 d, u8 mx, u8& r, u8& g, u8& b) {
    if (d == 0u) {
        r = 255;
        g = 0;
        b = 0;
        return;
    }
    const u32 span = (mx > 0u) ? static_cast<u32>(mx) : 1u;
    const u32 t = (static_cast<u32>(d) * 255u) / span;
    r = static_cast<u8>(255u - t);
    g = static_cast<u8>(220u - (t * 3u) / 4u);
    b = static_cast<u8>(t);
}

static void mark_grad (u8* rgb, u16 w, u16 h, const WalkMtnMk1& ai) {
    if (ai.phase() != 1u) {
        return;
    }
    const u8 mx = ai.grad_max();
    u16 gi = 0u;
    u16 gx = 0u;
    u16 gy = 0u;
    u8 d = 0u;
    while (ai.grad_tile(gi, gx, gy, d)) {
        u8 r = 0u;
        u8 g = 0u;
        u8 b = 0u;
        grad_rgb(d, mx, r, g, b);
        mark_pt(rgb, w, h, gx, gy, r, g, b);
        ++gi;
    }
}

static void mark_unit (u8* rgb, u16 w, u16 h, const WalkMtnMk1& ai, u8 id) {
    if (id == 0u) {
        if (ai.phase() == 0u) {
            mark_pt(rgb, w, h, ai.x(), ai.y(), 255, 255, 0);
        } else {
            mark_pt(rgb, w, h, ai.x(), ai.y(), 0, 255, 255);
        }
        return;
    }
    if (ai.phase() == 0u) {
        mark_pt(rgb, w, h, ai.x(), ai.y(), 255, 128, 0);
    } else {
        mark_pt(rgb, w, h, ai.x(), ai.y(), 255, 0, 255);
    }
}

static bool save_frame_ppm (
    const u8* terr_rgb,
    const MapBitOverlay& ov,
    u16 w,
    u16 h,
    u16 sx0,
    u16 sy0,
    u16 sx1,
    u16 sy1,
    bool u1_on,
    const WalkMtnMk1& ai0,
    const WalkMtnMk1* ai1,
    cstr path) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0u; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (ov.get(x, y) != 0u) {
            r = terr_rgb[i * 3u + 0];
            g = terr_rgb[i * 3u + 1];
            b = terr_rgb[i * 3u + 2];
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    mark_grad(rgb, w, h, ai0);
    if (ai1 != nullptr) {
        mark_grad(rgb, w, h, *ai1);
    }
    mark_cross(rgb, w, h, sx0, sy0, 255, 255, 255);
    if (u1_on) {
        mark_cross(rgb, w, h, sx1, sy1, 0, 255, 0);
    }
    mark_unit(rgb, w, h, ai0, 0u);
    if (ai1 != nullptr) {
        mark_unit(rgb, w, h, *ai1, 1u);
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

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    const bool verbose = (argc >= 2 && argv[1] != nullptr && std::atoi(argv[1]) >= 2);
    print_cls_size(sizeof(WalkMtnMk1));
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, WM_IN_TERR, WM_IN_CLIM, WM_IN_RIV)) {
        t_fail("*** FAILED load map\n");
        return 1;
    }
    if (!ensure_dir(WM_FRAMES)) {
        t_fail("*** FAILED mkdir %s\n", WM_FRAMES);
        return 1;
    }
    MapBitOverlay ov(map.width(), map.height());
    u16 ux = 0u;
    u16 uy = 0u;
    if (!find_spawn(map, ov, WM_SIGHT, ux, uy)) {
        t_fail("*** FAILED find mtn spawn\n");
        return 1;
    }
    u8* terr_rgb = nullptr;
    if (!load_terr_rgb(WM_IN_TERR, map.width(), map.height(), &terr_rgb)) {
        t_fail("*** FAILED load terrain rgb\n");
        return 1;
    }
    double* move_us = new double[WM_MOVE_CAP];
    if (move_us == nullptr) {
        t_fail("*** FAILED alloc move_us\n");
        delete[] terr_rgb;
        return 1;
    }
    u32 move_n = 0u;
    const u16 map_w = map.width();
    const u16 map_h = map.height();
    WalkMtnMk1 ai0(map, ov, ux, uy, WM_SIGHT, 0u);
    WalkMtnMk1* ai1 = nullptr;
    const u16 spawn_x = ux;
    const u16 spawn_y = uy;
    char path[160];
    frame_path(path, sizeof(path), WM_FRAMES, 0u);
    if (!save_frame_ppm(terr_rgb, ov, map_w, map_h, spawn_x, spawn_y, spawn_x, spawn_y, false, ai0, nullptr, path)) {
        t_fail("*** FAILED save frame %u\n", 0u);
        delete[] terr_rgb;
        return 1;
    }
    if (verbose) {
        std::printf("walk_mtn_mk1: %ux%u sight %u spawn (%u,%u)\n",
            static_cast<unsigned>(map.width()),
            static_cast<unsigned>(map.height()),
            static_cast<unsigned>(WM_SIGHT),
            static_cast<unsigned>(spawn_x),
            static_cast<unsigned>(spawn_y));
    }
    u16 turn = 0u;
    u32 steps0 = 0u;
    u32 steps1 = 0u;
    while (turn < WM_TURNS) {
        if (ai0.done() && (ai1 == nullptr || ai1->done())) {
            break;
        }
        ++turn;
        if (turn == WM_UNIT2_TURN && ai1 == nullptr) {
            ai1 = new WalkMtnMk1(map, ov, spawn_x, spawn_y, WM_SIGHT, 1u);
            if (verbose) {
                std::printf("  turn %u spawn unit 1 at (%u,%u)\n",
                    static_cast<unsigned>(turn),
                    static_cast<unsigned>(spawn_x),
                    static_cast<unsigned>(spawn_y));
            }
        }
        if (!ai0.done()) {
            const u16 ox = ai0.x();
            const u16 oy = ai0.y();
            timespec t0;
            timespec t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            ai0.move(1u);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            record_move(move_us, WM_MOVE_CAP, move_n, t0, t1);
            if (ai0.x() != ox || ai0.y() != oy) {
                ++steps0;
            }
        }
        if (ai1 != nullptr && !ai1->done()) {
            const u16 ox = ai1->x();
            const u16 oy = ai1->y();
            timespec t0;
            timespec t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            ai1->move(1u);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            record_move(move_us, WM_MOVE_CAP, move_n, t0, t1);
            if (ai1->x() != ox || ai1->y() != oy) {
                ++steps1;
            }
        }
        frame_path(path, sizeof(path), WM_FRAMES, turn);
        if (!save_frame_ppm(terr_rgb, ov, map_w, map_h, spawn_x, spawn_y, spawn_x, spawn_y, ai1 != nullptr, ai0, ai1, path)) {
            t_fail("*** FAILED save frame %u\n", static_cast<unsigned>(turn));
            delete ai1;
            delete[] move_us;
            delete[] terr_rgb;
            return 1;
        }
        if (verbose && (turn == 1u || (turn % 100u) == 0u)) {
            std::printf("  turn %u explored %u u0 (%u,%u) ph %u done %u",
                static_cast<unsigned>(turn),
                static_cast<unsigned>(cnt_exp(ov)),
                static_cast<unsigned>(ai0.x()),
                static_cast<unsigned>(ai0.y()),
                static_cast<unsigned>(ai0.phase()),
                static_cast<unsigned>(ai0.done() ? 1u : 0u));
            if (ai1 != nullptr) {
                std::printf(" u1 (%u,%u) ph %u done %u",
                    static_cast<unsigned>(ai1->x()),
                    static_cast<unsigned>(ai1->y()),
                    static_cast<unsigned>(ai1->phase()),
                    static_cast<unsigned>(ai1->done() ? 1u : 0u));
            }
            std::printf("\n");
        }
    }
    if (!save_frame_ppm(terr_rgb, ov, map_w, map_h, spawn_x, spawn_y, spawn_x, spawn_y, ai1 != nullptr, ai0, ai1, WM_OUT)) {
        t_fail("*** FAILED save %s\n", WM_OUT);
        delete ai1;
        delete[] move_us;
        delete[] terr_rgb;
        return 1;
    }
    std::printf("turns %u steps0 %u steps1 %u frames %u explored %u\n",
        static_cast<unsigned>(turn),
        static_cast<unsigned>(steps0),
        static_cast<unsigned>(steps1),
        static_cast<unsigned>(turn),
        static_cast<unsigned>(cnt_exp(ov)));
    print_move_timing(move_us, move_n, turn);
    std::printf("u0 end (%u,%u) phase %u done %u loc_exh %u\n",
        static_cast<unsigned>(ai0.x()),
        static_cast<unsigned>(ai0.y()),
        static_cast<unsigned>(ai0.phase()),
        static_cast<unsigned>(ai0.done() ? 1u : 0u),
        static_cast<unsigned>(ai0.loc_exh() ? 1u : 0u));
    if (ai1 != nullptr) {
        std::printf("u1 end (%u,%u) phase %u done %u loc_exh %u\n",
            static_cast<unsigned>(ai1->x()),
            static_cast<unsigned>(ai1->y()),
            static_cast<unsigned>(ai1->phase()),
            static_cast<unsigned>(ai1->done() ? 1u : 0u),
            static_cast<unsigned>(ai1->loc_exh() ? 1u : 0u));
    }
    std::printf("frames: %s\n", WM_FRAMES);
    std::printf("result: %s\n", WM_OUT);
    if (verbose) {
        t_pass("*** PASSED walk_mtn_mk1\n");
    }
    delete ai1;
    delete[] move_us;
    delete[] terr_rgb;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
