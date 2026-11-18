//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <cstdlib>

//================================================================================================================================
//=> - Combat gate vs path/form readiness (game_loop.trace) -
//================================================================================================================================
//
//  Instant-win vs real combat: same track means forms/pathing can succeed while captures stay near
//  zero without LUCKY_SEAT_INSTANT_COMBAT_WIN. This driver checks that pattern on one trace.
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
        std::printf("war_diag_combat_gate_trace: FAILED open %s\n", path);
        return 1;
    }
    unsigned n_form = 0u;
    unsigned n_cap = 0u;
    unsigned n_assault = 0u;
    unsigned n_buildup = 0u;
    unsigned n_set_goal = 0u;
    unsigned n_peace_mock = 0u;
    unsigned n_melee6 = 0u;
    unsigned n_grad = 0u;
    unsigned form_ge20 = 0u;
    unsigned form_to_bu = 0u;
    unsigned form_to_cap = 0u;
    unsigned form_to_goal = 0u;
    int pend_seat = -1;
    bool pend_open = false;
    char line[512];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        if (starts(line, "war army size")) {
            const unsigned seat = field_u(line, "seat=");
            const unsigned units = field_u(line, "units=");
            ++n_form;
            if (units >= 20u) {
                ++form_ge20;
            }
            pend_seat = static_cast<int>(seat);
            pend_open = true;
            continue;
        }
        if (starts(line, "war city capture")) {
            ++n_cap;
            if (pend_open && field_u(line, "seat=") == static_cast<unsigned>(pend_seat)) {
                ++form_to_cap;
                pend_open = false;
            }
            continue;
        }
        if (starts(line, "war assault")) {
            ++n_assault;
            continue;
        }
        if (starts(line, "war siege fail buildup")) {
            ++n_buildup;
            if (pend_open && field_u(line, "seat=") == static_cast<unsigned>(pend_seat)) {
                ++form_to_bu;
                pend_open = false;
            }
            continue;
        }
        if (starts(line, "war set goal fail")) {
            ++n_set_goal;
            if (pend_open && field_u(line, "seat=") == static_cast<unsigned>(pend_seat)) {
                ++form_to_goal;
                pend_open = false;
            }
            continue;
        }
        if (starts(line, "war peace mock")) {
            ++n_peace_mock;
            continue;
        }
        if (starts(line, "war cam melee fail") && field_u(line, "reason=") == 6u) {
            ++n_melee6;
            continue;
        }
        if (starts(line, "war muster grad fail")) {
            ++n_grad;
        }
    }
    std::fclose(fp);
    std::printf("war_diag_combat_gate_trace path=%s\n", path);
    std::printf("ready: forms=%u forms_units_ge20=%u set_goal_fail=%u\n", n_form, form_ge20, n_set_goal);
    std::printf("combat: captures=%u assault_lines=%u buildup=%u peace_mock=%u melee_wipe6=%u muster_grad_fail=%u\n",
        n_cap, n_assault, n_buildup, n_peace_mock, n_melee6, n_grad);
    std::printf("after form: buildup=%u capture=%u set_goal=%u\n", form_to_bu, form_to_cap, form_to_goal);
    const bool formed = (n_form > 0u);
    const bool path_ok = (n_set_goal * 2u <= n_form + 1u);
    const bool low_cap = (n_cap * 5u <= n_form + 1u) || (n_cap <= 5u && n_buildup >= n_cap * 10u);
    const bool mock_dom = (n_buildup > n_cap * 5u) || (n_assault == 0u && n_form > 0u);
    std::printf(
        "hypothesis: without instant-win, armies form/path but mock/BuildUp (not assault conquest) dominates.\n");
    const bool ok = formed && path_ok && low_cap && mock_dom;
    std::printf("war_diag_combat_gate_trace: %s\n",
        ok ? "PASS (combat/mock gate; track formed)" : "FAIL (not a clear combat-gate stall)");
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
