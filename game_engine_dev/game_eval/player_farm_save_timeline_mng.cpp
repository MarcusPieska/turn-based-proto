//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_farm_save_timeline_mng.h"

#include <cstdio>

#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "std_add_helper.h"

//================================================================================================================================
//=> - PlayerFarmSaveTimelineMng -
//================================================================================================================================

PlayerFarmSaveTimelineMng::PlayerFarmSaveTimelineMng ()
    : m_tl(nullptr),
      m_seat(nullptr),
      m_player_n(0),
      m_save_n(0) {
}

PlayerFarmSaveTimelineMng::~PlayerFarmSaveTimelineMng () {
    clr();
}

void PlayerFarmSaveTimelineMng::clr () {
    delete[] m_tl;
    m_tl = nullptr;
    delete[] m_seat;
    m_seat = nullptr;
    m_player_n = 0;
    m_save_n = 0;
}

bool PlayerFarmSaveTimelineMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n == 0) {
        return false;
    }
    m_tl = new PlayerFarmSaveTimeline[player_n];
    m_seat = new u16[player_n];
    m_player_n = player_n;
    m_save_n = save_n;
    for (u16 i = 0; i < player_n; ++i) {
        if (!m_tl[i].setup(save_n)) {
            clr();
            return false;
        }
        m_seat[i] = i;
    }
    return true;
}

bool PlayerFarmSaveTimelineMng::fill (const EvalPaths& paths) {
    if (m_tl == nullptr || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    u32* tallies = new u32[m_player_n];
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char map_p[512];
        if (!paths.map_path(turn, map_p, sizeof(map_p))) {
            delete[] tallies;
            return false;
        }
        {
            GameArraySimple map;
            if (!GameIo::load_map_tiles(map_p, map)) {
                delete[] tallies;
                return false;
            }
            for (u16 p = 0; p < m_player_n; ++p) {
                tallies[p] = 0;
            }
            const u16 w = map.width();
            const u16 h = map.height();
            for (u16 y = 0; y < h; ++y) {
                for (u16 x = 0; x < w; ++x) {
                    const GameTileSimple* t = map.tile(x, y);
                    if (t == nullptr || !StdAddHelper::has_farm(t)) {
                        continue;
                    }
                    const u8 own = static_cast<u8>(t->m_civ_owner);
                    if (own == U8_KEY_NULL || own >= m_player_n) {
                        continue;
                    }
                    tallies[own] = tallies[own] + 1u;
                }
            }
        }
        for (u16 p = 0; p < m_player_n; ++p) {
            if (!m_tl[p].set(si, tallies[p])) {
                delete[] tallies;
                return false;
            }
        }
    }
    delete[] tallies;
    for (u16 p = 0; p < m_player_n; ++p) {
        m_tl[p].sync_count();
    }
    return true;
}

void PlayerFarmSaveTimelineMng::sort () {
    if (m_tl == nullptr || m_player_n < 2) {
        return;
    }
    for (u16 i = 0; i < m_player_n; ++i) {
        u16 best = i;
        for (u16 j = static_cast<u16>(i + 1u); j < m_player_n; ++j) {
            if (m_tl[j].count() > m_tl[best].count()) {
                best = j;
            }
        }
        if (best != i) {
            m_tl[i].swap(m_tl[best]);
            const u16 s = m_seat[i];
            m_seat[i] = m_seat[best];
            m_seat[best] = s;
        }
    }
}

u16 PlayerFarmSaveTimelineMng::player_n () const {
    return m_player_n;
}

u16 PlayerFarmSaveTimelineMng::save_n () const {
    return m_save_n;
}

PlayerFarmSaveTimeline& PlayerFarmSaveTimelineMng::at (u16 i) {
    return m_tl[i];
}

const PlayerFarmSaveTimeline& PlayerFarmSaveTimelineMng::at (u16 i) const {
    return m_tl[i];
}

u16 PlayerFarmSaveTimelineMng::seat (u16 i) const {
    if (m_seat == nullptr || i >= m_player_n) {
        return UINT16_MAX;
    }
    return m_seat[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
