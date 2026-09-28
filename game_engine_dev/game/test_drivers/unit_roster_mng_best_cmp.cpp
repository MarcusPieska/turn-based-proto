//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_DIR = "/home/w/Projects/simple-map-gen/unit-roster";
static const char* G_MK_A = "mk01";
static const char* G_MK_B = "mk02";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool load_tokens (const char* path, std::vector<std::string>* out) {
    if (path == nullptr || out == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        std::printf("unit_roster_mng_best_cmp: cannot open %s\n", path);
        return false;
    }
    char buf[512];
    while (std::fgets(buf, sizeof(buf), fp) != nullptr) {
        char* p = buf;
        while (*p != 0) {
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
                ++p;
            }
            if (*p == 0) {
                break;
            }
            char* start = p;
            while (*p != 0 && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
                ++p;
            }
            if (*p != 0) {
                *p = 0;
                ++p;
            }
            out->push_back(std::string(start));
        }
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    char path_a[512];
    char path_b[512];
    const char* mk_a = G_MK_A;
    const char* mk_b = G_MK_B;
    if (argc >= 3) {
        mk_a = argv[1];
        mk_b = argv[2];
    }
    std::snprintf(path_a, sizeof(path_a), "%s/unit_roster_%s_best_of_type.txt", G_DIR, mk_a);
    std::snprintf(path_b, sizeof(path_b), "%s/unit_roster_%s_best_of_type.txt", G_DIR, mk_b);

    std::vector<std::string> tok_a;
    std::vector<std::string> tok_b;
    if (!load_tokens(path_a, &tok_a) || !load_tokens(path_b, &tok_b)) {
        return 1;
    }
    std::printf("unit_roster_mng_best_cmp: %s tokens=%zu  %s tokens=%zu\n",
        mk_a, tok_a.size(), mk_b, tok_b.size());
    if (tok_a.size() != tok_b.size()) {
        std::printf("FAIL: token count mismatch (%zu vs %zu)\n", tok_a.size(), tok_b.size());
        return 1;
    }
    for (size_t i = 0; i < tok_a.size(); ++i) {
        if (tok_a[i] != tok_b[i]) {
            std::printf("FAIL: token %zu  %s='%s'  %s='%s'\n",
                i, mk_a, tok_a[i].c_str(), mk_b, tok_b[i].c_str());
            return 1;
        }
    }
    std::printf("PASS: %s and %s best_of_type outputs match\n", mk_a, mk_b);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
