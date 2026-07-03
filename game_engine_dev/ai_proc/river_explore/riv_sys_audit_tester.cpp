//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "test_support.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool def_sfx_map (cstr in, char* out, size_t cap, cstr sfx) {
    const char* dot = std::strrchr(in, '.');
    if (dot == nullptr) {
        const int n = std::snprintf(out, cap, "%s%s.ppm", in, sfx);
        return n > 0 && static_cast<size_t>(n) < cap;
    }
    const size_t pre = static_cast<size_t>(dot - in);
    const size_t sfx_len = std::strlen(sfx) + 4u;
    if (pre + sfx_len >= cap) {
        return false;
    }
    std::memcpy(out, in, pre);
    const int n = std::snprintf(out + pre, cap - pre, "%s.ppm", sfx);
    return n > 0 && static_cast<size_t>(pre) + static_cast<size_t>(n) < cap;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    if (argc < 2 || argv[1] == nullptr) {
        std::printf("usage: %s <result.ppm> [sysmap.ppm]\n", argv[0]);
        return 1;
    }
    char sys_buf[512];
    char bad_buf[512];
    cstr sys_map = nullptr;
    if (argc >= 3 && argv[2] != nullptr) {
        sys_map = argv[2];
        if (!def_sfx_map(argv[2], bad_buf, sizeof(bad_buf), "_badmap")) {
            std::printf("*** FAILED badmap path\n");
            return 1;
        }
    } else if (!def_sfx_map(argv[1], sys_buf, sizeof(sys_buf), "_sysmap")
        || !def_sfx_map(argv[1], bad_buf, sizeof(bad_buf), "_badmap")) {
        std::printf("*** FAILED sysmap path\n");
        return 1;
    } else {
        sys_map = sys_buf;
    }
    if (!re_riv_audit_run(argv[1], sys_map, bad_buf)) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
