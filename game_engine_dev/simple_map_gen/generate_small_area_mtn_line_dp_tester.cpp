//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cerrno>
#include <cstring>
#include <map>
#include <tuple>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>

#include "game_primitives.h"
#include "generate_river_lines.h"
#include "generate_river_network.h"
#include "generate_river_pts.h"
#include "generate_river_sectors.h"
#include "generate_small_area_mtn_line_dp.h"
#include "generate_small_shape.h"
#include "generate_watershed_mtn_lines.h"
#include "generator_constants.h"
#include "map_loader.h"
#include "map_terrain_validate.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_watershed_path = "/home/w/Projects/simple-map-gen/small-area-mtn-line-dp-watersheds.ppm";
static const char* g_pts_path = "/home/w/Projects/simple-map-gen/small-area-mtn-line-dp-points.ppm";
static const char* g_stamp_path = "/home/w/Projects/simple-map-gen/small-area-mtn-line-dp-stamped.ppm";
static const char* g_shape_dir = "/home/w/Projects/simple-map-gen/small-area-mtn-line-dp-shapes";
static const i32 g_hills_perc = 90;
static const i32 g_tile_cap_perc = 30;
static const i32 g_min_dist_perc = 2;
static const u32 g_min_split_tiles = 100;
static const f64 g_split_angle_deg = 150.0;
static const f64 g_min_seg_len = 8.0;
static const u32 g_max_stamp_n = 60;
static const f64 g_oversize_factor = 1.2;
static const u16 g_oversize_px = 10;

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h);

static bool mkdirs (cstr path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    char buf[384];
    std::strncpy(buf, path, sizeof(buf) - 1u);
    buf[sizeof(buf) - 1u] = '\0';
    for (char* p = buf + 1; *p != '\0'; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
                return false;
            }
            *p = '/';
        }
    }
    return mkdir(buf, 0755) == 0 || errno == EEXIST;
}

static void draw_thick_line (
    u8* rgb,
    u16 w,
    u16 h,
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2,
    u8 r,
    u8 g,
    u8 b,
    i32 thick) 
{
    i32 dx = x1 - x2;
    if (dx < 0) {
        dx = -dx;
    }
    i32 dy = y1 - y2;
    if (dy < 0) {
        dy = -dy;
    }
    i32 sx = (x1 < x2) ? 1 : -1;
    i32 sy = (y1 < y2) ? 1 : -1;
    i32 err = dx - dy;
    i32 x = x1;
    i32 y = y1;
    while (true) {
        for (i32 ty = -thick; ty <= thick; ++ty) {
            for (i32 tx = -thick; tx <= thick; ++tx) {
                if (tx * tx + ty * ty > thick * thick) {
                    continue;
                }
                const i32 px = x + tx;
                const i32 py = y + ty;
                if (px < 0 || py < 0 || static_cast<u32>(px) >= static_cast<u32>(w) || static_cast<u32>(py) >= static_cast<u32>(h)) {
                    continue;
                }
                const u32 p = (static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px)) * 3u;
                rgb[p + 0] = r;
                rgb[p + 1] = g;
                rgb[p + 2] = b;
            }
        }
        if (x == x2 && y == y2) {
            break;
        }
        const i32 e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

static bool save_watershed_viz (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverSectorsResult* sectors,
    const RiverNetworkResult* network,
    u32 seed) 
{
    if (path == nullptr || terrain == nullptr || sectors == nullptr || network == nullptr
        || sectors->overlay == nullptr || sectors->nodes == nullptr || network->overlay == nullptr
        || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        MapTerrainValidate::rgb_from_class(terrain[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    std::srand(seed);
    std::map<u16, std::tuple<u8, u8, u8>> sys_clr;
    for (u16 bi = 0; bi < network->basin_n; ++bi) {
        const u16 bid = network->basins[bi].idx;
        if (sys_clr.find(bid) == sys_clr.end()) {
            sys_clr[bid] = {
                static_cast<u8>(50 + (std::rand() % 151)),
                static_cast<u8>(50 + (std::rand() % 151)),
                static_cast<u8>(50 + (std::rand() % 151))};
        }
    }
    const u8 line_r = 0;
    const u8 line_g = 100;
    const u8 line_b = 255;
    if (network->conns != nullptr) {
        for (u16 ci = 0; ci < network->conn_n; ++ci) {
            const u16 ia = network->conns[ci].a;
            const u16 ib = network->conns[ci].b;
            draw_thick_line(
                rgb,
                w,
                h,
                sectors->nodes[ia].cx,
                sectors->nodes[ia].cy,
                sectors->nodes[ib].cx,
                sectors->nodes[ib].cy,
                line_r,
                line_g,
                line_b,
                1);
        }
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 bid = network->overlay[i];
        if (bid == static_cast<u16>(RIVER_BASIN_NONE)) {
            continue;
        }
        std::map<u16, std::tuple<u8, u8, u8>>::const_iterator it = sys_clr.find(bid);
        if (it == sys_clr.end()) {
            continue;
        }
        const u32 p = i * 3u;
        rgb[p + 0] = std::get<0>(it->second);
        rgb[p + 1] = std::get<1>(it->second);
        rgb[p + 2] = std::get<2>(it->second);
    }
    if (network->conns != nullptr) {
        for (u16 ci = 0; ci < network->conn_n; ++ci) {
            const u16 ia = network->conns[ci].a;
            const u16 ib = network->conns[ci].b;
            draw_thick_line(
                rgb,
                w,
                h,
                sectors->nodes[ia].cx,
                sectors->nodes[ia].cy,
                sectors->nodes[ib].cx,
                sectors->nodes[ib].cy,
                line_r,
                line_g,
                line_b,
                1);
        }
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_shape_land (u8 cls) {
    return !is_water(cls);
}

static bool is_plains (u8 cls) {
    return cls == TERR_PLAINS[0];
}

static bool is_hills (u8 cls) {
    return cls == TERR_HILLS[0];
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    if (path == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void set_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    rgb[i * 3u + 0] = r;
    rgb[i * 3u + 1] = g;
    rgb[i * 3u + 2] = b;
}

static void draw_dot (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            set_px(rgb, w, h, x + dx, y + dy, r, g, b);
        }
    }
}

static void draw_line (u8* rgb, u16 w, u16 h, i32 x0, i32 y0, i32 x1, i32 y1, u8 r, u8 g, u8 b) {
    i32 dx = x1 - x0;
    i32 dy = y1 - y0;
    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    i32 sx = (x0 < x1) ? 1 : -1;
    i32 sy = (y0 < y1) ? 1 : -1;
    i32 err = dx - dy;
    i32 x = x0;
    i32 y = y0;
    while (true) {
        set_px(rgb, w, h, x, y, r, g, b);
        if (x == x1 && y == y1) {
            break;
        }
        const i32 e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

static bool side_hills_ok (u16 pln, u16 hil, i32 perc) {
    const u32 tot = static_cast<u32>(pln) + static_cast<u32>(hil);
    if (tot == 0) {
        return false;
    }
    return static_cast<u32>(hil) * 100u >= tot * static_cast<u32>(perc);
}

static bool seg_hills_ok (const WatershedBorderSeg& s, i32 perc) {
    return side_hills_ok(s.a_plains, s.a_hills, perc) || side_hills_ok(s.b_plains, s.b_hills, perc);
}

static u32 tile_cap_n (u32 border_tile_n, i32 perc) {
    if (border_tile_n == 0 || perc <= 0) {
        return 0;
    }
    if (perc >= 100) {
        return border_tile_n;
    }
    return (border_tile_n * static_cast<u32>(perc) + 99u) / 100u;
}

static u32 max_mouth_dist (const WatershedMtnLinesResult* mtns) {
    if (mtns == nullptr || mtns->segs == nullptr || mtns->seg_n == 0) {
        return 0;
    }
    return mtns->segs[mtns->seg_n - 1].mouth_dist;
}

static bool seg_dist_too_short (u32 mouth_dist, u32 max_dist, i32 min_perc) {
    if (max_dist == 0 || min_perc <= 0) {
        return false;
    }
    if (min_perc >= 100) {
        return false;
    }
    return mouth_dist * 100u < max_dist * static_cast<u32>(min_perc);
}

static i32 seg_angle_deg (i32 x0, i32 y0, i32 x1, i32 y1) {
    const f64 dx = static_cast<f64>(x1 - x0);
    const f64 dy = static_cast<f64>(y1 - y0);
    const f64 ang = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846;
    return static_cast<i32>(std::lround(ang));
}

static u16 seg_width (f64 dist) {
    u16 sw = static_cast<u16>(std::lround(dist));
    if (sw < 1u) {
        sw = 1u;
    }
    return sw;
}

static u16 seg_height (u16 width) {
    u16 sh = static_cast<u16>(std::lround(static_cast<f64>(width) * 2.0 / 5.0));
    if (sh < 1u) {
        sh = 1u;
    }
    return sh;
}

static u16 oversize_dim (u16 base, f64 fac, u16 add_px) {
    f64 v = static_cast<f64>(base) * fac + static_cast<f64>(add_px);
    i32 r = static_cast<i32>(std::lround(v));
    if (r < 1) {
        r = 1;
    }
    return static_cast<u16>(r);
}

static u32 stamp_shape (
    u8* terrain,
    u16 mw,
    u16 mh,
    const Generate_SmallShape& gen,
    i32 off_x,
    i32 off_y) 
{
    const u16 sw = gen.width();
    const u16 sh = gen.height();
    const u8* st = gen.terrain().data();
    if (st == nullptr || sw == 0 || sh == 0) {
        return 0;
    }
    u32 land_n = 0;
    u32 stamp_n = 0;
    for (u16 sy = 0; sy < sh; ++sy) {
        for (u16 sx = 0; sx < sw; ++sx) {
            const u8 cls = st[static_cast<u32>(sy) * static_cast<u32>(sw) + static_cast<u32>(sx)];
            if (!is_shape_land(cls)) {
                continue;
            }
            ++land_n;
            const i32 wx = off_x + static_cast<i32>(sx);
            const i32 wy = off_y + static_cast<i32>(sy);
            if (wx < 0 || wy < 0 || wx >= static_cast<i32>(mw) || wy >= static_cast<i32>(mh)) {
                continue;
            }
            const u32 wi = static_cast<u32>(wy) * static_cast<u32>(mw) + static_cast<u32>(wx);
            if (is_water(terrain[wi])) {
                continue;
            }
            terrain[wi] = TERR_MOUNTAINS[0];
            ++stamp_n;
        }
    }
    std::printf(
        "    stamp shape land px %u stamped px %u (black land -> mountain)\n",
        land_n,
        stamp_n);
    return stamp_n;
}

static bool build_rgb_base (const u8* terrain, u16 w, u16 h, u8* rgb) {
    if (terrain == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        MapTerrainValidate::rgb_from_class(terrain[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    return true;
}

static void draw_dp_overlay (u8* rgb, u16 w, u16 h, const SmallAreaMtnLineDpResult* dp) {
    if (rgb == nullptr || dp == nullptr || dp->chains == nullptr) {
        return;
    }
    for (u16 ci = 0; ci < dp->chain_n; ++ci) {
        const SmallAreaMtnLineDpChain& ch = dp->chains[ci];
        if (ch.border_tiles != nullptr) {
            for (u32 bi = 0; bi < ch.border_tile_n; ++bi) {
                set_px(rgb, w, h, ch.border_tiles[bi].x, ch.border_tiles[bi].y, 0, 0, 0);
            }
        }
        if (ch.pts == nullptr || ch.pt_n < 2u) {
            continue;
        }
        for (u16 pi = 0; pi + 1u < ch.pt_n; ++pi) {
            draw_line(
                rgb,
                w,
                h,
                ch.pts[pi].x,
                ch.pts[pi].y,
                ch.pts[pi + 1u].x,
                ch.pts[pi + 1u].y,
                255,
                255,
                0);
        }
        for (u16 pi = 0; pi < ch.pt_n; ++pi) {
            draw_dot(rgb, w, h, ch.pts[pi].x, ch.pts[pi].y, 255, 0, 0);
        }
        draw_dot(rgb, w, h, ch.end_a.x, ch.end_a.y, 0, 0, 255);
        draw_dot(rgb, w, h, ch.end_b.x, ch.end_b.y, 0, 0, 255);
    }
}

static u32 stamp_dp_shapes (
    u8* terrain,
    u16 w,
    u16 h,
    const SmallAreaMtnLineDpResult* dp,
    u32 seed,
    u16 seg_i) 
{
    if (terrain == nullptr || dp == nullptr || dp->chains == nullptr) {
        return 0;
    }
    if (!mkdirs(g_shape_dir)) {
        std::printf("failed to create shape dir: %s\n", g_shape_dir);
    }
    u32 shape_n = 0;
    u32 stamp_px_n = 0;
    for (u16 ci = 0; ci < dp->chain_n; ++ci) {
        const SmallAreaMtnLineDpChain& ch = dp->chains[ci];
        if (ch.pts == nullptr || ch.pt_n < 2u) {
            continue;
        }
        for (u16 pi = 0; pi + 1u < ch.pt_n; ++pi) {
            const i32 x0 = ch.pts[pi].x;
            const i32 y0 = ch.pts[pi].y;
            const i32 x1 = ch.pts[pi + 1u].x;
            const i32 y1 = ch.pts[pi + 1u].y;
            const f64 dx = static_cast<f64>(x1 - x0);
            const f64 dy = static_cast<f64>(y1 - y0);
            const f64 dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 1.0) {
                continue;
            }
            const u16 base_sw = seg_width(dist);
            const u16 base_sh = seg_height(base_sw);
            const u16 sw = oversize_dim(base_sw, g_oversize_factor, g_oversize_px);
            const u16 sh = oversize_dim(base_sh, g_oversize_factor, g_oversize_px);
            const i32 mid_x = (x0 + x1) / 2;
            const i32 mid_y = (y0 + y1) / 2;
            const i32 ang = seg_angle_deg(x0, y0, x1, y1);
            SmallShapeParams sp = {};
            sp.m_seed = seed ^ (static_cast<u32>(ci) * 73856093u) ^ (static_cast<u32>(pi) * 19349663u);
            sp.m_width = sw;
            sp.m_height = sh;
            sp.m_angle_deg = ang;
            Generate_SmallShape shape(sp);
            if (!shape.generate() || !shape.is_valid()) {
                continue;
            }
            const u16 out_w = shape.width();
            const u16 out_h = shape.height();
            const i32 off_x = mid_x - static_cast<i32>(out_w) / 2;
            const i32 off_y = mid_y - static_cast<i32>(out_h) / 2;
            char shape_path[384];
            std::snprintf(
                shape_path,
                sizeof(shape_path),
                "%s/small-area-mtn-line-dp-shape-seg%03u-c%02u-p%02u.ppm",
                g_shape_dir,
                seg_i,
                ci,
                pi);
            std::printf(
                "    small shape base %ux%u request %ux%u canvas %u output %ux%u off (%d,%d) angle %d\n",
                base_sw,
                base_sh,
                sw,
                sh,
                shape.canvas_size(),
                out_w,
                out_h,
                off_x,
                off_y,
                ang);
            if (!shape.save_output(shape_path)) {
                std::printf("    failed to save shape: %s\n", shape_path);
            } else {
                std::printf("    saved shape (black=land): %s\n", shape_path);
            }
            const u32 px_n = stamp_shape(terrain, w, h, shape, off_x, off_y);
            stamp_px_n += px_n;
            if (px_n > 0) {
                ++shape_n;
            }
        }
    }
    if (shape_n > 0) {
        std::printf("  stamped %u shapes, %u pixels\n", shape_n, stamp_px_n);
    }
    return stamp_px_n;
}

static bool save_stamp_viz (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverLinesResult* lines) 
{
    if (path == nullptr || terrain == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        MapTerrainValidate::rgb_from_class(terrain[i], &r, &g, &b);
        if (lines != nullptr && lines->overlay != nullptr && lines->overlay[i] != 0) {
            r = 0;
            g = 0;
            b = 255;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static void print_seg (const WatershedMtnLinesResult* mtns, u16 si) {
    if (mtns == nullptr || mtns->segs == nullptr || si >= mtns->seg_n) {
        return;
    }
    const WatershedBorderSeg& s = mtns->segs[si];
    std::printf("  seg %u: basins %u-%u ov %u mouths (%u,%u)-(%u,%u) dist %u tiles %u\n",
        si,
        s.basin_a,
        s.basin_b,
        s.ov_idx,
        s.mouth_ax,
        s.mouth_ay,
        s.mouth_bx,
        s.mouth_by,
        s.mouth_dist,
        s.tile_n);
    std::printf("    a pln/hil %u/%u  b pln/hil %u/%u  hills ok %s\n",
        s.a_plains,
        s.a_hills,
        s.b_plains,
        s.b_hills,
        seg_hills_ok(s, g_hills_perc) ? "yes" : "no");
}

//================================================================================================================================
//=> - Test -
//================================================================================================================================

i32 test_generate_small_area_mtn_line_dp_basic (u32 seed) {
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(g_map_path, map)) {
        std::printf("failed to load map: %s\n", g_map_path);
        return -1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terrain_in = map.data();
    if (terrain_in == nullptr || w == 0 || h == 0) {
        std::printf("invalid map data\n");
        return -1;
    }
    const u32 terr_n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[terr_n];
    u8* terrain_pts = new u8[terr_n];
    if (terrain == nullptr || terrain_pts == nullptr) {
        std::printf("out of memory\n");
        delete[] terrain;
        delete[] terrain_pts;
        return -1;
    }
    for (u32 i = 0; i < terr_n; ++i) {
        terrain[i] = terrain_in[i];
        terrain_pts[i] = terrain_in[i];
    }
    RiverPtsResult* pts = Generate_RiverPts::generate(terrain, w, h, seed);
    if (pts == nullptr) {
        std::printf("Generate_RiverPts failed\n");
        delete[] terrain;
        delete[] terrain_pts;
        return -1;
    }
    RiverSectorsResult* sectors = Generate_RiverSectors::generate(terrain, w, h, pts, seed);
    Generate_RiverPts::free_result(pts);
    if (sectors == nullptr) {
        std::printf("Generate_RiverSectors failed\n");
        delete[] terrain;
        delete[] terrain_pts;
        return -1;
    }
    RiverNetworkResult* network = Generate_RiverNetwork::generate(terrain, w, h, sectors);
    if (network == nullptr) {
        Generate_RiverSectors::free_result(sectors);
        delete[] terrain;
        delete[] terrain_pts;
        std::printf("Generate_RiverNetwork failed\n");
        return -1;
    }
    RiverLinesResult* lines = Generate_RiverLines::generate(terrain, w, h, sectors, network, seed);
    if (lines == nullptr) {
        Generate_RiverNetwork::free_result(network);
        Generate_RiverSectors::free_result(sectors);
        delete[] terrain;
        delete[] terrain_pts;
        std::printf("Generate_RiverLines failed\n");
        return -1;
    }
    WatershedMtnLinesResult* mtns = Generate_WatershedMtnLines::generate(terrain, w, h, network);
    if (mtns == nullptr) {
        Generate_RiverLines::free_result(lines);
        Generate_RiverNetwork::free_result(network);
        Generate_RiverSectors::free_result(sectors);
        delete[] terrain;
        delete[] terrain_pts;
        std::printf("Generate_WatershedMtnLines failed\n");
        return -1;
    }
    const bool ok_watershed = save_watershed_viz(g_watershed_path, terrain_pts, w, h, sectors, network, seed);
    if (!ok_watershed) {
        std::printf("failed to save: %s\n", g_watershed_path);
    } else {
        std::printf("saved: %s (%u watersheds)\n", g_watershed_path, network->basin_n);
    }
    std::printf("watershed border segments: %u  border tiles: %u\n", mtns->seg_n, mtns->border_tile_n);
    const u32 tile_cap = tile_cap_n(mtns->border_tile_n, g_tile_cap_perc);
    const u32 max_dist = max_mouth_dist(mtns);
    std::printf(
        "selection: from end, hills>=%d%% on a side, tile cap %d%% (%u), max stamp %u, oversize %.2f +%u px\n",
        g_hills_perc,
        g_tile_cap_perc,
        tile_cap,
        g_max_stamp_n,
        g_oversize_factor,
        g_oversize_px);
    SmallAreaMtnLineDpParams dp_params = {};
    dp_params.m_min_split_tile_n = g_min_split_tiles;
    dp_params.m_split_angle_deg = g_split_angle_deg;
    dp_params.m_min_seg_len = g_min_seg_len;
    std::vector<SmallAreaMtnLineDpResult*> dp_viz_list;
    u32 covered = 0;
    u32 tried_n = 0;
    u32 stamped_n = 0;
    u32 total_stamp_px = 0;
    double dp_usec = 0.0;
    for (u16 si = mtns->seg_n; si > 0; --si) {
        const u16 idx = static_cast<u16>(si - 1u);
        const WatershedBorderSeg& s = mtns->segs[idx];
        if (seg_dist_too_short(s.mouth_dist, max_dist, g_min_dist_perc)) {
            std::printf("stop: seg %u below min dist %d%%\n", idx, g_min_dist_perc);
            break;
        }
        if (!seg_hills_ok(s, g_hills_perc)) {
            continue;
        }
        if (s.tile_n == 0) {
            continue;
        }
        if (covered + s.tile_n > tile_cap) {
            std::printf("stop: seg %u would exceed tile cap\n", idx);
            break;
        }
        ++tried_n;
        std::printf("try seg %u:\n", idx);
        print_seg(mtns, idx);
        const clock_t t0 = clock();
        SmallAreaMtnLineDpResult* dp = Generate_SmallAreaMtnLineDp::generate(mtns, idx, dp_params);
        const clock_t t1 = clock();
        dp_usec += static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC) * 1000000.0;
        if (dp == nullptr) {
            std::printf("  Generate_SmallAreaMtnLineDp failed\n");
            continue;
        }
        std::printf("  dp chains %u\n", dp->chain_n);
        const u32 stamp_px = stamp_dp_shapes(terrain, w, h, dp, seed, idx);
        if (stamp_px > 0) {
            ++stamped_n;
            total_stamp_px += stamp_px;
            covered += s.tile_n;
            dp_viz_list.push_back(dp);
            std::printf("  covered %u / %u border tiles\n", covered, tile_cap);
            if (stamped_n >= g_max_stamp_n) {
                std::printf("stop: reached max stamp %u\n", g_max_stamp_n);
                break;
            }
        } else {
            std::printf("  no shapes stamped\n");
            Generate_SmallAreaMtnLineDp::free_result(dp);
        }
    }
    std::printf(
        "selection done: tried %u  stamped %u  total pixels %u  dp total %.0f us\n",
        tried_n,
        stamped_n,
        total_stamp_px,
        dp_usec);
    if (dp_viz_list.empty()) {
        Generate_WatershedMtnLines::free_result(mtns);
        Generate_RiverLines::free_result(lines);
        Generate_RiverNetwork::free_result(network);
        Generate_RiverSectors::free_result(sectors);
        delete[] terrain;
        delete[] terrain_pts;
        std::printf("no watershed pair stamped\n");
        return -1;
    }
    u8* rgb_pts = new u8[static_cast<size_t>(terr_n) * 3u];
    if (rgb_pts == nullptr) {
        for (size_t vi = 0; vi < dp_viz_list.size(); ++vi) {
            Generate_SmallAreaMtnLineDp::free_result(dp_viz_list[vi]);
        }
        Generate_WatershedMtnLines::free_result(mtns);
        Generate_RiverLines::free_result(lines);
        Generate_RiverNetwork::free_result(network);
        Generate_RiverSectors::free_result(sectors);
        delete[] terrain;
        delete[] terrain_pts;
        std::printf("out of memory\n");
        return -1;
    }
    build_rgb_base(terrain_pts, w, h, rgb_pts);
    for (size_t vi = 0; vi < dp_viz_list.size(); ++vi) {
        draw_dp_overlay(rgb_pts, w, h, dp_viz_list[vi]);
    }
    const bool ok_pts = save_rgb_ppm(g_pts_path, rgb_pts, w, h);
    delete[] rgb_pts;
    const bool ok_stamp = save_stamp_viz(g_stamp_path, terrain, w, h, lines);
    for (size_t vi = 0; vi < dp_viz_list.size(); ++vi) {
        Generate_SmallAreaMtnLineDp::free_result(dp_viz_list[vi]);
    }
    Generate_WatershedMtnLines::free_result(mtns);
    Generate_RiverLines::free_result(lines);
    Generate_RiverNetwork::free_result(network);
    Generate_RiverSectors::free_result(sectors);
    delete[] terrain;
    delete[] terrain_pts;
    if (!ok_pts) {
        std::printf("failed to save: %s\n", g_pts_path);
        return -1;
    }
    if (!ok_stamp) {
        std::printf("failed to save: %s\n", g_stamp_path);
        return -1;
    }
    std::printf("saved: %s\n", g_pts_path);
    std::printf("saved: %s\n", g_stamp_path);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    u32 seed = 42;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    return test_generate_small_area_mtn_line_dp_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
