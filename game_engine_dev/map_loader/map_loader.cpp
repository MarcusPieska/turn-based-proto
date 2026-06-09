//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_loader.h"

#include "map_terrain_validate.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - MapLoader helpers -
//================================================================================================================================

static bool rd_ppm_rgb (cstr path, u16* out_w, u16* out_h, u8** out_rgb) {
    if (path == nullptr || out_w == nullptr || out_h == nullptr || out_rgb == nullptr) {
        return false;
    }
    *out_w = 0;
    *out_h = 0;
    *out_rgb = nullptr;
    FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    char magic[3] = {};
    if (std::fscanf(fp, "%2s", magic) != 1 || magic[0] != 'P' || magic[1] != '6') {
        std::fclose(fp);
        return false;
    }
    int c = std::fgetc(fp);
    while (c == '#') {
        while (c != '\n' && c != EOF) {
            c = std::fgetc(fp);
        }
        c = std::fgetc(fp);
    }
    ungetc(c, fp);
    unsigned wi = 0;
    unsigned hi = 0;
    unsigned maxv = 0;
    if (std::fscanf(fp, "%u %u %u", &wi, &hi, &maxv) != 3 || maxv != 255u) {
        std::fclose(fp);
        return false;
    }
    c = std::fgetc(fp);
    if (c == EOF) {
        std::fclose(fp);
        return false;
    }
    if (wi == 0 || hi == 0 || wi > 65535u || hi > 65535u) {
        std::fclose(fp);
        return false;
    }
    const u16 w = static_cast<u16>(wi);
    const u16 h = static_cast<u16>(hi);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    u8* rgb = new u8[nbytes];
    if (std::fread(rgb, 1, nbytes, fp) != nbytes) {
        delete[] rgb;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    *out_w = w;
    *out_h = h;
    *out_rgb = rgb;
    return true;
}

//================================================================================================================================
//=> - MapLoader -
//================================================================================================================================

bool MapLoader::load_terrain_ppm (cstr path, MapTerrainData& out) {
    u16 w = 0;
    u16 h = 0;
    u8* rgb = nullptr;
    if (!rd_ppm_rgb(path, &w, &h, &rgb)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (!MapTerrainValidate::chk_rgb(rgb, n)) {
        delete[] rgb;
        return false;
    }
    u8* cls = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        const u8* px = rgb + i * 3u;
        bool matched = false;
        cls[i] = MapTerrainValidate::class_from_rgb(px[0], px[1], px[2], &matched);
        if (!matched) {
            delete[] cls;
            delete[] rgb;
            return false;
        }
    }
    delete[] rgb;
    const bool ok = out.assign_copy(w, h, cls);
    delete[] cls;
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
