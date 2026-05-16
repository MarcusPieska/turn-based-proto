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
#include "generate_terrain_combo_min.h"
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

static bool gen_outline (u32 seed, f32 lim, MapArrayTerrain& out) {
    TerrainContOutlineParams params;
    params.m_seed = seed;
    params.m_width = 1000;
    params.m_height = 1000;
    params.m_fill_mode = TERR_OUTLINE_FILL_MODE_PERLIN_NOISE;
    params.m_pn_params.m_inner_grad_limit = lim;
    Generate_TerrainContOutline gen(params);
    if (!gen.generate() || !gen.is_valid()) {
        return false;
    }
    return out.assign_copy(gen.width(), gen.height(), gen.terrain().data());
}

static i32 run_var_seed_series () {
    const f32 grad1 = 0.00f;
    const f32 grad2 = 0.65f;
    char out_dir[256];
    std::snprintf(out_dir, sizeof(out_dir), "../../../simple-map-gen/var-seed-outline/combo-min-%.4f-%.4f", grad1, grad2);
    if (!mkdirs(out_dir)) {
        std::printf("mkdir %s failed\n", out_dir);
        return -1;
    }
    const int n = 100;
    MapArrayTerrain ta;
    MapArrayTerrain tb;
    Generate_TerrainComboMin combo;
    for (int seed = 0; seed < n; ++seed) {
        if (!gen_outline(static_cast<u32>(seed), grad1, ta)) {
            std::printf("seed %d outline %.4f failed\n", seed, grad1);
            return -1;
        }
        if (!gen_outline(static_cast<u32>(seed), grad2, tb)) {
            std::printf("seed %d outline %.4f failed\n", seed, grad2);
            return -1;
        }
        if (!combo.generate(ta, tb) || !combo.is_valid()) {
            std::printf("seed %d combo min failed\n", seed);
            return -1;
        }
        char path[384];
        std::snprintf(path, sizeof(path), "%s/frame_%03d.ppm", out_dir, seed);
        combo.save_output(path);
    }
    std::printf("wrote %d frames to %s (combo-min %.4f + %.4f, seeds 0..%d)\n", n, out_dir, grad1, grad2, n - 1);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return run_var_seed_series();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
