//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cmath>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "eval_driver.h"
#include "eval_need.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "prof-suite-eval";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_prof_suite.py";
static const u16 k_tag_cap = 256u;
static const u16 k_file_cap = 32u;
static const u16 k_name_cap = 64u;

//================================================================================================================================
//=> - Types -
//================================================================================================================================

struct TagAgg {
    char m_nm[k_name_cap];
    u64 m_sum;
    u32 m_n;
};

struct ProfFile {
    char m_path[512];
    char m_stem[96];
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool mkdir_p (cstr path) {
    if (path == nullptr) {
        return false;
    }
    return ::mkdir(path, 0755) == 0 || errno == EEXIST;
}

static bool mkdir_deep (cstr path) {
    if (path == nullptr || path[0] == 0) {
        return false;
    }
    char tmp[512];
    if (std::snprintf(tmp, sizeof(tmp), "%s", path) <= 0) {
        return false;
    }
    const u32 n = static_cast<u32>(std::strlen(tmp));
    for (u32 i = 1; i < n; ++i) {
        if (tmp[i] != '/') {
            continue;
        }
        tmp[i] = 0;
        if (!mkdir_p(tmp)) {
            return false;
        }
        tmp[i] = '/';
    }
    return mkdir_p(tmp);
}

static bool is_prof_name (cstr nm) {
    if (nm == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(std::strlen(nm));
    if (n < 10u || std::strncmp(nm, "prof_", 5) != 0) {
        return false;
    }
    return n >= 4u && std::strcmp(nm + (n - 4u), ".txt") == 0;
}

static bool dir_of (cstr path, char* out, u32 cap) {
    if (path == nullptr || out == nullptr || cap < 2u) {
        return false;
    }
    const char* slash = std::strrchr(path, '/');
    if (slash == nullptr || slash == path) {
        return std::snprintf(out, cap, ".") > 0;
    }
    const u32 n = static_cast<u32>(slash - path);
    if (n + 1u >= cap) {
        return false;
    }
    std::memcpy(out, path, n);
    out[n] = 0;
    return true;
}

static u16 find_tag (TagAgg* tags, u16 n, cstr nm) {
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(tags[i].m_nm, nm) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool add_sample (TagAgg* tags, u16* n, cstr nm, u64 ns) {
    if (tags == nullptr || n == nullptr || nm == nullptr || nm[0] == 0) {
        return false;
    }
    u16 i = find_tag(tags, *n, nm);
    if (i == U16_KEY_NULL) {
        if (*n >= k_tag_cap) {
            return false;
        }
        i = *n;
        if (std::snprintf(tags[i].m_nm, k_name_cap, "%s", nm) <= 0) {
            return false;
        }
        tags[i].m_sum = 0u;
        tags[i].m_n = 0u;
        *n = static_cast<u16>(*n + 1u);
    }
    tags[i].m_sum += ns;
    tags[i].m_n += 1u;
    return true;
}

static cstr unit_name (u8 u) {
    if (u == 0u) {
        return "s";
    }
    if (u == 1u) {
        return "ms";
    }
    if (u == 2u) {
        return "us";
    }
    return "ns";
}

static f64 unit_div (u8 u) {
    if (u == 0u) {
        return 1000000000.0;
    }
    if (u == 1u) {
        return 1000000.0;
    }
    if (u == 2u) {
        return 1000.0;
    }
    return 1.0;
}

static u8 pick_unit (f64 avg_ns) {
    for (u8 u = 0u; u < 4u; ++u) {
        if ((avg_ns / unit_div(u)) > 1.0) {
            return u;
        }
    }
    return 3u;
}

static int cmp_tag (const void* a, const void* b) {
    const TagAgg* ta = static_cast<const TagAgg*>(a);
    const TagAgg* tb = static_cast<const TagAgg*>(b);
    const f64 aa = (ta->m_n == 0u) ? 0.0 : (static_cast<f64>(ta->m_sum) / static_cast<f64>(ta->m_n));
    const f64 ab = (tb->m_n == 0u) ? 0.0 : (static_cast<f64>(tb->m_sum) / static_cast<f64>(tb->m_n));
    if (aa < ab) {
        return 1;
    }
    if (aa > ab) {
        return -1;
    }
    return std::strcmp(ta->m_nm, tb->m_nm);
}

static bool scan_prof_dir (cstr dir, ProfFile* out, u16* n) {
    if (dir == nullptr || out == nullptr || n == nullptr) {
        return false;
    }
    *n = 0u;
    DIR* dp = ::opendir(dir);
    if (dp == nullptr) {
        return false;
    }
    for (;;) {
        const dirent* de = ::readdir(dp);
        if (de == nullptr) {
            break;
        }
        if (!is_prof_name(de->d_name)) {
            continue;
        }
        if (*n >= k_file_cap) {
            break;
        }
        ProfFile& f = out[*n];
        if (std::snprintf(f.m_path, sizeof(f.m_path), "%s/%s", dir, de->d_name) <= 0) {
            continue;
        }
        if (std::snprintf(f.m_stem, sizeof(f.m_stem), "%s", de->d_name) <= 0) {
            continue;
        }
        char* dot = std::strrchr(f.m_stem, '.');
        if (dot != nullptr) {
            *dot = 0;
        }
        *n = static_cast<u16>(*n + 1u);
    }
    ::closedir(dp);
    return *n != 0u;
}

static bool load_prof (cstr path, TagAgg* tags, u16* n) {
    if (path == nullptr || tags == nullptr || n == nullptr) {
        return false;
    }
    *n = 0u;
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        return false;
    }
    char line[256];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        if (line[0] == '#' || line[0] == 0 || line[0] == '\n') {
            continue;
        }
        char* eq = std::strchr(line, '=');
        if (eq == nullptr) {
            continue;
        }
        *eq = 0;
        char* val = eq + 1;
        while (*val == ' ') {
            ++val;
        }
        char* end = nullptr;
        const unsigned long long ns = std::strtoull(val, &end, 10);
        if (end == val) {
            continue;
        }
        if (!add_sample(tags, n, line, static_cast<u64>(ns))) {
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return *n != 0u;
}

static void wr_avgs (std::FILE* fp, cstr stem, const TagAgg* tags, u16 n) {
    if (fp == nullptr || tags == nullptr) {
        return;
    }
    std::fprintf(fp, "# profiler=%s tags=%u\n", stem, static_cast<u32>(n));
    std::fprintf(fp, "tag calls avg unit\n");
    for (u16 i = 0; i < n; ++i) {
        const TagAgg& t = tags[i];
        if (t.m_n == 0u) {
            continue;
        }
        const f64 avg_ns = static_cast<f64>(t.m_sum) / static_cast<f64>(t.m_n);
        const u8 u = pick_unit(avg_ns);
        const f64 avg = avg_ns / unit_div(u);
        std::fprintf(fp, "%s %u %.2f %s\n", t.m_nm, static_cast<u32>(t.m_n), avg, unit_name(u));
    }
}

static void print_avgs (cstr stem, const TagAgg* tags, u16 n) {
    std::printf("=== %s averages ===\n", stem);
    for (u16 i = 0; i < n; ++i) {
        const TagAgg& t = tags[i];
        if (t.m_n == 0u) {
            continue;
        }
        const f64 avg_ns = static_cast<f64>(t.m_sum) / static_cast<f64>(t.m_n);
        const f64 sum_ns = static_cast<f64>(t.m_sum);
        const u8 ua = pick_unit(avg_ns);
        const u8 us = pick_unit(sum_ns);
        const f64 avg = avg_ns / unit_div(ua);
        const f64 sum = sum_ns / unit_div(us);
        std::printf("  %-28s calls=%-8u avg=%.2f %s  tot=%.2f %s\n",
            t.m_nm, static_cast<u32>(t.m_n), avg, unit_name(ua), sum, unit_name(us));
    }
}

static bool run_plot (cstr src, cstr out_png) {
    char cmd[2048];
    if (std::snprintf(cmd, sizeof(cmd), "python3 \"%s\" \"%s\" \"%s\"", G_PLOT_PY, src, out_png) <= 0) {
        return false;
    }
    return std::system(cmd) == 0;
}

//================================================================================================================================
//=> - ProfSuiteEvalTester -
//================================================================================================================================

class ProfSuiteEvalTester : public EvalDriver {
public:
    ProfSuiteEvalTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    return n;
}

ProfSuiteEvalTester::ProfSuiteEvalTester ()
    : EvalDriver(make_need()) {
}

int ProfSuiteEvalTester::run () {
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("prof_suite_eval: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("prof_suite_eval: cannot mkdir\n");
        return 1;
    }
    char prof_dir[512];
    if (!dir_of(paths().trace_path(), prof_dir, sizeof(prof_dir))) {
        std::printf("prof_suite_eval: bad trace dir\n");
        return 1;
    }
    ProfFile files[k_file_cap];
    u16 file_n = 0u;
    if (!scan_prof_dir(prof_dir, files, &file_n)) {
        std::printf("prof_suite_eval: no prof_*.txt under %s\n", prof_dir);
        return 1;
    }
    std::printf("prof_suite_eval found=%u dir=%s\n", static_cast<u32>(file_n), prof_dir);
    u16 ok_n = 0u;
    for (u16 fi = 0; fi < file_n; ++fi) {
        TagAgg tags[k_tag_cap];
        u16 tag_n = 0u;
        if (!load_prof(files[fi].m_path, tags, &tag_n)) {
            std::printf("prof_suite_eval: load failed %s\n", files[fi].m_path);
            continue;
        }
        std::qsort(tags, static_cast<size_t>(tag_n), sizeof(TagAgg), cmp_tag);
        print_avgs(files[fi].m_stem, tags, tag_n);
        char avg_p[512];
        char png_p[512];
        if (std::snprintf(avg_p, sizeof(avg_p), "%s/%s_avgs.txt", data_dir, files[fi].m_stem) <= 0
            || std::snprintf(png_p, sizeof(png_p), "%s/%s.png", eval_dir, files[fi].m_stem) <= 0) {
            continue;
        }
        std::FILE* afp = std::fopen(avg_p, "w");
        if (afp == nullptr) {
            std::printf("prof_suite_eval: cannot write %s\n", avg_p);
            continue;
        }
        wr_avgs(afp, files[fi].m_stem, tags, tag_n);
        std::fclose(afp);
        char avg_copy[512];
        if (std::snprintf(avg_copy, sizeof(avg_copy), "%s/%s_avgs.txt", eval_dir, files[fi].m_stem) > 0) {
            char cp[2048];
            if (std::snprintf(cp, sizeof(cp), "cp \"%s\" \"%s\"", avg_p, avg_copy) > 0) {
                (void)std::system(cp);
            }
        }
        std::fflush(stdout);
        if (!run_plot(files[fi].m_path, png_p)) {
            std::printf("prof_suite_eval: plot failed %s\n", png_p);
            continue;
        }
        std::printf("wrote %s\n", avg_p);
        std::printf("wrote %s\n", png_p);
        ok_n = static_cast<u16>(ok_n + 1u);
    }
    std::printf("prof_suite_eval ok=%u/%u\n", static_cast<u32>(ok_n), static_cast<u32>(file_n));
    return (ok_n == file_n && ok_n != 0u) ? 0 : 1;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    ProfSuiteEvalTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
