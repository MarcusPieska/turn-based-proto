//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <sys/stat.h>
#include <sys/types.h>

#include "game_primitives.h"
#include "generate_terrain_combo_min.h"
#include "generate_terrain_cont_outline.h"
#include "generate_terrain_rotation.h"

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

static bool build_combo_min (u32 seed, MapArrayTerrain& out) {
    const f32 grad1 = 0.00f;
    const f32 grad2 = 0.65f;
    MapArrayTerrain ta;
    MapArrayTerrain tb;
    if (!gen_outline(seed, grad1, ta)) {
        return false;
    }
    if (!gen_outline(seed, grad2, tb)) {
        return false;
    }
    Generate_TerrainComboMin combo;
    if (!combo.generate(ta, tb) || !combo.is_valid()) {
        return false;
    }
    const u8* td = combo.terrain_class();
    if (td == nullptr) {
        return false;
    }
    return out.assign_copy(combo.width(), combo.height(), td);
}

static i32 run_var_angle_series (u32 seed) {
    char out_dir[256];
    std::snprintf(out_dir, sizeof(out_dir), "../../../simple-map-gen/rotation");
    if (!mkdirs(out_dir)) {
        std::printf("mkdir %s failed\n", out_dir);
        return -1;
    }
    MapArrayTerrain orig;
    const clock_t t_setup0 = clock();
    if (!build_combo_min(seed, orig)) {
        std::printf("combo min seed %u failed\n", seed);
        return -1;
    }
    const clock_t t_setup1 = clock();
    const double setup_sec = static_cast<double>(t_setup1 - t_setup0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("source terrain setup time: %.6f s (seed %u, %u x %u)\n", setup_sec, seed, orig.width(), orig.height());
    Generate_TerrainRotation rot;
    const int deg_first = 1;
    const int deg_last = 359;
    const int n = deg_last - deg_first + 1;
    const clock_t t_rot0 = clock();
    for (int deg = deg_first; deg <= deg_last; ++deg) {
        if (!rot.generate(orig, deg) || !rot.is_valid()) {
            std::printf("rotation %d deg failed\n", deg);
            return -1;
        }
        char path[384];
        std::snprintf(path, sizeof(path), "%s/frame_%03d.ppm", out_dir, deg);
        if (!rot.save_output(path)) {
            std::printf("save %s failed\n", path);
            return -1;
        }
    }
    const clock_t t_rot1 = clock();
    const double rot_sec = static_cast<double>(t_rot1 - t_rot0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf(
        "wrote %d frames to %s (original map, %d..%d deg cw, total rotate time %.6f s, avg %.6f s/frame)\n",
        n,
        out_dir,
        deg_first,
        deg_last,
        rot_sec,
        rot_sec / static_cast<double>(n));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    u32 seed = 3;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    return run_var_angle_series(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
