//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime> 

#include "r1_gen_res_sectors.h"
#include "land_mass_index.h"
#include "map_terrain_data.h"
#include "p1_make_map.h"
#include "p1_tester_harness.h"

//================================================================================================================================
//=> - Test -
//================================================================================================================================

i32 test_r1_gen_res_sectors (P1_TesterHarness& h) {
    char sec_path[320];
    char terr_path[320];
    if (!h.path_pri(sec_path, sizeof(sec_path))) {
        std::printf("failed to ensure sector output path\n");
        return -1;
    }
    if (!h.path_sec(terr_path, sizeof(terr_path))) {
        std::printf("failed to ensure terrain output path\n");
        return -1;
    }
    if (!h.run_input()) {
        std::printf("P1_MakeMap input failed for res sectors\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for res sectors\n");
        return -1;
    }
    LandMassIndex mass;
    if (!mass.generate(chain.m_terrain, chain.m_w, chain.m_h) || !mass.is_valid()) {
        std::printf("LandMassIndex failed\n");
        return -1;
    }
    const LandMassIndexRslt& mr = mass.result();
    std::printf("land_masses=%u land_tiles=%u\n", (unsigned)mr.m_mass_n, mr.m_land_n);
    const u16 pct = h.sec_pct();
    R1_Gen_ResSectors gen;
    const clock_t t0 = clock();
    const bool ok = gen.generate(chain.m_terrain, chain.m_w, chain.m_h, mr, h.prm().m_seed, pct)
        && gen.is_valid();
    const clock_t t1 = clock();
    const double ms = (static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC)) * 1000.0;
    if (!ok) {
        std::printf("R1_Gen_ResSectors failed\n");
        return -1;
    }
    const R1_Gen_ResSectorsRslt& r = gen.result();
    u32 claimed = 0;
    const u32 npx = static_cast<u32>(r.m_w) * static_cast<u32>(r.m_h);
    for (u32 i = 0; i < npx; ++i) {
        if (r.m_ov[i] != static_cast<u16>(R1_RES_SECTOR_NONE)) {
            claimed = claimed + 1u;
        }
    }
    std::printf("P1_MakeMap input time: %.6f s\n", h.input_sec());
    std::printf("R1_Gen_ResSectors gen time: %.2f ms (pct=%u sectors=%u claimed=%u, %u x %u)\n",
        ms, (unsigned)pct, (unsigned)r.m_sec_n, claimed, (unsigned)r.m_w, (unsigned)r.m_h);
    if (!gen.save_ppm(sec_path, chain.m_terrain)) {
        std::printf("failed to save sectors: %s\n", sec_path);
        return -1;
    }
    std::printf("saved: %s\n", sec_path);
    MapTerrainData terr_map;
    if (!terr_map.assign_copy(chain.m_w, chain.m_h, chain.m_terrain)) {
        std::printf("failed to copy terrain for save\n");
        return -1;
    }
    if (!terr_map.save_terrain_ppm(terr_path)) {
        std::printf("failed to save terrain: %s\n", terr_path);
        return -1;
    }
    std::printf("saved: %s\n", terr_path);
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
    const i32 rc = test_r1_gen_res_sectors(h);
    if (rc != 0) {
        return rc;
    }
    if (!h.finish()) {
        return -1;
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
