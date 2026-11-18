//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <cstdlib>

//================================================================================================================================
//=> - Declare-min vs muster/form sizes from game_loop.trace -
//================================================================================================================================
//
//  Declare gates on muster-city land depart count >= 20 (Form's leave-one source). This driver checks
//  whether engage musters/forms are still below that gate on an existing trace (pre- or post-fix).
//
//================================================================================================================================

static const char* G_TRACE = "/home/w/Projects/simple-map-gen/game_loop.trace";
static const unsigned G_MIN = 20u;

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
        std::printf("war_diag_declare_vs_muster_trace: FAILED open %s\n", path);
        return 1;
    }
    unsigned n_muster = 0u;
    unsigned n_form = 0u;
    unsigned muster_lt = 0u;
    unsigned muster_ge = 0u;
    unsigned form_lt = 0u;
    unsigned form_ge = 0u;
    unsigned muster_u1 = 0u;
    unsigned form_u1 = 0u;
    unsigned muster_grp1 = 0u;
    unsigned declare5 = 0u;
    char line[512];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        if (starts(line, "war declare fail") && field_u(line, "reason=") == 5u) {
            ++declare5;
            continue;
        }
        if (starts(line, "war muster start")) {
            const unsigned units = field_u(line, "units=");
            const unsigned cities = field_u(line, "cities=");
            ++n_muster;
            if (units < G_MIN) {
                ++muster_lt;
            } else {
                ++muster_ge;
            }
            if (units <= 1u) {
                ++muster_u1;
            }
            if (cities <= 1u) {
                ++muster_grp1;
            }
            continue;
        }
        if (starts(line, "war army size")) {
            const unsigned units = field_u(line, "units=");
            ++n_form;
            if (units < G_MIN) {
                ++form_lt;
            } else {
                ++form_ge;
            }
            if (units <= 1u) {
                ++form_u1;
            }
        }
    }
    std::fclose(fp);
    std::printf("war_diag_declare_vs_muster_trace path=%s declare_min=%u\n", path, G_MIN);
    std::printf("declare_fail_reason5(army_lt_min)=%u\n", declare5);
    std::printf("muster_start n=%u units<min=%u units>=min=%u units<=1=%u grps<=1=%u\n",
        n_muster, muster_lt, muster_ge, muster_u1, muster_grp1);
    std::printf("form/army_size n=%u units<min=%u units>=min=%u units<=1=%u\n",
        n_form, form_lt, form_ge, form_u1);
    std::printf(
        "note: post-fix declare uses muster-city count; thin muster/form here means gate still mismatched or old trace.\n");
    const bool thin = (n_muster > 0u && muster_lt * 2u >= n_muster && form_lt * 2u >= n_form);
    std::printf("war_diag_declare_vs_muster_trace: %s\n",
        thin ? "THIN (muster/form often < declare_min)" : "OK (muster/form usually >= declare_min)");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
