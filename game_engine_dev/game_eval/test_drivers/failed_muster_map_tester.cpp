//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>

#include "build_adds_array.h"
#include "eval_driver.h"
#include "eval_need.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "failed-muster-map";
static const char* G_PLOT_PY =
    "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/plot_failed_muster_map.py";
static const u16 k_seat_cap = 256u;
static const u16 k_grp_cap = 2048u;
static const u16 k_fail_cap = 256u;

//================================================================================================================================
//=> - Types -
//================================================================================================================================

struct MusterDump {
    u16 m_seat;
    u16 m_sx;
    u16 m_sy;
    u16 m_gx[k_grp_cap];
    u16 m_gy[k_grp_cap];
    u16 m_gn;
    bool m_ok;
};

struct FailEv {
    u16 m_seat;
    u16 m_enemy;
    u32 m_turn;
    MusterDump m_dump;
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

static const u8 k_own_pal[][3] = {
    {220, 40, 40}, {40, 90, 220}, {40, 170, 70}, {220, 110, 30},
    {190, 40, 170}, {30, 170, 170}, {150, 70, 30}, {100, 40, 180},
    {200, 160, 40}, {40, 140, 200}, {160, 50, 90}, {90, 140, 40},
    {137, 25, 25}, {25, 56, 137}, {25, 106, 44}, {137, 69, 19},
    {119, 25, 106}, {19, 106, 106}, {94, 44, 19}, {62, 25, 112},
    {125, 100, 25}, {25, 87, 125}, {100, 31, 56}, {56, 87, 25},
};
static const u16 k_own_pal_n = static_cast<u16>(sizeof(k_own_pal) / sizeof(k_own_pal[0]));
static const u8 k_road_gray = 128u;

static void set_px (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 0; *g = 0; *b = 0;
    if (cls == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1]; *g = TERR_OCEAN[2]; *b = TERR_OCEAN[3];
    } else if (cls == TERR_SEA[0]) {
        *r = TERR_SEA[1]; *g = TERR_SEA[2]; *b = TERR_SEA[3];
    } else if (cls == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1]; *g = TERR_COASTAL[2]; *b = TERR_COASTAL[3];
    } else if (cls == TERR_INLAND_SEA[0]) {
        *r = TERR_INLAND_SEA[1]; *g = TERR_INLAND_SEA[2]; *b = TERR_INLAND_SEA[3];
    } else if (cls == TERR_INLAND_LAKE[0]) {
        *r = TERR_INLAND_LAKE[1]; *g = TERR_INLAND_LAKE[2]; *b = TERR_INLAND_LAKE[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

static bool is_open_water (u8 terr) {
    return terr == TERR_OCEAN[0] || terr == TERR_SEA[0] || terr == TERR_COASTAL[0];
}

static bool is_water_base (u8 terr) {
    return is_open_water(terr) || terr == TERR_INLAND_SEA[0] || terr == TERR_INLAND_LAKE[0];
}

static void shade_own_soft (u8* rgb, u16 w, u16 h, u16 x, u16 y, u16 seat) {
    if (x >= w || y >= h) {
        return;
    }
    const u8* c = k_own_pal[seat % k_own_pal_n];
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = static_cast<u8>((static_cast<u16>(rgb[i + 0]) * 5u + static_cast<u16>(c[0]) * 3u) / 8u);
    rgb[i + 1] = static_cast<u8>((static_cast<u16>(rgb[i + 1]) * 5u + static_cast<u16>(c[1]) * 3u) / 8u);
    rgb[i + 2] = static_cast<u8>((static_cast<u16>(rgb[i + 2]) * 5u + static_cast<u16>(c[2]) * 3u) / 8u);
}

static bool wr_base_ppm (cstr path, const GameArraySimple& map) {
    if (path == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0; u8 g = 0; u8 b = 0;
            const u8 terr = map.get_terrain(x, y);
            if (is_water_base(terr)) {
                terr_rgb(terr, &r, &g, &b);
            } else {
                climate_to_rgb(map.get_climate(x, y), &r, &g, &b);
            }
            if (map.get_river(x, y) != 0) {
                r = 40; g = 100; b = 220;
            }
            if (terr == TERR_MOUNTAINS[0]) {
                r = 120; g = 72; b = 40;
            }
            set_px(rgb, w, h, x, y, r, g, b);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 own = map.get_civ_owner(x, y);
            if (own == U8_KEY_NULL || is_open_water(map.get_terrain(x, y))) {
                continue;
            }
            shade_own_soft(rgb, w, h, x, y, static_cast<u16>(own));
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!road_is_built(map.get_road_typ(x, y))) {
                continue;
            }
            set_px(rgb, w, h, x, y, k_road_gray, k_road_gray, k_road_gray);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_add_typ(x, y) != static_cast<u8>(BUILD_ADD_CITY)) {
                continue;
            }
            set_px(rgb, w, h, x, y, 0, 0, 0);
        }
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

static void dump_clr (MusterDump& d) {
    d.m_seat = U16_KEY_NULL;
    d.m_sx = 0;
    d.m_sy = 0;
    d.m_gn = 0;
    d.m_ok = false;
}

static bool parse_trace (cstr path, FailEv* fails, u16* out_n) {
    if (path == nullptr || fails == nullptr || out_n == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        return false;
    }
    MusterDump last[k_seat_cap];
    for (u16 i = 0; i < k_seat_cap; ++i) {
        dump_clr(last[i]);
    }
    u16 fn = 0;
    char line[1024];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        unsigned sx = 0;
        unsigned sy = 0;
        if (std::sscanf(line, "army state=musterStg_%u_%u", &sx, &sy) == 2) {
            MusterDump cur;
            dump_clr(cur);
            cur.m_sx = static_cast<u16>(sx);
            cur.m_sy = static_cast<u16>(sy);
            while (std::fgets(line, sizeof(line), fp) != nullptr) {
                if (std::strncmp(line, "army state=musterStg_", 21) == 0) {
                    const long back = -static_cast<long>(std::strlen(line));
                    std::fseek(fp, back, SEEK_CUR);
                    break;
                }
                if (std::strncmp(line, "war ", 4) == 0) {
                    const long back = -static_cast<long>(std::strlen(line));
                    std::fseek(fp, back, SEEK_CUR);
                    break;
                }
                if (std::strncmp(line, "army state=musterGroup", 22) != 0) {
                    if (std::strncmp(line, "army state=end", 14) == 0) {
                        continue;
                    }
                    continue;
                }
                unsigned owner = 0;
                unsigned unit = 0;
                unsigned x = 0;
                unsigned y = 0;
                bool got = false;
                while (std::fgets(line, sizeof(line), fp) != nullptr) {
                    if (std::strncmp(line, "army state=end", 14) == 0) {
                        break;
                    }
                    if (std::sscanf(line, "unit state owner=%u unit=%u at=(%u,%u)", &owner, &unit, &x, &y) == 4) {
                        if (!got) {
                            if (cur.m_seat == U16_KEY_NULL) {
                                cur.m_seat = static_cast<u16>(owner);
                            }
                            if (cur.m_gn < k_grp_cap) {
                                cur.m_gx[cur.m_gn] = static_cast<u16>(x);
                                cur.m_gy[cur.m_gn] = static_cast<u16>(y);
                                cur.m_gn++;
                            }
                            got = true;
                        }
                    }
                }
            }
            if (cur.m_seat < k_seat_cap && cur.m_gn > 0u) {
                cur.m_ok = true;
                last[cur.m_seat] = cur;
            }
            continue;
        }
        unsigned seat = 0;
        unsigned enemy = 0;
        unsigned turn = 0;
        if (std::sscanf(line, "war form army fail seat=%u enemy=%u turn=%u", &seat, &enemy, &turn) == 3) {
            if (seat >= k_seat_cap || fn >= k_fail_cap) {
                continue;
            }
            if (!last[seat].m_ok) {
                continue;
            }
            fails[fn].m_seat = static_cast<u16>(seat);
            fails[fn].m_enemy = static_cast<u16>(enemy);
            fails[fn].m_turn = turn;
            fails[fn].m_dump = last[seat];
            fn++;
        }
    }
    std::fclose(fp);
    *out_n = fn;
    return true;
}

static u32 pick_map_turn (const EvalPaths& paths, u32 turn) {
    u32 best = 0;
    bool any = false;
    for (u16 i = 0; i < paths.save_turn_n(); ++i) {
        const u32 t = paths.save_turn_at(i);
        if (t > turn) {
            continue;
        }
        if (!any || t > best) {
            best = t;
            any = true;
        }
    }
    return any ? best : 0u;
}

static bool wr_marks (cstr path, const FailEv& ev, u32 map_turn) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    const MusterDump& d = ev.m_dump;
    std::fprintf(fp, "# seat=%u turn=%u enemy=%u staging=%u,%u map_turn=%u groups=%u\n",
        static_cast<u32>(ev.m_seat),
        static_cast<u32>(ev.m_turn),
        static_cast<u32>(ev.m_enemy),
        static_cast<u32>(d.m_sx),
        static_cast<u32>(d.m_sy),
        static_cast<u32>(map_turn),
        static_cast<u32>(d.m_gn));
    std::fprintf(fp, "staging %u %u\n", static_cast<u32>(d.m_sx), static_cast<u32>(d.m_sy));
    for (u16 i = 0; i < d.m_gn; ++i) {
        std::fprintf(fp, "group %u %u\n", static_cast<u32>(d.m_gx[i]), static_cast<u32>(d.m_gy[i]));
    }
    std::fclose(fp);
    return true;
}

static bool run_plot (cstr base_ppm, cstr marks, cstr out_ppm) {
    char cmd[2048];
    if (std::snprintf(cmd, sizeof(cmd), "python3 \"%s\" \"%s\" \"%s\" \"%s\"",
            G_PLOT_PY, base_ppm, marks, out_ppm) <= 0) {
        return false;
    }
    return std::system(cmd) == 0;
}

//================================================================================================================================
//=> - FailedMusterMapTester -
//================================================================================================================================

class FailedMusterMapTester : public EvalDriver {
public:
    FailedMusterMapTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.trace();
    n.save_seq();
    n.logs().m_war_form_army_fail = 1;
    n.logs().m_army_info = 1;
    n.logs().m_unit_state = 1;
    return n;
}

FailedMusterMapTester::FailedMusterMapTester ()
    : EvalDriver(make_need()) {
}

int FailedMusterMapTester::run () {
    if (!paths().scan_saves()) {
        std::printf("failed_muster_map: scan_saves failed\n");
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("failed_muster_map: bad out paths\n");
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("failed_muster_map: cannot mkdir\n");
        return 1;
    }
    FailEv fails[k_fail_cap];
    u16 fail_n = 0;
    if (!parse_trace(paths().trace_path(), fails, &fail_n)) {
        std::printf("failed_muster_map: cannot parse trace\n");
        return 1;
    }
    if (fail_n == 0u) {
        std::printf("failed_muster_map: no failed musters with dumps\n");
        return 1;
    }
    std::printf("failed_muster_map fails=%u\n", static_cast<u32>(fail_n));
    u16 ok_n = 0;
    for (u16 i = 0; i < fail_n; ++i) {
        const FailEv& ev = fails[i];
        const u32 map_turn = pick_map_turn(paths(), ev.m_turn);
        if (map_turn == 0u) {
            std::printf("failed_muster_map: no map save for seat=%u turn=%u\n",
                static_cast<u32>(ev.m_seat), static_cast<u32>(ev.m_turn));
            continue;
        }
        char map_p[512];
        if (!paths().map_path(map_turn, map_p, sizeof(map_p))) {
            continue;
        }
        char base_p[512];
        char marks_p[512];
        char out_p[512];
        if (std::snprintf(base_p, sizeof(base_p), "%s/fail_%02u_s%u_t%u_base.ppm",
                data_dir, static_cast<u32>(i), static_cast<u32>(ev.m_seat), static_cast<u32>(ev.m_turn)) <= 0
            || std::snprintf(marks_p, sizeof(marks_p), "%s/fail_%02u_s%u_t%u_marks.txt",
                data_dir, static_cast<u32>(i), static_cast<u32>(ev.m_seat), static_cast<u32>(ev.m_turn)) <= 0
            || std::snprintf(out_p, sizeof(out_p), "%s/failed_muster_s%u_t%u.ppm",
                data_dir, static_cast<u32>(ev.m_seat), static_cast<u32>(ev.m_turn)) <= 0) {
            continue;
        }
        GameArraySimple map;
        if (!GameIo::load_map_tiles(map_p, map)) {
            std::printf("failed_muster_map: load_map failed %s\n", map_p);
            continue;
        }
        if (!wr_base_ppm(base_p, map)) {
            std::printf("failed_muster_map: base ppm failed\n");
            continue;
        }
        if (!wr_marks(marks_p, ev, map_turn)) {
            std::printf("failed_muster_map: marks write failed\n");
            continue;
        }
        std::fflush(stdout);
        if (!run_plot(base_p, marks_p, out_p)) {
            std::printf("failed_muster_map: plot failed %s\n", out_p);
            continue;
        }
        std::printf("wrote %s groups=%u\n", out_p, static_cast<u32>(ev.m_dump.m_gn));
        ok_n++;
    }
    std::printf("failed_muster_map ok maps=%u/%u\n", static_cast<u32>(ok_n), static_cast<u32>(fail_n));
    return (ok_n > 0u) ? 0 : 1;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    FailedMusterMapTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
