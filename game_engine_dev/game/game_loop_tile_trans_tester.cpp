//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

//================================================================================================================================
//=> - Types / constants -
//================================================================================================================================

typedef const char* cstr;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

static const char* G_PPM_ROOT = "/home/w/Projects/simple-map-gen/game-loop-frames";
static const char* G_OUT_ROOT = "/home/w/Projects/simple-map-gen/game-loop-tile-trans";
static const char* G_DEF_DIR = "seed-101-p100";

struct PpmImg {
    u16 w;
    u16 h;
    std::vector<u8> rgb;
};

struct TurnFrame {
    u32 turn;
    char path[512];
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool mk_dir (cstr path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    return ::mkdir(path, 0755) == 0 || errno == EEXIST;
}

static bool parse_turn_name (cstr name, u32* out_turn) {
    if (name == nullptr || out_turn == nullptr) {
        return false;
    }
    unsigned t = 0u;
    if (std::sscanf(name, "turn_%u.ppm", &t) != 1) {
        return false;
    }
    *out_turn = static_cast<u32>(t);
    return true;
}

static bool load_ppm (cstr path, PpmImg* out) {
    if (path == nullptr || out == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    char magic[8];
    unsigned w = 0u;
    unsigned h = 0u;
    unsigned maxv = 0u;
    if (std::fscanf(fp, "%7s", magic) != 1 || std::strcmp(magic, "P6") != 0) {
        std::fclose(fp);
        return false;
    }
    int c = std::fgetc(fp);
    while (c == '#' || c == '\n' || c == '\r' || c == ' ' || c == '\t') {
        if (c == '#') {
            while (c != EOF && c != '\n') {
                c = std::fgetc(fp);
            }
        }
        c = std::fgetc(fp);
    }
    if (c == EOF) {
        std::fclose(fp);
        return false;
    }
    std::ungetc(c, fp);
    if (std::fscanf(fp, "%u %u %u", &w, &h, &maxv) != 3 || w == 0u || h == 0u || maxv != 255u) {
        std::fclose(fp);
        return false;
    }
    c = std::fgetc(fp);
    if (c != '\n' && c != ' ' && c != '\t' && c != '\r') {
        std::fclose(fp);
        return false;
    }
    const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    out->w = static_cast<u16>(w);
    out->h = static_cast<u16>(h);
    out->rgb.assign(n, 0u);
    const bool ok = std::fread(out->rgb.data(), 1, n, fp) == n;
    std::fclose(fp);
    return ok;
}

static bool save_ppm (cstr path, const PpmImg& img) {
    if (path == nullptr || img.w == 0u || img.h == 0u || img.rgb.empty()) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(img.w), static_cast<unsigned>(img.h));
    const size_t n = static_cast<size_t>(img.w) * static_cast<size_t>(img.h) * 3u;
    const bool ok = std::fwrite(img.rgb.data(), 1, n, fp) == n;
    std::fclose(fp);
    return ok;
}

static bool px_eq (const u8* a, const u8* b) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

static bool list_turns (cstr dir, std::vector<TurnFrame>* out) {
    if (dir == nullptr || out == nullptr) {
        return false;
    }
    out->clear();
    DIR* d = ::opendir(dir);
    if (d == nullptr) {
        return false;
    }
    while (const dirent* e = ::readdir(d)) {
        if (e->d_name[0] == '.') {
            continue;
        }
        u32 turn = 0u;
        if (!parse_turn_name(e->d_name, &turn)) {
            continue;
        }
        TurnFrame f = {};
        f.turn = turn;
        if (std::snprintf(f.path, sizeof(f.path), "%s/%s", dir, e->d_name) <= 0) {
            ::closedir(d);
            return false;
        }
        out->push_back(f);
    }
    ::closedir(d);
    for (size_t i = 1; i < out->size(); ++i) {
        TurnFrame key = (*out)[i];
        size_t j = i;
        while (j > 0u && (*out)[j - 1u].turn > key.turn) {
            (*out)[j] = (*out)[j - 1u];
            --j;
        }
        (*out)[j] = key;
    }
    return !out->empty();
}

static const char* base_name (cstr path) {
    if (path == nullptr) {
        return "";
    }
    const char* slash = std::strrchr(path, '/');
    return slash != nullptr ? slash + 1 : path;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    char in_dir[512];
    if (argc >= 2 && argv[1] != nullptr && argv[1][0] != '\0') {
        if (argv[1][0] == '/') {
            if (std::snprintf(in_dir, sizeof(in_dir), "%s", argv[1]) <= 0) {
                std::printf("bad input path\n");
                return 1;
            }
        } else if (std::snprintf(in_dir, sizeof(in_dir), "%s/%s", G_PPM_ROOT, argv[1]) <= 0) {
            std::printf("bad input path\n");
            return 1;
        }
    } else if (std::snprintf(in_dir, sizeof(in_dir), "%s/%s", G_PPM_ROOT, G_DEF_DIR) <= 0) {
        std::printf("bad default path\n");
        return 1;
    }

    const char* leaf = base_name(in_dir);
    char out_dir[512];
    if (std::snprintf(out_dir, sizeof(out_dir), "%s/%s", G_OUT_ROOT, leaf) <= 0) {
        std::printf("bad out path\n");
        return 1;
    }
    if (!mk_dir(G_OUT_ROOT) || !mk_dir(out_dir)) {
        std::printf("FAIL: mkdir %s\n", out_dir);
        return 1;
    }

    std::vector<TurnFrame> frames;
    if (!list_turns(in_dir, &frames)) {
        std::printf("FAIL: no turn_*.ppm in %s\n", in_dir);
        return 1;
    }
    if (frames[0].turn != 0u) {
        std::printf("FAIL: missing turn_0000.ppm (first is turn_%04u)\n", frames[0].turn);
        return 1;
    }

    PpmImg prev;
    if (!load_ppm(frames[0].path, &prev)) {
        std::printf("FAIL: load %s\n", frames[0].path);
        return 1;
    }
    const u32 tile_n = static_cast<u32>(prev.w) * static_cast<u32>(prev.h);
    std::vector<u8> chg(tile_n, 0u);
    std::printf("in=%s\nout=%s\nframes=%zu size=%ux%u\n",
        in_dir, out_dir, frames.size(),
        static_cast<unsigned>(prev.w), static_cast<unsigned>(prev.h));

    u64 tot_xfer = 0u;
    for (size_t fi = 1; fi < frames.size(); ++fi) {
        PpmImg cur;
        if (!load_ppm(frames[fi].path, &cur)) {
            std::printf("FAIL: load %s\n", frames[fi].path);
            return 1;
        }
        if (cur.w != prev.w || cur.h != prev.h) {
            std::printf("FAIL: size mismatch turn_%04u\n", frames[fi].turn);
            return 1;
        }

        PpmImg delta;
        delta.w = cur.w;
        delta.h = cur.h;
        delta.rgb.assign(static_cast<size_t>(tile_n) * 3u, 0u);

        u32 xfer_n = 0u;
        u32 first_n = 0u;
        for (u32 i = 0u; i < tile_n; ++i) {
            const u8* a = &prev.rgb[static_cast<size_t>(i) * 3u];
            const u8* b = &cur.rgb[static_cast<size_t>(i) * 3u];
            if (px_eq(a, b)) {
                continue;
            }
            if (chg[i] < 255u) {
                chg[i] = static_cast<u8>(chg[i] + 1u);
            }
            u8* d = &delta.rgb[static_cast<size_t>(i) * 3u];
            if (chg[i] >= 2u) {
                ++xfer_n;
                d[0] = 255;
                d[1] = 40;
                d[2] = 40;
            } else {
                ++first_n;
                d[0] = 40;
                d[1] = 40;
                d[2] = 120;
            }
        }

        char dpath[560];
        if (std::snprintf(dpath, sizeof(dpath), "%s/delta_%04u.ppm", out_dir, frames[fi].turn) <= 0) {
            std::printf("FAIL: delta path\n");
            return 1;
        }
        if (!save_ppm(dpath, delta)) {
            std::printf("FAIL: write %s\n", dpath);
            return 1;
        }

        tot_xfer += static_cast<u64>(xfer_n);
        std::printf("turn_%04u -> turn_%04u: transfers=%u first_claim=%u delta=%s\n",
            frames[fi - 1u].turn,
            frames[fi].turn,
            static_cast<unsigned>(xfer_n),
            static_cast<unsigned>(first_n),
            dpath);
        prev = std::move(cur);
    }

    std::printf("*** DONE pairs=%zu total_transfer_pixels=%llu\n",
        frames.size() > 0u ? frames.size() - 1u : 0u,
        static_cast<unsigned long long>(tot_xfer));
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
