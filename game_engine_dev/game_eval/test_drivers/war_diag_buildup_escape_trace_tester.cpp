//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <cstdlib>

//================================================================================================================================
//=> - BuildUp escape: post-buildup form growth / capture from game_loop.trace -
//================================================================================================================================
//
//  After first siege_fail_buildup per seat, check later army_size max and whether any city capture
//  occurs. Seats with no later form or max_units still <20 are treated as stuck in thin BuildUp.
//
//================================================================================================================================

static const char* G_TRACE = "/home/w/Projects/simple-map-gen/game_loop.trace";
static const unsigned G_MIN = 20u;
static const unsigned G_SEAT_CAP = 256u;

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
        std::printf("war_diag_buildup_escape_trace: FAILED open %s\n", path);
        return 1;
    }
    unsigned first_bu[G_SEAT_CAP];
    unsigned later_forms[G_SEAT_CAP];
    unsigned max_later[G_SEAT_CAP];
    unsigned later_cap[G_SEAT_CAP];
    unsigned bu_n[G_SEAT_CAP];
    for (unsigned i = 0u; i < G_SEAT_CAP; ++i) {
        first_bu[i] = 0u;
        later_forms[i] = 0u;
        max_later[i] = 0u;
        later_cap[i] = 0u;
        bu_n[i] = 0u;
    }
    char line[512];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        if (starts(line, "war siege fail buildup")) {
            const unsigned seat = field_u(line, "seat=");
            const unsigned turn = field_u(line, "turn=");
            if (seat >= G_SEAT_CAP) {
                continue;
            }
            ++bu_n[seat];
            if (first_bu[seat] == 0u) {
                first_bu[seat] = turn;
            }
            continue;
        }
        if (starts(line, "war army size")) {
            const unsigned seat = field_u(line, "seat=");
            const unsigned units = field_u(line, "units=");
            const unsigned turn = field_u(line, "turn=");
            if (seat >= G_SEAT_CAP || first_bu[seat] == 0u || turn <= first_bu[seat]) {
                continue;
            }
            ++later_forms[seat];
            if (units > max_later[seat]) {
                max_later[seat] = units;
            }
            continue;
        }
        if (starts(line, "war city capture")) {
            const unsigned seat = field_u(line, "seat=");
            const unsigned turn = field_u(line, "turn=");
            if (seat >= G_SEAT_CAP || first_bu[seat] == 0u || turn <= first_bu[seat]) {
                continue;
            }
            ++later_cap[seat];
        }
    }
    std::fclose(fp);
    unsigned seats = 0u;
    unsigned stuck_no_form = 0u;
    unsigned stuck_thin = 0u;
    unsigned grew = 0u;
    unsigned with_cap = 0u;
    std::printf("war_diag_buildup_escape_trace path=%s min=%u\n", path, G_MIN);
    for (unsigned s = 0u; s < G_SEAT_CAP; ++s) {
        if (first_bu[s] == 0u) {
            continue;
        }
        ++seats;
        const bool thin = (later_forms[s] == 0u) || (max_later[s] < G_MIN);
        if (later_forms[s] == 0u) {
            ++stuck_no_form;
        } else if (max_later[s] < G_MIN) {
            ++stuck_thin;
        } else {
            ++grew;
        }
        if (later_cap[s] > 0u) {
            ++with_cap;
        }
        std::printf(
            "  seat=%u first_bu@%u bu_n=%u later_forms=%u max_later_units=%u later_captures=%u %s\n",
            s, first_bu[s], bu_n[s], later_forms[s], max_later[s], later_cap[s],
            thin ? "STUCK_THIN" : "GREW");
    }
    std::printf(
        "seats_with_buildup=%u stuck_no_later_form=%u stuck_max<min=%u grew_ge_min=%u later_capture_seats=%u\n",
        seats, stuck_no_form, stuck_thin, grew, with_cap);
    std::printf(
        "hypothesis: BuildUp often never reforms a force >= declare_min; growth without capture still stalls.\n");
    const bool ok = (seats > 0u && (stuck_no_form + stuck_thin) * 2u >= seats);
    std::printf("war_diag_buildup_escape_trace: %s\n",
        ok ? "PASS (BuildUp stickiness dominant)" : "FAIL (most seats escape thin BuildUp)");
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
