//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "ai_whiteboard.h"
#include "generate_distance_p2p_mk4.h"
#include "game_map_defs.h"
#include "walk_p2p_mk4.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_w = 10u;
static const u16 k_h = 10u;
static const u32 k_tile_n = static_cast<u32>(k_w) * static_cast<u32>(k_h);
static const u16 k_dist_sent = Generate_DistanceP2P_Mk4::k_dist_sent;

static const u8 k_ck_plain = 0u;
static const u8 k_ck_hill = 1u;
static const u8 k_ck_mtn = 2u;
static const u8 k_ck_riv = 3u;

static const cstr k_ansi_rst = "\033[0m";
static const cstr k_ansi_grn = "\033[32m";
static const cstr k_ansi_blu = "\033[94m";

struct WalkSimRes {
    u32 path_len;
    u32 game_turns;
    bool traced;
};

struct CtrlCase {
    cstr m_name;
    const u8* m_ck;
    u16 m_src_x;
    u16 m_src_y;
    u16 m_dst_x;
    u16 m_dst_y;
};

static const u8 k_map_empty[k_tile_n] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


static const u8 k_map_riv_straight[k_tile_n] = {
    0, 0, 3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 3, 3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 3, 3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 3, 3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 3, 3, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 3, 0, 0, 0,
};

static const u8 k_map_riv_run[k_tile_n] = {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 3, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
};

static const u8 k_map_riv_fork[k_tile_n] = {
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 0, 0, 0, 3, 3, 3, 0, 0, 2,
    2, 0, 0, 0, 3, 0, 3, 0, 0, 2,
    2, 0, 0, 0, 3, 0, 3, 0, 0, 2,
    2, 0, 0, 0, 3, 3, 3, 0, 0, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
};

static const CtrlCase k_cases[] = {
    //{"empty", k_map_empty, 0u, 0u, 0u, 0u},
    {"riv_straight", k_map_riv_straight, 0u, 0u, 9u, 9u},
    //{"riv_run", k_map_riv_run, 8u, 2u, 1u, 2u},
    //{"riv_fork", k_map_riv_fork, 8u, 5u, 4u, 2u},
};

static const u32 k_case_n = static_cast<u32>(sizeof(k_cases) / sizeof(k_cases[0]));

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 tidx (u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(k_w) + static_cast<u32>(x);
}

static void decode_ck (const u8* ck, u8* terrain, u8* rivers) {
    for (u32 i = 0; i < k_tile_n; ++i) {
        switch (ck[i]) {
        case k_ck_hill:
            terrain[i] = TERR_HILLS[0];
            rivers[i] = 0u;
            break;
        case k_ck_mtn:
            terrain[i] = TERR_MOUNTAINS[0];
            rivers[i] = 0u;
            break;
        case k_ck_riv:
            terrain[i] = TERR_PLAINS[0];
            rivers[i] = 1u;
            break;
        default:
            terrain[i] = TERR_PLAINS[0];
            rivers[i] = 0u;
            break;
        }
    }
}

static void print_ck (const u8* ck) {
    std::printf("  terrain (0=plain 1=hill 2=mtn 3=riv):\n");
    for (u16 y = 0; y < k_h; ++y) {
        std::printf("    ");
        for (u16 x = 0; x < k_w; ++x) {
            std::printf("%3u ", static_cast<unsigned>(ck[tidx(x, y)]));
        }
        std::printf("\n");
    }
}

static void print_dist (const u16* dist, u16 sx, u16 sy, u16 dx, u16 dy) {
    std::printf("  overlay (--=unreach):\n");
    for (u16 y = 0; y < k_h; ++y) {
        std::printf("    ");
        for (u16 x = 0; x < k_w; ++x) {
            char tag = ' ';
            if (x == sx && y == sy) {
                tag = 'S';
            } else if (x == dx && y == dy) {
                tag = 'D';
            }
            const u16 d = dist[tidx(x, y)];
            if (tag != ' ') {
                if (d == k_dist_sent) {
                    std::printf("%c-- ", tag);
                } else {
                    std::printf("%c%02u ", tag, static_cast<unsigned>(d));
                }
            } else if (d == k_dist_sent) {
                std::printf(" -- ");
            } else {
                std::printf("%3u ", static_cast<unsigned>(d));
            }
        }
        std::printf("\n");
    }
}

static WalkSimRes sim_walk (
    const WalkP2P_Mk4& wk,
    const u16* dist,
    const u8* terrain,
    const u8* rivers,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy,
    u8* path_m) {
    WalkSimRes res = {};
    u16 px = sx;
    u16 py = sy;
    i32 mp = static_cast<i32>(PATH_MP_TURN);
    u32 turn = 1u;
    for (u32 i = 0; i < k_tile_n; ++i) {
        path_m[i] = 0u;
    }
    path_m[tidx(px, py)] = 1u;
    u32 guard = 0;
    while (px != dx || py != dy) {
        if (++guard > k_tile_n) {
            break;
        }
        while (px != dx || py != dy) {
            const WalkP2P_Mk4::StepRes st = wk.peek_step(
                dist, terrain, rivers, k_w, k_h, px, py);
            if (!st.have) {
                res.game_turns = turn;
                return res;
            }
            const i32 cost = static_cast<i32>(st.cost);
            if (mp < cost) {
                break;
            }
            mp -= cost;
            px = st.nx;
            py = st.ny;
            path_m[tidx(px, py)] = 1u;
            res.path_len++;
            if (mp <= 0) {
                break;
            }
        }
        if (px == dx && py == dy) {
            res.traced = true;
            break;
        }
        turn++;
        mp += static_cast<i32>(PATH_MP_TURN);
    }
    res.game_turns = turn;
    return res;
}

static void print_path_view (
    const u8* ck,
    const u8* rivers,
    const u8* path_m,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy) {
    std::printf("  path (green=walk, blue=riv):\n");
    for (u16 y = 0; y < k_h; ++y) {
        std::printf("    ");
        for (u16 x = 0; x < k_w; ++x) {
            const u32 i = tidx(x, y);
            cstr col = "";
            if (path_m[i] != 0u) {
                col = k_ansi_grn;
            } else if (rivers[i] != 0u) {
                col = k_ansi_blu;
            }
            char tag = ' ';
            if (x == sx && y == sy) {
                tag = 'S';
            } else if (x == dx && y == dy) {
                tag = 'D';
            }
            if (tag != ' ') {
                std::printf("%s%c%02u%s ", col, tag,
                    static_cast<unsigned>(ck[i]), k_ansi_rst);
            } else {
                std::printf("%s%3u%s ", col,
                    static_cast<unsigned>(ck[i]), k_ansi_rst);
            }
        }
        std::printf("\n");
    }
}

static bool run_case (const CtrlCase& cs) {
    u8 terrain[k_tile_n];
    u8 rivers[k_tile_n];
    decode_ck(cs.m_ck, terrain, rivers);
    u16 dist[k_tile_n];
    u32 pred[k_tile_n];
    const i32 tn_i = static_cast<i32>(k_tile_n);
    AiWbSheet mp_sh(tn_i);
    AiWbSheet sq_sh(tn_i);
    AiWbSheet turn_sh(tn_i);
    if (!mp_sh.ok() || !sq_sh.ok() || !turn_sh.ok()) {
        std::printf("  alloc fail\n");
        return false;
    }
    u16* scr[Generate_DistanceP2P_Mk4::k_scr_n] = {
        mp_sh.get(), sq_sh.get(), turn_sh.get()};
    u16 max_turn = 0;
    const bool ok = Generate_DistanceP2P_Mk4::generate(
        terrain, rivers, k_w, k_h,
        cs.m_src_x, cs.m_src_y, cs.m_dst_x, cs.m_dst_y,
        dist, pred, scr, &max_turn);
    std::printf("=== %s ===\n", cs.m_name);
    print_ck(cs.m_ck);
    print_dist(dist, cs.m_src_x, cs.m_src_y, cs.m_dst_x, cs.m_dst_y);
    if (!ok) {
        std::printf("  src (%u,%u) dst (%u,%u) max_turn %u path fail\n\n",
            static_cast<unsigned>(cs.m_src_x),
            static_cast<unsigned>(cs.m_src_y),
            static_cast<unsigned>(cs.m_dst_x),
            static_cast<unsigned>(cs.m_dst_y),
            static_cast<unsigned>(max_turn));
        return false;
    }
    u8 path_m[k_tile_n];
    const WalkP2P_Mk4 wk;
    const WalkSimRes wres = sim_walk(
        wk, dist, terrain, rivers,
        cs.m_src_x, cs.m_src_y, cs.m_dst_x, cs.m_dst_y, path_m);
    print_path_view(cs.m_ck, rivers, path_m,
        cs.m_src_x, cs.m_src_y, cs.m_dst_x, cs.m_dst_y);
    std::printf("  src (%u,%u) dst (%u,%u) max_turn %u walk %s len %u turns %u\n",
        static_cast<unsigned>(cs.m_src_x),
        static_cast<unsigned>(cs.m_src_y),
        static_cast<unsigned>(cs.m_dst_x),
        static_cast<unsigned>(cs.m_dst_y),
        static_cast<unsigned>(max_turn),
        wres.traced ? "ok" : "stall",
        static_cast<unsigned>(wres.path_len),
        static_cast<unsigned>(wres.game_turns));
    std::printf("\n");
    return wres.traced;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    u32 pass = 0;
    for (u32 i = 0; i < k_case_n; ++i) {
        if (run_case(k_cases[i])) {
            pass++;
        }
    }
    std::printf("minor_ctrl: %u/%u ok\n", pass, k_case_n);
    return pass == k_case_n ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
