//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <cstdlib>

//================================================================================================================================
//=> - Trace stall summary (printf only) -
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
        std::printf("war_diag_trace_stall: FAILED open %s\n", path);
        return 1;
    }
    unsigned capture = 0u;
    unsigned melee6 = 0u;
    unsigned assault_stop = 0u;
    unsigned set_goal = 0u;
    unsigned exposure = 0u;
    unsigned buildup = 0u;
    unsigned peace_mock = 0u;
    unsigned peace = 0u;
    unsigned start_camp1 = 0u;
    unsigned start_camp4 = 0u;
    unsigned start_camp5 = 0u;
    unsigned declare5 = 0u;
    unsigned thin_army = 0u;
    unsigned army_lines = 0u;
    unsigned spg5 = 0u;
    unsigned spg4 = 0u;
    char line[512];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        if (starts(line, "war city capture")) {
            ++capture;
        } else if (starts(line, "war cam melee fail") && field_u(line, "reason=") == 6u) {
            ++melee6;
        } else if (starts(line, "war assault fail stop")) {
            ++assault_stop;
        } else if (starts(line, "war set goal fail")) {
            ++set_goal;
        } else if (starts(line, "war exposure fail")) {
            ++exposure;
        } else if (starts(line, "war siege fail buildup")) {
            ++buildup;
        } else if (starts(line, "war peace mock")) {
            ++peace_mock;
        } else if (starts(line, "war peace ")) {
            ++peace;
        } else if (starts(line, "war start camp fail")) {
            const unsigned r = field_u(line, "reason=");
            if (r == 1u) {
                ++start_camp1;
            } else if (r == 4u) {
                ++start_camp4;
            } else if (r == 5u) {
                ++start_camp5;
            }
        } else if (starts(line, "war declare fail") && field_u(line, "reason=") == 5u) {
            ++declare5;
        } else if (starts(line, "war army size")) {
            ++army_lines;
            const unsigned units = field_u(line, "units=");
            if (units > 0u && units < 20u) {
                ++thin_army;
            }
        } else if (starts(line, "spg fail")) {
            const unsigned r = field_u(line, "reason=");
            if (r == 5u) {
                ++spg5;
            } else if (r == 4u) {
                ++spg4;
            }
        }
    }
    std::fclose(fp);
    std::printf("war_diag_trace_stall path=%s\n", path);
    std::printf("captures=%u peace=%u peace_mock=%u\n", capture, peace, peace_mock);
    std::printf("melee_wipe_r6=%u assault_fail_stop=%u siege_fail_buildup=%u\n", melee6, assault_stop, buildup);
    std::printf("set_goal_fail=%u spg_dial_fail_r5=%u spg_outside_r4=%u exposure_fail=%u\n",
        set_goal, spg5, spg4, exposure);
    std::printf("start_camp r1=%u r4=%u r5=%u declare_r5=%u\n",
        start_camp1, start_camp4, start_camp5, declare5);
    std::printf("army_size_lines=%u thin_army_units_lt_20=%u\n", army_lines, thin_army);
    std::printf("verdict:\n");
    if (buildup > capture * 10u) {
        std::printf("  dominant stall: siege_fail_buildup (mock says cannot take; endless BuildUp).\n");
    }
    if (set_goal > 100u || spg5 > 100u) {
        std::printf("  pathing stall: set_goal_fail / spg dial_fail (reason5) after thin or post-peace armies.\n");
    }
    if (thin_army * 2u > army_lines) {
        std::printf("  form_army often yields <20 units (size frequently 0) before assault/buildup.\n");
    }
    if (melee6 > 0u) {
        std::printf("  residual melee wipe reason6 still present.\n");
    }
    const bool ok = (buildup > 0u || set_goal > 0u || thin_army > 0u || melee6 > 0u);
    std::printf("war_diag_trace_stall: %s\n", ok ? "PASS (stall signals present)" : "FAIL (no stall signals)");
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
