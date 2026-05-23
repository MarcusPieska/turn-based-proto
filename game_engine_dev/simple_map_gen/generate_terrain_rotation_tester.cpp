//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
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

static bool ensure_maps_dir () {
    if (mkdir("maps", 0755) != 0 && errno != EEXIST) {
        return false;
    }
    return true;
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
    MapArrayTerrain ta;
    MapArrayTerrain tb;
    if (!gen_outline(seed, 0.00f, ta)) {
        return false;
    }
    if (!gen_outline(seed, 0.85f, tb)) {
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

i32 test_generate_terrain_rotation_basic (u32 seed, i32 degrees_cw) {
    if (!ensure_maps_dir()) {
        std::printf("could not create maps/\n");
        return -1;
    }
    MapArrayTerrain src;
    const clock_t t_setup0 = clock();
    if (!build_combo_min(seed, src)) {
        std::printf("combo min terrain failed\n");
        return -1;
    }
    const clock_t t_setup1 = clock();
    const double setup_sec = static_cast<double>(t_setup1 - t_setup0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("source terrain setup time: %.6f s (%u x %u)\n", setup_sec, src.width(), src.height());
    if (!src.save("maps/out_map_terrain_rotation_source.ppm")) {
        std::printf("save source failed\n");
        return -1;
    }
    Generate_TerrainRotation rot;
    const clock_t t_rot0 = clock();
    const bool ok = rot.generate(src, degrees_cw);
    const clock_t t_rot1 = clock();
    const double rot_sec = static_cast<double>(t_rot1 - t_rot0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !rot.is_valid()) {
        std::printf("Generate_TerrainRotation failed (degrees=%d)\n", degrees_cw);
        return -1;
    }
    std::printf(
        "Generate_TerrainRotation rotate time: %.6f s (%d deg cw, %u x %u -> %u x %u)\n",
        rot_sec,
        degrees_cw,
        src.width(),
        src.height(),
        rot.width(),
        rot.height());
    char path[128];
    std::snprintf(path, sizeof(path), "maps/out_map_terrain_rotation_%d.ppm", degrees_cw);
    if (!rot.save_output(path)) {
        std::printf("save %s failed\n", path);
        return -1;
    }
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    u32 seed = 0;
    i32 degrees_cw = 90;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    if (argc >= 3) {
        degrees_cw = static_cast<i32>(std::strtol(argv[2], nullptr, 10));
    }
    return test_generate_terrain_rotation_basic(seed, degrees_cw);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
