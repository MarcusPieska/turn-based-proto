//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "generate_distance_to_ocean_coast_tm.h"
#include "generate_distance_to_ocean_coast.h"
#include "ai_whiteboard.h"
#include "game_map_defs.h"
#include "map_loader.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr k_in_path = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr k_in_riv = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr k_out_path = "/home/w/Projects/simple-map-gen/distance-to-ocean-coast-tm.ppm";
static const cstr k_out_riv = "/home/w/Projects/simple-map-gen/distance-to-ocean-coast-tm-riv.ppm";
static const cstr k_out_self = "/home/w/Projects/simple-map-gen/distance-to-ocean-coast-tm-self.ppm";
static const u8 k_wtr_b = 30;
static const u8 k_wtr_g = 110;
static const u8 k_wtr_r = 220;
static const u8 k_riv_r = 40;
static const u8 k_riv_g = 160;
static const u8 k_riv_b = 255;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_water (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static u8 norm_gray (u16 d, u16 max_d) {
    if (d == 0xFFFFu) {
        return 0;
    }
    if (max_d == 0) {
        return 255;
    }
    return static_cast<u8>((static_cast<u32>(d) * 255u) / static_cast<u32>(max_d));
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
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

static bool load_riv (cstr path, u16 ew, u16 eh, u8** out_riv) {
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
    if (c == EOF || wi == 0 || hi == 0 || wi != ew || hi != eh) {
        std::fclose(fp);
        return false;
    }
    const u32 tn = static_cast<u32>(ew) * static_cast<u32>(eh);
    u8* rgb = new u8[tn * 3u];
    if (std::fread(rgb, 1, static_cast<size_t>(tn) * 3u, fp) != static_cast<size_t>(tn) * 3u) {
        delete[] rgb;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    u8* riv = new u8[tn];
    for (u32 i = 0; i < tn; ++i) {
        const u8* px = rgb + i * 3u;
        riv[i] = (px[0] != 0 || px[1] != 0 || px[2] != 0) ? 1 : 0;
    }
    delete[] rgb;
    *out_riv = riv;
    return true;
}

static bool build_img (
    const u8* terrain,
    const u16* dist,
    u16 w,
    u16 h,
    u16 max_d,
    u8* rgb) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        const u8 t = terrain[i];
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (is_water(t)) {
            r = k_wtr_r;
            g = k_wtr_g;
            b = k_wtr_b;
        } else {
            const u8 gry = norm_gray(dist[i], max_d);
            r = gry;
            g = gry;
            b = gry;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    return true;
}

static bool build_img_riv (
    const u8* terrain,
    const u8* rivers,
    const u16* dist,
    u16 w,
    u16 h,
    u16 max_d,
    u8* rgb) {
    if (!build_img(terrain, dist, w, h, max_d, rgb)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (rivers[i] == 0u || is_water(terrain[i])) {
            continue;
        }
        rgb[i * 3u + 0] = k_riv_r;
        rgb[i * 3u + 1] = k_riv_g;
        rgb[i * 3u + 2] = k_riv_b;
    }
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(k_in_path, map)) {
        std::printf("*** FAILED load %s\n", k_in_path);
        return 1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terrain = map.data();
    u8* rivers = nullptr;
    if (!load_riv(k_in_riv, w, h, &rivers)) {
        std::printf("*** FAILED load %s\n", k_in_riv);
        return 1;
    }
    u16 max_d = 0;
    u16 norm_d = 0;
    u16* ref = Generate_DistanceToOceanCoast::generate(terrain, w, h, &norm_d);
    if (ref == nullptr) {
        delete[] rivers;
        std::printf("*** FAILED naive generate\n");
        return 1;
    }
    delete[] ref;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const i32 tn_i = static_cast<i32>(n);
    AiWbSheet turn_sh(tn_i);
    AiWbSheet scr0(tn_i);
    AiWbSheet scr1(tn_i);
    if (!turn_sh.ok() || !scr0.ok() || !scr1.ok()) {
        delete[] rivers;
        std::printf("*** FAILED whiteboard alloc\n");
        return 1;
    }
    u16* scr[Generate_DistanceToOceanCoast_Tm::k_scr_n] = {
        scr0.get(), scr1.get()};
    const clock_t t0 = clock();
    const bool ok = Generate_DistanceToOceanCoast_Tm::generate(
        terrain, rivers, w, h, turn_sh.get(), scr, &max_d);
    const double gen_sec = static_cast<double>(clock() - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok) {
        delete[] rivers;
        std::printf("*** FAILED generate\n");
        return 1;
    }
    const u16* dist = turn_sh.get();
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        delete[] rivers;
        std::printf("*** FAILED alloc rgb\n");
        return 1;
    }
    build_img(terrain, dist, w, h, norm_d, rgb);
    u8* rgb_riv = new u8[static_cast<size_t>(n) * 3u];
    if (rgb_riv == nullptr) {
        delete[] rivers;
        delete[] rgb;
        std::printf("*** FAILED alloc rgb_riv\n");
        return 1;
    }
    build_img_riv(terrain, rivers, dist, w, h, norm_d, rgb_riv);
    u8* rgb_self = new u8[static_cast<size_t>(n) * 3u];
    if (rgb_self == nullptr) {
        delete[] rivers;
        delete[] rgb;
        delete[] rgb_riv;
        std::printf("*** FAILED alloc rgb_self\n");
        return 1;
    }
    build_img(terrain, dist, w, h, max_d, rgb_self);
    if (!save_rgb_ppm(k_out_path, rgb, w, h)) {
        delete[] rivers;
        delete[] rgb;
        delete[] rgb_riv;
        delete[] rgb_self;
        std::printf("*** FAILED save %s\n", k_out_path);
        return 1;
    }
    if (!save_rgb_ppm(k_out_riv, rgb_riv, w, h)) {
        delete[] rivers;
        delete[] rgb;
        delete[] rgb_riv;
        delete[] rgb_self;
        std::printf("*** FAILED save %s\n", k_out_riv);
        return 1;
    }
    if (!save_rgb_ppm(k_out_self, rgb_self, w, h)) {
        delete[] rivers;
        delete[] rgb;
        delete[] rgb_riv;
        delete[] rgb_self;
        std::printf("*** FAILED save %s\n", k_out_self);
        return 1;
    }
    delete[] rivers;
    delete[] rgb;
    delete[] rgb_riv;
    delete[] rgb_self;
    std::printf("Generate_DistanceToOceanCoast: %ux%u max_depth %u\n",
        static_cast<unsigned>(w),
        static_cast<unsigned>(h),
        static_cast<unsigned>(norm_d));
    std::printf("Generate_DistanceToOceanCoast_Tm: %ux%u max_depth %u\n",
        static_cast<unsigned>(w),
        static_cast<unsigned>(h),
        static_cast<unsigned>(max_d));
    std::printf("norm scope (naive max_depth): %u\n",
        static_cast<unsigned>(norm_d));
    std::printf("norm scope (tm max_depth): %u\n",
        static_cast<unsigned>(max_d));
    std::printf("overlay time: %.6f s\n", gen_sec);
    std::printf("saved: %s\n", k_out_path);
    std::printf("saved: %s\n", k_out_riv);
    std::printf("saved: %s\n", k_out_self);
    std::printf("*** PASSED generate_distance_to_ocean_coast_tm\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
