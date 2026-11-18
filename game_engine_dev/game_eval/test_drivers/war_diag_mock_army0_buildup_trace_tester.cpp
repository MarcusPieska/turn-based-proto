//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring> 
#include <cstdlib>

//================================================================================================================================
//=> Mock army=0 on every siege_fail_buildup -
//================================================================================================================================
//
//  Without instant-win, prep_mocks may chew MockMusterSiege copies then log buildup with army=0
//  barrage=0. This driver counts that stuck pattern on a harness/game_loop trace.
//
//================================================================================================================================

static const char* G_TRACE = "/home/w/Projects/simple-map-gen/game-eval/war-harness-save/war_harness.trace";

static bool starts (const char* line, const char* pref) {
    return std::strncmp(line, pref, std::strlen(pref)) == 0;
}

static unsigned field_u (const char* line, const char* key) {
    const char* p = std::strstr(line, key);
    if (p == nullptr) {
        return 0u;
    }
    p += std::strlen(key);
    return static_cast<unsigned>(std::strtoul(p, nullptr, 10));
}

int main (int argc, char** argv) {
    const char* path = G_TRACE;
    if (argc >= 2) {
        path = argv[1];
    }
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        std::printf("war_diag_mock_army0_buildup_trace: FAILED open %s\n", path);
        return 1;
    }
    unsigned n_bu = 0u;
    unsigned n_army0 = 0u;
    unsigned n_br0 = 0u;
    unsigned n_both0 = 0u;
    unsigned army_max = 0u;
    unsigned br_max = 0u;
    unsigned n_cap = 0u;
    char line[512];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        if (starts(line, "war city capture")) {
            ++n_cap;
            continue;
        }
        if (!starts(line, "war siege fail buildup")) {
            continue;
        }
        ++n_bu;
        const unsigned army = field_u(line, "army=");
        const unsigned br = field_u(line, "barrage=");
        if (army > army_max) {
            army_max = army;
        }
        if (br > br_max) {
            br_max = br;
        }
        if (army == 0u) {
            ++n_army0;
        }
        if (br == 0u) {
            ++n_br0;
        }
        if (army == 0u && br == 0u) {
            ++n_both0;
        }
    }
    std::fclose(fp);
    const bool stuck = (n_bu > 0u && n_both0 == n_bu && n_cap == 0u);
    std::printf(
        "war_diag_mock_army0_buildup_trace path=%s\n"
        "buildups=%u army0=%u barrage0=%u both0=%u army_max=%u barrage_max=%u captures=%u\n",
        path, n_bu, n_army0, n_br0, n_both0, army_max, br_max, n_cap);
    if (stuck) {
        std::printf(
            "hypothesis: every BuildUp logs empty mock (army=0 barrage=0); prep_mocks chewed copies "
            "without taken, never reaches assault/capture.\n");
        std::printf("war_diag_mock_army0_buildup_trace: PASS (army0 buildup stuck)\n");
        return 0;
    }
    std::printf("war_diag_mock_army0_buildup_trace: CLEAR (not a pure army0 stuck pattern)\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================