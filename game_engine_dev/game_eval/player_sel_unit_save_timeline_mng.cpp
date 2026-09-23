//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_sel_unit_save_timeline_mng.h"

#include <cstdio>

#include "eval_paths.h"
#include "game_io.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"

//================================================================================================================================
//=> - PlayerSelUnitSaveTimelineMng -
//================================================================================================================================

PlayerSelUnitSaveTimelineMng::PlayerSelUnitSaveTimelineMng ()
    : m_tl(nullptr),
      m_seat(nullptr),
      m_player_n(0),
      m_save_n(0),
      m_unit_typ(U16_KEY_NULL) {
}

PlayerSelUnitSaveTimelineMng::~PlayerSelUnitSaveTimelineMng () {
    clr();
}

void PlayerSelUnitSaveTimelineMng::clr () {
    delete[] m_tl;
    m_tl = nullptr;
    delete[] m_seat;
    m_seat = nullptr;
    m_player_n = 0;
    m_save_n = 0;
    m_unit_typ = U16_KEY_NULL;
}

bool PlayerSelUnitSaveTimelineMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n == 0) {
        return false;
    }
    m_tl = new PlayerSelUnitSaveTimeline[player_n];
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

bool PlayerSelUnitSaveTimelineMng::fill (const EvalPaths& paths, u16 unit_typ_idx) {
    if (m_tl == nullptr || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    if (unit_typ_idx == U16_KEY_NULL) {
        return false;
    }
    m_unit_typ = unit_typ_idx;
    u32* tallies = new u32[m_player_n];
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char units_p[512];
        if (!paths.units_path(turn, units_p, sizeof(units_p))) {
            delete[] tallies;
            return false;
        }
        {
            UnitAddVector units;
            if (!GameIo::load_units(units_p, units)) {
                delete[] tallies;
                return false;
            }
            for (u16 p = 0; p < m_player_n; ++p) {
                tallies[p] = 0;
            }
            const u16 head = units.get_head_unit_add_idx();
            for (u16 k = 0; k < head; ++k) {
                const UnitAddStruct* u = units.get_unit_add(UnitAddKey::from_raw(k));
                if (u == nullptr) {
                    continue;
                }
                if (static_cast<u16>(u->m_unit_typ_idx) != m_unit_typ) {
                    continue;
                }
                if (u->m_player_idx >= m_player_n) {
                    continue;
                }
                tallies[u->m_player_idx] = tallies[u->m_player_idx] + 1u;
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

void PlayerSelUnitSaveTimelineMng::sort () {
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

u16 PlayerSelUnitSaveTimelineMng::player_n () const {
    return m_player_n;
}

u16 PlayerSelUnitSaveTimelineMng::save_n () const {
    return m_save_n;
}

u16 PlayerSelUnitSaveTimelineMng::unit_typ () const {
    return m_unit_typ;
}

PlayerSelUnitSaveTimeline& PlayerSelUnitSaveTimelineMng::at (u16 i) {
    return m_tl[i];
}

const PlayerSelUnitSaveTimeline& PlayerSelUnitSaveTimelineMng::at (u16 i) const {
    return m_tl[i];
}

u16 PlayerSelUnitSaveTimelineMng::seat (u16 i) const {
    if (m_seat == nullptr || i >= m_player_n) {
        return UINT16_MAX;
    }
    return m_seat[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
