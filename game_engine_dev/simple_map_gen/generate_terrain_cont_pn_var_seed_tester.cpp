//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <sys/types.h>

#include "game_primitives.h"
#include "generate_terrain_cont_pn.h"

//================================================================================================================================
//=> - Test helpers -
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

static i32 clamp_freq_arg (i32 argc, char* argv[]) {
    i32 f = 20;
    if (argc >= 2) {
        f = static_cast<i32>(std::strtol(argv[1], nullptr, 10));
    }
    if (f < 1) {
        f = 1;
    }
    if (f > 100) {
        f = 100;
    }
    return f;
}

static f32 freq_arg_to_layer_freq_base (i32 freq_idx) {
    const int n = 100;
    const f32 v0 = 0.05f;
    const f32 v1 = 2.00f;
    const int i = freq_idx - 1;
    return v0 + (v1 - v0) * static_cast<f32>(i) / static_cast<f32>(n - 1);
}

static i32 run_var_seed_series (i32 freq_idx) {
    const f32 freq_base = freq_arg_to_layer_freq_base(freq_idx);
    char out_dir[256];
    std::snprintf(out_dir, sizeof(out_dir), "../../../simple-map-gen/var-seed/freq-%d", freq_idx);
    if (!mkdirs(out_dir)) {
        std::printf("mkdir %s failed\n", out_dir);
        return -1;
    }
    const u32 seed_last = 100u;
    for (u32 seed = 0u; seed <= seed_last; ++seed) {
        TerrainContPnParams params;
        params.m_seed = seed;
        params.m_layer_freq_base = freq_base;
        Generate_TerrainContPn gen(params);
        if (!gen.is_valid()) {
            std::printf("seed %u failed\n", seed);
            return -1;
        }
        char path[384];
        std::snprintf(path, sizeof(path), "%s/frame_%03u.ppm", out_dir, seed);
        if (!gen.save_terrain_rgb(path)) {
            std::printf("save %s failed\n", path);
            return -1;
        }
    }
    std::printf(
        "wrote %u frames to %s (freq_idx=%d m_layer_freq_base=%.4f, seeds 0..%u)\n",
        seed_last + 1u,
        out_dir,
        freq_idx,
        freq_base,
        seed_last);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    const i32 freq_idx = clamp_freq_arg(argc, argv);
    return run_var_seed_series(freq_idx);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
