//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <cstdlib>

//================================================================================================================================
//=> - Formed-army size histogram from game_loop.trace -
//================================================================================================================================
//
//  Verifies whether Form yields campaign-ready force sizes. Prints unit/size buckets and how often
//  a thin form is followed by siege_fail_buildup or melee wipe before the next form.
//
//================================================================================================================================

static const char* G_TRACE = "/home/w/Projects/simple-map-gen/game_loop.trace";

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
        std::printf("war_diag_form_size_trace: FAILED open %s\n", path);
        return 1;
    }
    unsigned n_form = 0u;
    unsigned buck_u[6] = {};
    unsigned buck_sz0 = 0u;
    unsigned thin_then_buildup = 0u;
    unsigned thin_then_wipe = 0u;
    unsigned thin_then_capture = 0u;
    int pending_seat = -1;
    unsigned pending_units = 0u;
    unsigned pending_turn = 0u;
    bool pending_thin = false;
    char line[512];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        if (starts(line, "war army size")) {
            const unsigned seat = field_u(line, "seat=");
            const unsigned units = field_u(line, "units=");
            const unsigned size = field_u(line, "size=");
            const unsigned turn = field_u(line, "turn=");
            ++n_form;
            if (units <= 1u) {
                ++buck_u[0];
            } else if (units < 5u) {
                ++buck_u[1];
            } else if (units < 10u) {
                ++buck_u[2];
            } else if (units < 20u) {
                ++buck_u[3];
            } else if (units < 50u) {
                ++buck_u[4];
            } else {
                ++buck_u[5];
            }
            if (size == 0u) {
                ++buck_sz0;
            }
            pending_seat = static_cast<int>(seat);
            pending_units = units;
            pending_turn = turn;
            pending_thin = (units > 0u && units < 20u);
            continue;
        }
        if (pending_seat < 0 || !pending_thin) {
            continue;
        }
        const unsigned seat = field_u(line, "seat=");
        if (seat != static_cast<unsigned>(pending_seat)) {
            continue;
        }
        if (starts(line, "war army size")) {
            continue;
        }
        if (starts(line, "war siege fail buildup")) {
            ++thin_then_buildup;
            pending_thin = false;
        } else if (starts(line, "war cam melee fail") && field_u(line, "reason=") == 6u) {
            ++thin_then_wipe;
            pending_thin = false;
        } else if (starts(line, "war city capture")) {
            ++thin_then_capture;
            pending_thin = false;
        } else if (starts(line, "war peace")) {
            pending_thin = false;
        }
        (void)pending_units;
        (void)pending_turn;
    }
    std::fclose(fp);
    std::printf("war_diag_form_size_trace path=%s\n", path);
    std::printf("forms=%u size_eq_0=%u\n", n_form, buck_sz0);
    std::printf("units buckets: 1=%u 2-4=%u 5-9=%u 10-19=%u 20-49=%u 50+=%u\n",
        buck_u[0], buck_u[1], buck_u[2], buck_u[3], buck_u[4], buck_u[5]);
    std::printf("after thin form (<20): buildup=%u melee_wipe6=%u capture=%u\n",
        thin_then_buildup, thin_then_wipe, thin_then_capture);
    const unsigned thin = buck_u[0] + buck_u[1] + buck_u[2] + buck_u[3];
    std::printf("hypothesis: Form often emits <20 units; next outcome is buildup/wipe not sustained conquest.\n");
    const bool ok = (n_form > 0u && thin * 2u >= n_form);
    std::printf("war_diag_form_size_trace: %s\n", ok ? "PASS (thin-form dominant)" : "FAIL (thin-form not dominant)");
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
