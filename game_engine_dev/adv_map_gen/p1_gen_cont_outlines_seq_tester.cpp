//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <ctime>

#include <sys/stat.h>

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_gen_cont_outlines.h"
#include "whiteboard.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static const char* k_out_dir = "/home/w/Projects/simple-map-gen/cont-shapes";
static const u32 k_seed_n = 5u;

static bool ensure_dir (cstr path) {
    if (path == nullptr) {
        return false;
    }
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    if (mkdir(path, 0755) == 0) {
        return true;
    }
    return errno == EEXIST;
}

static bool save_cont_merge (cstr path, const u8* comp, u16 w, u16 h) {
    if (path == nullptr || comp == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* tmp = new u8[n];
    if (tmp == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        tmp[i] = comp[i] == 0u ? WL_OVERLAY_WATER_GRAY : WL_OVERLAY_LAND_GRAY;
    }
    MapArrayOverlay map;
    const bool ok = map.assign_copy(w, h, tmp) && map.save(path);
    delete[] tmp;
    return ok;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main () {
    if (!ensure_dir(k_out_dir)) {
        std::printf("failed to ensure output dir: %s\n", k_out_dir);
        return -1;
    }
    P1_RunPrm prm = p1_run_prm_def();
    double tot = 0.0;
    for (u32 seed = 0; seed < k_seed_n; ++seed) {
        prm.m_seed = seed;
        P1_Gen_ContOutlines gen(prm);
        const clock_t t0 = clock();
        const bool ok = gen.generate();
        const clock_t t1 = clock();
        const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
        tot += sec;
        if (!ok || !gen.is_valid()) {
            std::printf("seed %u failed\n", seed);
            return -1;
        }
        const P1_Gen_ContOutlinesRslt& r = gen.result();
        char path[384];
        std::snprintf(path, sizeof(path), "%s/cont_outline_%03u.ppm", k_out_dir, seed);
        const u8* ov = r.m_ov.data();
        if (ov == nullptr || !save_cont_merge(path, ov, r.m_w, r.m_h)) {
            std::printf("failed to save: %s\n", path); 
            return -1;
        }
        std::printf("seed %u: %.6f s saved %s\n", seed, sec, path);
        if (Whiteboard::chkout() == 0u) {
            Whiteboard::dealloc();
        }
    }
    std::printf("total: %.6f s (%u seeds)\n", tot, k_seed_n);
    if (Whiteboard::chkout() != 0u) {
        std::printf("Whiteboard checkout leak: %u\n", Whiteboard::chkout());
        return -1;
    }
    Whiteboard::dealloc();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
