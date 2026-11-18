//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <cstdlib>

//================================================================================================================================
//=> - BuildUp mock barrage gate (game_loop.trace) -
//================================================================================================================================
//
//  prep_mocks marches without siege sim when barrage_n > 10; else requires mock.siege taken.
//  Instant-win forces siege taken. This driver reads army=/barrage= on siege_fail_buildup lines
//  (needs a trace after that log fields were added).
//
//================================================================================================================================

static const char* G_TRACE = "/home/w/Projects/simple-map-gen/game_loop.trace";
static const unsigned G_MIN_BR = 10u;

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

static bool has_key (const char* line, const char* key) {
    return std::strstr(line, key) != nullptr;
}

int main (int argc, char** argv) {
    const char* path = G_TRACE;
    if (argc >= 2) {
        path = argv[1];
    }
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        std::printf("war_diag_mock_barrage_gate_trace: FAILED open %s\n", path);
        return 1;
    }
    unsigned n_bu = 0u;
    unsigned n_with = 0u;
    unsigned br_le = 0u;
    unsigned br_gt = 0u;
    unsigned army_sum = 0u;
    unsigned br_sum = 0u;
    unsigned army_max = 0u;
    unsigned br_max = 0u;
    char line[512];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        if (!starts(line, "war siege fail buildup")) {
            continue;
        }
        ++n_bu;
        if (!has_key(line, "army=") || !has_key(line, "barrage=")) {
            continue;
        }
        ++n_with;
        const unsigned army = field_u(line, "army=");
        const unsigned br = field_u(line, "barrage=");
        army_sum += army;
        br_sum += br;
        if (army > army_max) {
            army_max = army;
        }
        if (br > br_max) {
            br_max = br;
        }
        if (br <= G_MIN_BR) {
            ++br_le;
        } else {
            ++br_gt;
        }
    }
    std::fclose(fp);
    std::printf("war_diag_mock_barrage_gate_trace path=%s min_barrage=%u\n", path, G_MIN_BR);
    std::printf("buildups=%u with_army_barrage_fields=%u\n", n_bu, n_with);
    if (n_with == 0u) {
        std::printf("note: rebuild+rerun needed for army=/barrage= on buildup lines.\n");
        std::printf("war_diag_mock_barrage_gate_trace: WAIT (no barrage fields in trace)\n");
        return 0;
    }
    std::printf("barrage<=min=%u barrage>min=%u army_max=%u barrage_max=%u army_avg=%u barrage_avg=%u\n",
        br_le, br_gt, army_max, br_max,
        army_sum / n_with, br_sum / n_with);
    std::printf(
        "hypothesis: BuildUp is entered with barrage_n<=%u so prep_mocks requires mock.siege (instant-win skips that).\n",
        G_MIN_BR);
    const bool ok = (br_le * 2u >= n_with);
    std::printf("war_diag_mock_barrage_gate_trace: %s\n",
        ok ? "PASS (low-barrage mock gate dominant)" : "FAIL (many buildups already over barrage min)");
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
