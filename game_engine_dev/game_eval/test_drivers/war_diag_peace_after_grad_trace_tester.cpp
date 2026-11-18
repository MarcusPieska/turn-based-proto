//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <cstdlib>

//================================================================================================================================
//=> - Capture then muster-grad fail then peace (game_loop.trace) -
//================================================================================================================================
//
//  After a city capture, BuildUp remusters via make_muster_gradient(staging). If that pack fails,
//  war_try_mocks peaces. Counts capture -> muster_grad_fail -> peace_mock chains per seat.
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
        std::printf("war_diag_peace_after_grad_trace: FAILED open %s\n", path);
        return 1;
    }
    unsigned n_cap = 0u;
    unsigned n_grad = 0u;
    unsigned n_peace_mock = 0u;
    unsigned chain = 0u;
    int pend_seat = -1;
    unsigned pend_turn = 0u;
    bool saw_grad = false;
    char line[512];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        if (starts(line, "war city capture")) {
            pend_seat = static_cast<int>(field_u(line, "seat="));
            pend_turn = field_u(line, "turn=");
            saw_grad = false;
            ++n_cap;
            continue;
        }
        if (pend_seat < 0) {
            continue;
        }
        const unsigned seat = field_u(line, "seat=");
        if (seat != static_cast<unsigned>(pend_seat)) {
            continue;
        }
        if (starts(line, "war muster grad fail")) {
            ++n_grad;
            saw_grad = true;
            continue;
        }
        if (starts(line, "war peace mock")) {
            ++n_peace_mock;
            if (saw_grad) {
                ++chain;
                std::printf("  chain seat=%u after_capture_turn=%u peace_turn=%u\n",
                    seat, pend_turn, field_u(line, "turn="));
            }
            pend_seat = -1;
            continue;
        }
        if (starts(line, "war peace ") || starts(line, "war army size") || starts(line, "war city capture")) {
            if (!starts(line, "war city capture")) {
                pend_seat = -1;
            }
        }
    }
    std::fclose(fp);
    std::printf("war_diag_peace_after_grad_trace path=%s\n", path);
    std::printf("captures=%u muster_grad_fail=%u peace_mock=%u capture_grad_peace_chains=%u\n",
        n_cap, n_grad, n_peace_mock, chain);
    std::printf("hypothesis: wars end because remuster pack capital->staging fails after capture.\n");
    const bool hit = (chain > 0u) || (n_grad > 0u && n_peace_mock > 0u);
    std::printf("war_diag_peace_after_grad_trace: %s\n",
        hit ? "PASS (grad-fail peace present)" : "CLEAR (no capture-grad-peace chain)");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
