//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "game_primitives.h"
#include "p1_tester_cli.h"
#include "p1_tester_harness.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 test_tile_flags (P1_TesterHarness& h) {
    P1_MakeMap mk(h.prm());
    const clock_t t0 = clock();
    const bool ok = mk.generate(k_p1_step_seed_export);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !mk.is_valid()) {
        std::fprintf(stderr, "P1_MakeMap failed to generate\n");
        return -1;
    }
    const P1_MakeMapRslt& r = mk.result();
    if (r.m_flags == nullptr || r.m_terrain == nullptr) {
        std::fprintf(stderr, "missing flags or terrain\n");
        return -1;
    }
    const u32 npx = static_cast<u32>(r.m_w) * static_cast<u32>(r.m_h);
    u32 defensible_mtn_n = 0u;
    u32 watershed_lim_n = 0u;
    for (u32 i = 0; i < npx; ++i) {
        if (r.m_flags[i].defensible_mtn != 0u) {
            ++defensible_mtn_n;
        }
        if (r.m_flags[i].watershed_lim != 0u) {
            ++watershed_lim_n;
        }
    }
    char flags_pfx[320];
    std::snprintf(flags_pfx, sizeof(flags_pfx), "%s/p1-seed-%u/flags", P1_OUT_ROOT,
        static_cast<unsigned>(h.seed()));
    char flags_path[320];
    std::snprintf(flags_path, sizeof(flags_path), "%s.ppm", flags_pfx);
    if (!mk.save_flags_data_ppm(flags_path)) {
        std::fprintf(stderr, "save_flags_data_ppm failed: %s\n", flags_path);
        return -1;
    }
    if (!mk.save_flags_ppm(flags_pfx)) {
        std::fprintf(stderr, "save_flags_ppm failed: %s_*\n", flags_pfx);
        return -1;
    }
    std::printf("TileFlags: %.6f s (%u x %u) defensible_mtn=%u watershed_lim=%u\n",
        sec,
        static_cast<u32>(r.m_w),
        static_cast<u32>(r.m_h),
        static_cast<unsigned>(defensible_mtn_n),
        static_cast<unsigned>(watershed_lim_n));
    std::printf("saved: %s\n", flags_path);
    std::printf("saved: %s_defensible_mtn.ppm\n", flags_pfx);
    std::printf("saved: %s_watershed_lim.ppm\n", flags_pfx);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    P1_TesterHarness h;
    if (!h.begin(argc, argv)) {
        return -1;
    }
    const i32 rc = test_tile_flags(h);
    if (rc != 0) {
        return rc;
    }
    if (!h.finish()) {
        return -1;
    }
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
