//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>

#include "eval_driver.h"
#include "eval_need.h"
#include "game_io.h"
#include "runtime_static_loader.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_data.h"
#include "unit_static_key.h"

//================================================================================================================================
//= Paths =
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "camp-mvt-save-probe";
static const char* G_RT_LIB = "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";
static const u16 k_watch_cap = 64u;
static const u16 k_save_cap = 128u;
static const u32 k_fail_min = 50u;

//================================================================================================================================
//= Types =
//================================================================================================================================

struct Watch {
    u16 m_key;
    u16 m_seat;
    u32 m_fails;
    u16 m_typ;
    u8 m_size;
    u16 m_x[k_save_cap];
    u16 m_y[k_save_cap];
    i16 m_mp[k_save_cap];
    u8 m_camp[k_save_cap];
    u8 m_ext[k_save_cap];
    u8 m_have[k_save_cap];
};

//================================================================================================================================
//= Helpers =
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

static bool parse_trace (cstr path, Watch* w, u16* n) {
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        return false;
    }
    u32* fails = new u32[65536];
    u16* seats = new u16[65536];
    std::memset(fails, 0, sizeof(u32) * 65536u);
    for (u32 i = 0; i < 65536u; ++i) {
        seats[i] = U16_KEY_NULL;
    }
    char line[512];
    while (std::fgets(line, sizeof(line), fp) != nullptr) {
        unsigned seat = 0;
        unsigned unit = 0;
        unsigned reason = 0;
        if (std::sscanf(
                line,
                "war muster can_step fail seat=%u unit=%u reason=%u",
                &seat,
                &unit,
                &reason)
            != 3) {
            continue;
        }
        if (reason != 7u || unit > 65535u) {
            continue;
        }
        fails[unit] = fails[unit] + 1u;
        if (seats[unit] == U16_KEY_NULL) {
            seats[unit] = static_cast<u16>(seat);
        }
    }
    std::fclose(fp);
    *n = 0;
    for (;;) {
        u32 best_f = k_fail_min;
        u32 best_k = 65536u;
        for (u32 k = 0; k < 65536u; ++k) {
            if (fails[k] < best_f) {
                continue;
            }
            if (fails[k] > best_f || (fails[k] == best_f && k < best_k)) {
                best_f = fails[k];
                best_k = k;
            }
        }
        if (best_k >= 65536u || *n >= k_watch_cap) {
            break;
        }
        Watch& t = w[*n];
        t.m_key = static_cast<u16>(best_k);
        t.m_seat = seats[best_k];
        t.m_fails = best_f;
        t.m_typ = U16_KEY_NULL;
        t.m_size = 0;
        std::memset(t.m_have, 0, sizeof(t.m_have));
        *n = static_cast<u16>(*n + 1u);
        fails[best_k] = 0;
    }
    delete[] fails;
    delete[] seats;
    return *n > 0u;
}

static void fill_snap (Watch* w, u16 n, u16 si, const UnitAddVector& units) {
    const u16 head = units.get_head_unit_add_idx();
    for (u16 i = 0; i < n; ++i) {
        w[i].m_have[si] = 0;
        if (w[i].m_key >= head) {
            continue;
        }
        const UnitAddStruct* u = units.get_unit_add(UnitAddKey::from_raw(w[i].m_key));
        if (u == nullptr) {
            continue;
        }
        w[i].m_have[si] = 1;
        w[i].m_x[si] = static_cast<u16>(u->m_x);
        w[i].m_y[si] = static_cast<u16>(u->m_y);
        w[i].m_mp[si] = u->m_mvt_points;
        w[i].m_camp[si] = static_cast<u8>(u->m_in_campaign);
        w[i].m_ext[si] = static_cast<u8>(u->m_being_extended);
        w[i].m_typ = static_cast<u16>(u->m_unit_typ_idx);
        w[i].m_size = static_cast<u8>(u->m_unit_size);
        w[i].m_seat = static_cast<u16>(u->m_player_idx);
    }
}

static void wr_unit (
    std::FILE* fp,
    const Watch& w,
    u16 save_n,
    const u32* turns,
    u16 base_mp,
    u16 typ_mvt)
{
    u16 seen = 0;
    u16 le0 = 0;
    u16 gt0 = 0;
    u16 moved = 0;
    u16 px = U16_KEY_NULL;
    u16 py = U16_KEY_NULL;
    for (u16 si = 0; si < save_n; ++si) {
        if (w.m_have[si] == 0u) {
            continue;
        }
        seen = static_cast<u16>(seen + 1u);
        if (w.m_mp[si] <= 0) {
            le0 = static_cast<u16>(le0 + 1u);
        } else {
            gt0 = static_cast<u16>(gt0 + 1u);
        }
        if (px != U16_KEY_NULL && (px != w.m_x[si] || py != w.m_y[si])) {
            moved = static_cast<u16>(moved + 1u);
        }
        px = w.m_x[si];
        py = w.m_y[si];
    }
    std::fprintf(
        fp,
        "unit=%u seat=%u fails=%u typ=%u typ_mvt=%u size=%u base_mp=%u seen=%u mp_le0=%u mp_gt0=%u pos_chg=%u\n",
        static_cast<u32>(w.m_key),
        static_cast<u32>(w.m_seat),
        w.m_fails,
        static_cast<u32>(w.m_typ),
        static_cast<u32>(typ_mvt),
        static_cast<u32>(w.m_size),
        static_cast<u32>(base_mp),
        static_cast<u32>(seen),
        static_cast<u32>(le0),
        static_cast<u32>(gt0),
        static_cast<u32>(moved));
    for (u16 si = 0; si < save_n; ++si) {
        if (w.m_have[si] == 0u) {
            continue;
        }
        std::fprintf(
            fp,
            "  t=%u xy=(%u,%u) mp=%d camp=%u ext=%u\n",
            turns[si],
            static_cast<u32>(w.m_x[si]),
            static_cast<u32>(w.m_y[si]),
            static_cast<int>(w.m_mp[si]),
            static_cast<u32>(w.m_camp[si]),
            static_cast<u32>(w.m_ext[si]));
    }
}

//================================================================================================================================
//= CampMvtSaveProbeTester =
//================================================================================================================================

class CampMvtSaveProbeTester : public EvalDriver {
public:
    CampMvtSaveProbeTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

CampMvtSaveProbeTester::CampMvtSaveProbeTester () : EvalDriver(make_need()) {
}

int CampMvtSaveProbeTester::run () {
    const u16 save_n = paths().save_turn_n();
    if (save_n < 2u || save_n > k_save_cap) {
        std::printf("camp_mvt_save_probe: bad save_n=%u\n", static_cast<u32>(save_n));
        return 1;
    }
    Watch watch[k_watch_cap];
    u16 wn = 0;
    if (!parse_trace(paths().trace_path(), watch, &wn)) {
        std::printf("camp_mvt_save_probe: no NoMp watch units (min fails=%u)\n", k_fail_min);
        return 1;
    }
    std::printf("camp_mvt_save_probe watch_n=%u saves=%u\n", static_cast<u32>(wn), static_cast<u32>(save_n));
    u32 turns[k_save_cap];
    for (u16 si = 0; si < save_n; ++si) {
        turns[si] = paths().save_turn_at(si);
        char units_p[512];
        if (!paths().units_path(turns[si], units_p, sizeof(units_p))) {
            std::printf("camp_mvt_save_probe: units_path fail turn=%u\n", turns[si]);
            return 1;
        }
        UnitAddVector units;
        if (!GameIo::load_units(units_p, units)) {
            std::printf("camp_mvt_save_probe: load_units fail %s\n", units_p);
            return 1;
        }
        fill_snap(watch, wn, si, units);
    }
    RuntimeStaticLoader loader;
    const bool st_ok = loader.load(G_RT_LIB, G_RT_DATA);
    const u16 turn_mp = st_ok ? loader.statics().config().get_mov_pt_per_turn() : 0u;
    const u16 typ_n = st_ok ? loader.statics().unit().get_item_count() : 0u;
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))
        || !mkdir_deep(eval_dir)
        || !mkdir_deep(data_dir)) {
        std::printf("camp_mvt_save_probe: out paths failed\n");
        return 1;
    }
    char out_p[512];
    if (std::snprintf(out_p, sizeof(out_p), "%s/watch_mp.txt", data_dir) <= 0) {
        return 1;
    }
    std::FILE* fp = std::fopen(out_p, "w");
    if (fp == nullptr) {
        return 1;
    }
    std::fprintf(fp, "# NoMp can_step watch (fails>=%u); mp from unit saves\n", k_fail_min);
    u16 always_le0 = 0;
    u16 mixed = 0;
    u16 always_gt0 = 0;
    for (u16 i = 0; i < wn; ++i) {
        u16 typ_mvt = 0;
        u16 base_mp = 0;
        if (st_ok && watch[i].m_typ < typ_n) {
            typ_mvt = loader.statics().unit().get_item(UnitStaticDataKey::from_raw(watch[i].m_typ)).mvt_pts;
            base_mp = static_cast<u16>(static_cast<u32>(typ_mvt) * static_cast<u32>(turn_mp));
        }
        wr_unit(fp, watch[i], save_n, turns, base_mp, typ_mvt);
        u16 le0 = 0;
        u16 gt0 = 0;
        for (u16 si = 0; si < save_n; ++si) {
            if (watch[i].m_have[si] == 0u) {
                continue;
            }
            if (watch[i].m_mp[si] <= 0) {
                le0 = static_cast<u16>(le0 + 1u);
            } else {
                gt0 = static_cast<u16>(gt0 + 1u);
            }
        }
        if (le0 > 0u && gt0 == 0u) {
            always_le0 = static_cast<u16>(always_le0 + 1u);
        } else if (gt0 > 0u && le0 == 0u) {
            always_gt0 = static_cast<u16>(always_gt0 + 1u);
        } else if (le0 > 0u && gt0 > 0u) {
            mixed = static_cast<u16>(mixed + 1u);
        }
        std::printf(
            "unit=%u seat=%u fails=%u typ_mvt=%u base_mp=%u mp_le0=%u mp_gt0=%u\n",
            static_cast<u32>(watch[i].m_key),
            static_cast<u32>(watch[i].m_seat),
            watch[i].m_fails,
            static_cast<u32>(typ_mvt),
            static_cast<u32>(base_mp),
            static_cast<u32>(le0),
            static_cast<u32>(gt0));
    }
    std::fclose(fp);
    std::printf(
        "camp_mvt_save_probe ok watch=%u always_mp_le0=%u mixed_mp=%u always_mp_gt0=%u out=%s\n",
        static_cast<u32>(wn),
        static_cast<u32>(always_le0),
        static_cast<u32>(mixed),
        static_cast<u32>(always_gt0),
        out_p);
    return 0;
}

//================================================================================================================================
//= Main =
//================================================================================================================================

int main () {
    CampMvtSaveProbeTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//= End of file =
//================================================================================================================================
