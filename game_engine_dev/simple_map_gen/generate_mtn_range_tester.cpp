//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "game_primitives.h"
#include "generate_area_index.h"
#include "generate_generic_depth_overlay.h"
#include "generate_mtn_range.h"
#include "generator_constants.h"
#include "generator_whiteboard.h"
#include "map_loader.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_path = "/home/w/Projects/simple-map-gen/mtn-range-hills.ppm";
static const i32 g_coverage_perc = 20;
static const i32 g_target_depth = 5;

struct BrushPt {
    i8 dx;
    i8 dy;
};

struct Brush {
    const BrushPt* pts;
    u8 n;
};

static const BrushPt g_br_cross[] = {
    {0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}};
static const BrushPt g_br_x[] = {
    {0, 0}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
static const BrushPt g_br_star[] = {
    {0, 0}, {0, -2}, {-1, -1}, {1, -1}, {-2, 0}, {2, 0}, {-1, 1}, {1, 1}, {0, 2}};
static const BrushPt g_br_ring[] = {
    {0, -2}, {-1, -1}, {1, -1}, {-2, 0}, {2, 0}, {-1, 1}, {1, 1}, {0, 2}};
static const BrushPt g_br_diamond[] = {
    {0, 0}, {0, -1}, {-1, 0}, {1, 0}, {0, 1}, {0, -2}, {-1, -1}, {1, -1},
    {-2, 0}, {2, 0}, {-1, 1}, {1, 1}, {0, 2}};
static const BrushPt g_br_tee[] = {
    {0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, -2}, {-1, -2}, {1, -2}};
static const BrushPt g_br_hook[] = {
    {0, 0}, {-1, 0}, {-2, 0}, {0, -1}, {0, -2}, {1, -1}};
static const BrushPt g_br_blob[] = {
    {0, 0}, {-1, 0}, {1, 0}, {0, 1}, {-1, 1}, {1, -1}, {0, -2}};

static const Brush g_brushes[] = {
    {g_br_cross, 5},
    {g_br_x, 5},
    {g_br_star, 9},
    {g_br_ring, 8},
    {g_br_diamond, 13},
    {g_br_tee, 7},
    {g_br_hook, 6},
    {g_br_blob, 7}};
static const u8 g_brush_n = static_cast<u8>(sizeof(g_brushes) / sizeof(g_brushes[0]));

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;

static u32 rng_u32 (u32* s) {
    *s = *s * 1103515245u + 12345u;
    return (*s >> 16u) & 0x7FFFu;
}

static bool find_depth_seed (
    const u8* terrain,
    const u16* dist,
    u16 w,
    u16 h,
    u16 sx,
    u16 sy,
    u16* ox,
    u16* oy) 
{
    const u32 wi = static_cast<u32>(w);
    const u32 n = wi * static_cast<u32>(h);
    u8* vis = new u8[n];
    if (vis == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        vis[i] = 0;
    }
    u32* q = new u32[n];
    if (q == nullptr) {
        delete[] vis;
        return false;
    }
    u32 qn = 0;
    u32 qh = 0;
    const u32 seed = static_cast<u32>(sy) * wi + static_cast<u32>(sx);
    q[qn++] = seed;
    vis[seed] = 1;
    bool found = false;
    while (qh < qn && !found) {
        const u32 i = q[qh++];
        if (terrain[i] >= TERR_HILLS[0] && dist[i] != k_inf) {
            const u16 py = static_cast<u16>(i / wi);
            const u16 px = static_cast<u16>(i - static_cast<u32>(py) * wi);
            *ox = px;
            *oy = py;
            found = true;
            break;
        }
        const u16 py = static_cast<u16>(i / wi);
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * wi);
        if (px > 0) {
            const u32 j = i - 1u;
            if (vis[j] == 0 && terrain[j] >= TERR_HILLS[0]) {
                vis[j] = 1;
                q[qn++] = j;
            }
        }
        if (static_cast<u32>(px) + 1u < wi) {
            const u32 j = i + 1u;
            if (vis[j] == 0 && terrain[j] >= TERR_HILLS[0]) {
                vis[j] = 1;
                q[qn++] = j;
            }
        }
        if (py > 0) {
            const u32 j = i - wi;
            if (vis[j] == 0 && terrain[j] >= TERR_HILLS[0]) {
                vis[j] = 1;
                q[qn++] = j;
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + wi;
            if (vis[j] == 0 && terrain[j] >= TERR_HILLS[0]) {
                vis[j] = 1;
                q[qn++] = j;
            }
        }
    }
    delete[] q;
    delete[] vis;
    return found;
}

static void stamp_brush (u8* terrain, u16 w, u16 h, i32 cx, i32 cy, const Brush* br) {
    for (u8 k = 0; k < br->n; ++k) {
        const i32 x = cx + static_cast<i32>(br->pts[k].dx);
        const i32 y = cy + static_cast<i32>(br->pts[k].dy);
        if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
            continue;
        }
        const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
        if (terrain[i] == TERR_HILLS[0]) {
            terrain[i] = TERR_MOUNTAINS[0];
        }
    }
}

static void apply_mtn_pts (u8* terrain, u16 w, u16 h, const MtnRangeResult* res, u32 seed) {
    if (terrain == nullptr || res == nullptr || res->pts == nullptr) {
        return;
    }
    u32 rng = seed;
    for (u32 k = 0; k < res->pt_n; ++k) {
        const MtnRangePt* p = &res->pts[k];
        const u32 pick = rng_u32(&rng) % static_cast<u32>(g_brush_n);
        stamp_brush(
            terrain,
            w,
            h,
            static_cast<i32>(p->x),
            static_cast<i32>(p->y),
            &g_brushes[pick]);
    }
}

static i32 test_generate_mtn_range_basic () {
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(g_map_path, map)) {
        std::printf("failed to load map: %s\n", g_map_path);
        return -1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terrain = map.data();
    if (terrain == nullptr || w == 0 || h == 0) {
        std::printf("invalid map data\n");
        return -1;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    AreaIdxResult* areas = Generate_AreaIndex::generate_ge(terrain, w, h, TERR_HILLS[0]);
    if (areas == nullptr || areas->area_n == 0u) {
        Generate_AreaIndex::free_result(areas);
        std::printf("Generate_AreaIndex::generate_ge failed\n");
        return -1;
    }
    u16* dist = Generate_GenericDepthOverlay::generate_ge_dist(terrain, w, h, TERR_HILLS[0]);
    if (dist == nullptr) {
        Generate_AreaIndex::free_result(areas);
        std::printf("Generate_GenericDepthOverlay::generate_ge_dist failed\n");
        return -1;
    }
    u8* out_terrain = new u8[n];
    if (out_terrain == nullptr) {
        delete[] dist;
        Generate_AreaIndex::free_result(areas);
        return -1;
    }
    std::memcpy(out_terrain, terrain, static_cast<size_t>(n));
    const u32 use_n = (areas->area_n < 3u) ? areas->area_n : 3u;
    i32 rc = 0;
    for (u32 k = 0; k < use_n; ++k) {
        const AreaIdxEntry* e = &areas->areas[k];
        u16 seed_x = e->x;
        u16 seed_y = e->y;
        if (!find_depth_seed(terrain, dist, w, h, e->x, e->y, &seed_x, &seed_y)) {
            std::printf(
                "no depth seed  area=%u  seed=(%u,%u)  size=%u\n",
                k,
                e->x,
                e->y,
                e->size);
            rc |= -1;
            continue;
        }
        MtnRangeResult* range = Generate_MtnRange::generate(
            dist,
            w,
            h,
            g_coverage_perc,
            g_target_depth,
            seed_x,
            seed_y);
        if (range == nullptr) {
            std::printf(
                "Generate_MtnRange failed  area=%u  seed=(%u,%u)  size=%u\n",
                k,
                e->x,
                e->y,
                e->size);
            rc |= -1;
            continue;
        }
        std::printf(
            "area=%u  seed=(%u,%u)  depth_seed=(%u,%u)  size=%u  range_pts=%u\n",
            k,
            e->x,
            e->y,
            seed_x,
            seed_y,
            e->size,
            range->pt_n);
        apply_mtn_pts(
            out_terrain,
            w,
            h,
            range,
            static_cast<u32>(seed_x) + static_cast<u32>(seed_y) * static_cast<u32>(w) + k * 7919u);
        Generate_MtnRange::free_result(range);
    }
    delete[] dist;
    Generate_AreaIndex::free_result(areas);
    GeneratorWhiteboard::dealloc();
    MapTerrainData out_map;
    if (!out_map.assign_raw(w, h, out_terrain)) {
        delete[] out_terrain;
        std::printf("failed to assign output terrain\n");
        return -1;
    }
    delete[] out_terrain;
    if (!out_map.save_terrain_ppm(g_out_path)) {
        std::printf("failed to save: %s\n", g_out_path);
        return -1;
    }
    std::printf("saved=%s\n", g_out_path);
    return rc;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main () {
    return test_generate_mtn_range_basic();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
