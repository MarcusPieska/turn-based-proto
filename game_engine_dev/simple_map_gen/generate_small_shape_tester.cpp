//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

#include <sys/stat.h>
#include <sys/types.h>

#include "game_primitives.h"
#include "generate_small_shape.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* g_out_dir = "/home/w/Projects/simple-map-gen/small-shape";
static const u16 g_want_w = 50;
static const u16 g_want_h = 20;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool mkdirs (cstr path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    char buf[384];
    std::strncpy(buf, path, sizeof(buf) - 1u);
    buf[sizeof(buf) - 1u] = '\0';
    for (char* p = buf + 1; *p != '\0'; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
                return false;
            }
            *p = '/';
        }
    }
    return mkdir(buf, 0755) == 0 || errno == EEXIST;
}

//================================================================================================================================
//=> - Test -
//================================================================================================================================

static i32 run_small_shape_series (u32 seed) {
    if (!mkdirs(g_out_dir)) {
        std::printf("mkdir %s failed\n", g_out_dir);
        return -1;
    }
    double gen_usec = 0.0;
    const i32 frame_n = 36;
    for (i32 deg = 0; deg < 360; deg += 10) {
        SmallShapeParams params = {};
        params.m_seed = seed;
        params.m_width = g_want_w;
        params.m_height = g_want_h;
        params.m_angle_deg = deg;
        Generate_SmallShape gen(params);
        const clock_t t0 = clock();
        const bool ok = gen.generate();
        const clock_t t1 = clock();
        const double usec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC) * 1000000.0;
        gen_usec += usec;
        if (!ok || !gen.is_valid()) {
            std::printf("Generate_SmallShape failed at %d deg\n", deg);
            return -1;
        }
        char path[384];
        std::snprintf(path, sizeof(path), "%s/small-shape-%03d.ppm", g_out_dir, deg);
        if (!gen.save_output(path)) {
            std::printf("failed to save: %s\n", path);
            return -1;
        }
    }
    const u32 out_sz = static_cast<u32>(g_want_w > g_want_h ? g_want_w * 2u : g_want_h * 2u);
    std::printf(
        "Generate_SmallShape average generate time: %.0f us (%u x %u request, canvas %u x %u, %d frames)\n",
        gen_usec / static_cast<double>(frame_n),
        static_cast<u32>(g_want_w),
        static_cast<u32>(g_want_h),
        out_sz,
        out_sz,
        frame_n);
    std::printf("saved: %s/small-shape-000.ppm .. small-shape-350.ppm\n", g_out_dir);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    u32 seed = 42;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    return run_small_shape_series(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
