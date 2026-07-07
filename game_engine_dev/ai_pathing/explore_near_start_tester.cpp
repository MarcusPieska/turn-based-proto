//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <chrono>

#include "explore_near.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "game_primitives.h"
#include "map_bit_overlay.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr EN_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr EN_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr EN_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr EN_START_MAP = "/home/w/Projects/simple-map-gen/explore_near_start_map.ppm";
static const cstr EN_OUT = "/home/w/Projects/simple-map-gen/explore_near_start_result.ppm";
static const u16 EN_TURNS = PATH_MP_TURN;
static const u16 EN_SIGHT = 3u;

static const u8 k_ov_off = 0u;
static const u8 k_ov_land = 1u;
static const u8 k_ov_near = 2u;
static const u8 k_ov_side = 3u;

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

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool is_land (u8 t) {
    if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0]) {
        return false;
    }
    return true;
}

static bool find_center_land (const GameArraySimple& map, u16& sx, u16& sy) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u16 cx = w / 2u;
    const u16 cy = h / 2u;
    if (is_land(map.get_terrain(cx, cy))) {
        sx = cx;
        sy = cy;
        return true;
    }
    const u16 max_d = (w > h) ? w : h;
    for (u16 d = 1u; d <= max_d; ++d) {
        for (i16 dy = -static_cast<i16>(d); dy <= static_cast<i16>(d); ++dy) {
            for (i16 dx = -static_cast<i16>(d); dx <= static_cast<i16>(d); ++dx) {
                if ((dx < 0 ? -dx : dx) != static_cast<i32>(d) && (dy < 0 ? -dy : dy) != static_cast<i32>(d)) {
                    continue;
                }
                const i32 xi = static_cast<i32>(cx) + dx;
                const i32 yi = static_cast<i32>(cy) + dy;
                if (xi < 0 || yi < 0) {
                    continue;
                }
                const u16 x = static_cast<u16>(xi);
                const u16 y = static_cast<u16>(yi);
                if (x >= w || y >= h) {
                    continue;
                }
                if (is_land(map.get_terrain(x, y))) {
                    sx = x;
                    sy = y;
                    return true;
                }
            }
        }
    }
    return false;
}

static bool flood_main_land (const GameArraySimple& map, u16 sx, u16 sy, u8* ov) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tile_n = map.tile_n();
    u32* q = new u32[tile_n];
    if (q == nullptr) {
        return false;
    }
    for (u32 i = 0u; i < tile_n; ++i) {
        ov[i] = k_ov_off;
    }
    const u32 si = tidx(w, sx, sy);
    ov[si] = k_ov_land;
    u32 qn = 0u;
    q[qn++] = si;
    for (u32 qh = 0u; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (u32 k = 0u; k < MAP_NBR4_N; ++k) {
            const i32 nx = static_cast<i32>(px) + MAP_NBR4_DX[k];
            const i32 ny = static_cast<i32>(py) + MAP_NBR4_DY[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 cx = static_cast<u16>(nx);
            const u16 cy = static_cast<u16>(ny);
            if (cx >= w || cy >= h || !is_land(map.get_terrain(cx, cy))) {
                continue;
            }
            const u32 ni = tidx(w, cx, cy);
            if (ov[ni] != k_ov_off) {
                continue;
            }
            ov[ni] = k_ov_land;
            q[qn++] = ni;
        }
    }
    delete[] q;
    return qn > 0u;
}

static void prof_print (u64* us, u32 n) {
    if (n == 0u) {
        std::printf("can_ai_start_from: samples 0\n");
        return;
    }
    u64 sum = 0u;
    for (u32 i = 0u; i < n; ++i) {
        sum += us[i];
    }
    const u64 avg = sum / static_cast<u64>(n);
    std::printf("can_ai_start_from: samples %u avg %llu us\n",
        static_cast<unsigned>(n),
        static_cast<unsigned long long>(avg));
    const u32 wn = (n < 10u) ? n : 10u;
    u64* cp = new u64[n];
    if (cp == nullptr) {
        return;
    }
    for (u32 i = 0u; i < n; ++i) {
        cp[i] = us[i];
    }
    for (u32 s = 0u; s < wn; ++s) {
        u32 mx = s;
        for (u32 i = s + 1u; i < n; ++i) {
            if (cp[i] > cp[mx]) {
                mx = i;
            }
        }
        const u64 tmp = cp[s];
        cp[s] = cp[mx];
        cp[mx] = tmp;
    }
    std::printf("  worst %u:", static_cast<unsigned>(wn));
    for (u32 i = 0u; i < wn; ++i) {
        std::printf(" %llu", static_cast<unsigned long long>(cp[i]));
    }
    std::printf(" us\n");
    delete[] cp;
}

static void classify_starts (
    const GameArraySimple& map,
    MapBitOverlay& ov,
    u16 sight,
    const ExOpt& coast,
    const ExOpt& mtn,
    const ExOpt& riv,
    u8* cls,
    u32& n_near,
    u32& n_side,
    u32& n_land,
    u64* prof_us,
    u32& prof_n) {
    const u16 w = map.width();
    const u16 h = map.height();
    n_near = 0u;
    n_side = 0u;
    n_land = 0u;
    for (u16 y = 0u; y < h; ++y) {
        for (u16 x = 0u; x < w; ++x) {
            const u32 i = tidx(w, x, y);
            if (cls[i] != k_ov_land) {
                continue;
            }
            const auto t0 = std::chrono::high_resolution_clock::now();
            const u16 st = ExploreNear::can_ai_start_from(map, ov, x, y, sight, coast, mtn, riv);
            const auto t1 = std::chrono::high_resolution_clock::now();
            prof_us[prof_n] = static_cast<u64>(
                std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
            ++prof_n;
            if (st >= 1u && st <= 2u) {
                cls[i] = k_ov_near;
                ++n_near;
            } else if (st >= 10u) {
                cls[i] = k_ov_side;
                ++n_side;
            } else {
                ++n_land;
            }
        }
    }
}

static u8 shade (u8 base, u8 tint, u8 wt) {
    const u32 b = static_cast<u32>(base);
    const u32 t = static_cast<u32>(tint);
    return static_cast<u8>((b * (255u - wt) + t * wt) / 255u);
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
    int wi = 0;
    int hi = 0;
    if (std::fscanf(fp, "%d %d", &wi, &hi) != 2) {
        std::fclose(fp);
        return false;
    }
    int maxv = 0;
    if (std::fscanf(fp, "%d", &maxv) != 1) {
        std::fclose(fp);
        return false;
    }
    c = std::fgetc(fp);
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

static bool save_rgb_ppm (const u8* rgb, u16 w, u16 h, cstr path) {
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

static bool save_start_map (const u8* terr_rgb, const u8* cls, u16 w, u16 h, cstr path) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0u; i < n; ++i) {
        const u8 tr = terr_rgb[i * 3u + 0u];
        const u8 tg = terr_rgb[i * 3u + 1u];
        const u8 tb = terr_rgb[i * 3u + 2u];
        u8 r = tr;
        u8 g = tg;
        u8 b = tb;
        if (cls[i] == k_ov_near) {
            r = shade(tr, 255u, 128u);
            g = shade(tg, 255u, 128u);
            b = shade(tb, 0u, 128u);
        } else if (cls[i] == k_ov_side) {
            r = shade(tr, 255u, 128u);
            g = shade(tg, 0u, 128u);
            b = shade(tb, 0u, 128u);
        }
        rgb[i * 3u + 0u] = r;
        rgb[i * 3u + 1u] = g;
        rgb[i * 3u + 2u] = b;
    }
    const bool ok = save_rgb_ppm(rgb, w, h, path);
    delete[] rgb;
    return ok;
}

static bool find_near_spawn (const u8* cls, u16 w, u16 h, u16& ox, u16& oy) {
    const u16 cx = w / 2u;
    const u16 cy = h / 2u;
    u32 best_d2 = 0xffffffffu;
    bool found = false;
    for (u16 y = 0u; y < h; ++y) {
        for (u16 x = 0u; x < w; ++x) {
            if (cls[tidx(w, x, y)] != k_ov_near) {
                continue;
            }
            const i32 dx = static_cast<i32>(x) - static_cast<i32>(cx);
            const i32 dy = static_cast<i32>(y) - static_cast<i32>(cy);
            const u32 d2 = static_cast<u32>(dx * dx + dy * dy);
            if (!found || d2 < best_d2) {
                found = true;
                best_d2 = d2;
                ox = x;
                oy = y;
            }
        }
    }
    return found;
}

static bool save_frame_ppm (
    const u8* terr_rgb,
    const MapBitOverlay& ov,
    u16 w,
    u16 h,
    u16 sx,
    u16 sy,
    u16 ux,
    u16 uy,
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
    for (i16 dy = -2; dy <= 2; ++dy) {
        for (i16 dx = -2; dx <= 2; ++dx) {
            if ((dx < 0 ? -dx : dx) > 1 && (dy < 0 ? -dy : dy) > 1) {
                continue;
            }
            const i32 xi = static_cast<i32>(sx) + dx;
            const i32 yi = static_cast<i32>(sy) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 x = static_cast<u16>(xi);
            const u16 y = static_cast<u16>(yi);
            if (x >= w || y >= h) {
                continue;
            }
            const u32 i = tidx(w, x, y);
            rgb[i * 3u + 0] = 0;
            rgb[i * 3u + 1] = 255;
            rgb[i * 3u + 2] = 0;
        }
    }
    if (ux < w && uy < h) {
        const u32 ui = tidx(w, ux, uy);
        rgb[ui * 3u + 0] = 255;
        rgb[ui * 3u + 1] = 255;
        rgb[ui * 3u + 2] = 0;
    }
    const bool ok = save_rgb_ppm(rgb, w, h, path);
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
    const u32 tile_n = map.tile_n();
    const u16 map_w = map.width();
    const u16 map_h = map.height();
    u8* cls = new u8[tile_n];
    if (cls == nullptr) {
        t_fail("*** FAILED alloc overlays\n");
        delete[] cls;
        return 1;
    }
    MapBitOverlay ov(map_w, map_h);
    const ExOpt coast = {EX_OPT_IGNORE, 0u};
    const ExOpt mtn = {EX_OPT_IGNORE, 0u};
    const ExOpt riv = {EX_OPT_PURSUE, 4u};
    u16 seed_x = 0u;
    u16 seed_y = 0u;
    if (!find_center_land(map, seed_x, seed_y)) {
        t_fail("*** FAILED find center land\n");
        delete[] cls;
        return 1;
    }
    if (!flood_main_land(map, seed_x, seed_y, cls)) {
        t_fail("*** FAILED flood main landmass\n");
        delete[] cls;
        return 1;
    }
    u32 n_near = 0u;
    u32 n_side = 0u;
    u32 n_land = 0u;
    u64* prof_us = new u64[tile_n];
    u32 prof_n = 0u;
    if (prof_us == nullptr) {
        t_fail("*** FAILED alloc profile\n");
        delete[] cls;
        return 1;
    }
    classify_starts(map, ov, EN_SIGHT, coast, mtn, riv, cls, n_near, n_side, n_land, prof_us, prof_n);
    prof_print(prof_us, prof_n);
    u8* terr_rgb = nullptr;
    if (!load_terr_rgb(EN_IN_TERR, map_w, map_h, &terr_rgb)) {
        t_fail("*** FAILED load terrain rgb\n");
        delete[] prof_us;
        delete[] cls;
        return 1;
    }
    if (!save_start_map(terr_rgb, cls, map_w, map_h, EN_START_MAP)) {
        t_fail("*** FAILED save %s\n", EN_START_MAP);
        delete[] terr_rgb;
        delete[] prof_us;
        delete[] cls;
        return 1;
    }
    u16 spawn_x = 0u;
    u16 spawn_y = 0u;
    if (!find_near_spawn(cls, map_w, map_h, spawn_x, spawn_y)) {
        t_fail("*** FAILED find near spawn\n");
        delete[] terr_rgb;
        delete[] prof_us;
        delete[] cls;
        return 1;
    }
    if (verbose) {
        std::printf("explore_near_start: %ux%u sight %u\n",
            static_cast<unsigned>(map_w),
            static_cast<unsigned>(map_h),
            static_cast<unsigned>(EN_SIGHT));
        std::printf("  landmass seed (%u,%u)\n",
            static_cast<unsigned>(seed_x),
            static_cast<unsigned>(seed_y));
        std::printf("  overlay near %u side %u land %u\n",
            static_cast<unsigned>(n_near),
            static_cast<unsigned>(n_side),
            static_cast<unsigned>(n_land));
        std::printf("  near spawn (%u,%u)\n",
            static_cast<unsigned>(spawn_x),
            static_cast<unsigned>(spawn_y));
    }
    ExploreNear ai(
        map,
        ov,
        spawn_x,
        spawn_y,
        EN_SIGHT,
        0u,
        WN_BIAS_NONE,
        coast,
        mtn,
        riv);
    u16 turn = 0u;
    u32 steps = 0u;
    while (turn < EN_TURNS && !ai.done()) {
        ++turn;
        const u16 ox = ai.x();
        const u16 oy = ai.y();
        ai.move(1u);
        if (ai.x() != ox || ai.y() != oy) {
            ++steps;
        }
    }
    if (!save_frame_ppm(terr_rgb, ov, map_w, map_h, spawn_x, spawn_y, ai.x(), ai.y(), EN_OUT)) {
        t_fail("*** FAILED save %s\n", EN_OUT);
        delete[] terr_rgb;
        delete[] prof_us;
        delete[] cls;
        return 1;
    }
    std::printf("start_map: %s\n", EN_START_MAP);
    std::printf("turns %u steps %u\n",
        static_cast<unsigned>(turn),
        static_cast<unsigned>(steps));
    std::printf("end (%u,%u) st %u done %u riv_done %u\n",
        static_cast<unsigned>(ai.x()),
        static_cast<unsigned>(ai.y()),
        static_cast<unsigned>(ai.st()),
        static_cast<unsigned>(ai.done() ? 1u : 0u),
        static_cast<unsigned>(ai.riv_done()));
    std::printf("result: %s\n", EN_OUT);
    if (verbose) {
        t_pass("*** PASSED explore_near_start\n");
    }
    delete[] terr_rgb;
    delete[] prof_us;
    delete[] cls;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
