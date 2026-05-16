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
#include "generate_terrain_cont_outline.h"

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

static i32 run_var_radial_series (u32 seed) {
    char out_dir[256];
    std::snprintf(out_dir, sizeof(out_dir), "../../../simple-map-gen/var-radial-outline/seed-%u", seed);
    if (!mkdirs(out_dir)) {
        std::printf("mkdir %s failed\n", out_dir);
        return -1;
    }
    const int n = 100;
    const f32 v0 = 0.00f;
    const f32 v1 = 0.99f;
    for (int i = 0; i < n; ++i) {
        TerrainContOutlineParams params;
        params.m_seed = seed;
        params.m_width = 1000;
        params.m_height = 1000;
        params.m_fill_mode = TERR_OUTLINE_FILL_MODE_PERLIN_NOISE;
        params.m_pn_params.m_inner_grad_limit = v0 + (v1 - v0) * static_cast<f32>(i) / static_cast<f32>(n - 1);
        Generate_TerrainContOutline gen(params);
        if (!gen.generate() || !gen.is_valid()) {
            std::printf("frame %d inner_grad_limit=%.4f failed\n", i, params.m_pn_params.m_inner_grad_limit);
            return -1;
        }
        char path[384];
        std::snprintf(path, sizeof(path), "%s/frame_%03d.ppm", out_dir, i);
        if (!gen.save_output(path)) {
            std::printf("save %s failed\n", path);
            return -1;
        }
    }
    std::printf("wrote %d frames to %s (m_inner_grad_limit %.4f .. %.4f)\n", n, out_dir, v0, v1);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    u32 seed = 0;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    return run_var_radial_series(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
