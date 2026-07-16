//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "circular_tile_areas.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_img = 61;
static const u16 k_img_c = 30;
static const char* k_out = "/home/w/Projects/simple-map-gen/city-border-offsets/circular_bands.ppm";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool wr_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main () {
    u8 rgb[k_img * k_img * 3];
    std::memset(rgb, 255, sizeof(rgb));
    bool use_red = true;
    u16 valid_n = 0;

    for (i32 r = 100; r >= -100; --r) {
        const CircArea area = CircularTileAreas::get(static_cast<u16>(r));
        if (area.m_lim == 0) {
            continue;
        }
        const u8 cr = use_red ? 255 : 0;
        const u8 cb = use_red ? 0 : 255;
        for (u16 i = 0; i < area.m_lim; ++i) {
            const i32 x = static_cast<i32>(k_img_c) + static_cast<i32>(area.m_brd[i][0]);
            const i32 y = static_cast<i32>(k_img_c) + static_cast<i32>(area.m_brd[i][1]);
            if (x < 0 || y < 0 || x >= static_cast<i32>(k_img) || y >= static_cast<i32>(k_img)) {
                continue;
            }
            u8* p = &rgb[(static_cast<u32>(y) * k_img + static_cast<u32>(x)) * 3u];
            p[0] = cr;
            p[1] = 0;
            p[2] = cb;
        }
        use_red = !use_red;
        ++valid_n;
        std::printf("painted r=%d lim=%u\n", r, (unsigned)area.m_lim);
    }

    if (!wr_ppm(k_out, rgb, k_img, k_img)) {
        std::printf("fail write %s\n", k_out);
        return 1;
    }
    std::printf("valid=%u wrote %s\n", (unsigned)valid_n, k_out);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
