//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_war_log_timeline_mng.h"

#include "eval_log.h"
#include "log_dbg/log_war_army_cant_fight.h"
#include "log_dbg/log_war_army_size.h"
#include "log_dbg/log_war_army_upgrade.h"
#include "log_dbg/log_war_assault_fail_stop.h"
#include "log_dbg/log_war_city_capture.h"
#include "log_dbg/log_war_declare_fail.h"
#include "log_dbg/log_war_form_army_fail.h"
#include "log_dbg/log_war_muster.h"
#include "log_dbg/log_war_peace.h"
#include "log_dbg/log_war_peace_mock.h"
#include "log_dbg/log_war_targets.h"

//================================================================================================================================
//=> - LuckyWarLogTimelineMng -
//================================================================================================================================

LuckyWarLogTimelineMng::LuckyWarLogTimelineMng ()
    : m_seat_n(0u)
    , m_max_turn(0u) {
    for (u16 i = 0; i < k_seat_cap; ++i) {
        m_seat[i] = U16_KEY_NULL;
        m_at_war[i] = 0u;
        m_last_army[i] = 0u;
    }
}

LuckyWarLogTimelineMng::~LuckyWarLogTimelineMng () {
    clr();
}

void LuckyWarLogTimelineMng::clr () {
    for (u16 i = 0; i < m_seat_n; ++i) {
        m_tl[i].clr();
        m_seat[i] = U16_KEY_NULL;
        m_at_war[i] = 0u;
        m_last_army[i] = 0u;
    }
    m_seat_n = 0u;
    m_max_turn = 0u;
}

i16 LuckyWarLogTimelineMng::find (u16 seat) const {
    for (u16 i = 0; i < m_seat_n; ++i) {
        if (m_seat[i] == seat) {
            return static_cast<i16>(i);
        }
    }
    return -1;
}

bool LuckyWarLogTimelineMng::ensure (u16 seat, u16* out_i) {
    if (out_i == nullptr) {
        return false;
    }
    const i16 f = find(seat);
    if (f >= 0) {
        *out_i = static_cast<u16>(f);
        return true;
    }
    if (m_seat_n >= k_seat_cap) {
        return false;
    }
    *out_i = m_seat_n;
    m_seat[m_seat_n] = seat;
    m_at_war[m_seat_n] = 0u;
    m_last_army[m_seat_n] = 0u;
    m_tl[m_seat_n].clr();
    m_seat_n = static_cast<u16>(m_seat_n + 1u);
    return true;
}

static void note_turn (u16* mx, u16 turn) {
    if (turn > *mx) {
        *mx = turn;
    }
}

bool LuckyWarLogTimelineMng::fill (const EvalLog& log) {
    clr();
    for (u32 li = 0; li < log.line_n(); ++li) {
        const char* s = log.line(li);
        unsigned a0 = 0;
        unsigned a1 = 0;
        unsigned a2 = 0;
        unsigned a3 = 0;
        unsigned a4 = 0;
        unsigned a5 = 0;
        unsigned a6 = 0;
        unsigned a7 = 0;
        if (LOG_WAR_MUSTER::PARSE(s, &a0, &a1, &a2, &a3, &a4, &a5, &a6, &a7)) {
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a7);
            note_turn(&m_max_turn, turn);
            if (m_at_war[idx] == 0u) {
                (void)m_tl[idx].add(turn, LuckyWarEv::Start, static_cast<u16>(a1), 0u);
                m_at_war[idx] = 1u;
            }
            (void)m_tl[idx].add(turn, LuckyWarEv::Muster, static_cast<u16>(a4), static_cast<u16>(a5));
            continue;
        }
        if (LOG_WAR_ARMY_SIZE::PARSE(s, &a0, &a1, &a2, &a3, &a4, &a5)) {
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a5);
            note_turn(&m_max_turn, turn);
            const u16 army = static_cast<u16>(a1);
            if (m_last_army[idx] > army) {
                (void)m_tl[idx].add(turn, LuckyWarEv::Loss, static_cast<u16>(m_last_army[idx] - army), 0u);
            }
            m_last_army[idx] = army;
            (void)m_tl[idx].add(turn, LuckyWarEv::Army, army, static_cast<u16>(a2));
            continue;
        }
        if (LOG_WAR_CITY_CAPTURE::PARSE(s, &a0, &a1, &a2, &a3, &a4)) {
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a4);
            note_turn(&m_max_turn, turn);
            (void)m_tl[idx].add(turn, LuckyWarEv::Capture, static_cast<u16>(a2), static_cast<u16>(a3));
            continue;
        }
        if (LOG_WAR_ARMY_CANT_FIGHT::PARSE(s, &a0, &a1, &a2, &a3)) {
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a3);
            note_turn(&m_max_turn, turn);
            if (m_last_army[idx] > 0u) {
                (void)m_tl[idx].add(turn, LuckyWarEv::Loss, m_last_army[idx], 0u);
                m_last_army[idx] = 0u;
            }
            continue;
        }
        if (LOG_WAR_TARGETS::PARSE(s, &a0, &a1, &a2, &a3)) {
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a3);
            note_turn(&m_max_turn, turn);
            (void)m_tl[idx].add(turn, LuckyWarEv::Targets, static_cast<u16>(a2), 0u);
            continue;
        }
        if (LOG_WAR_FORM_ARMY_FAIL::PARSE(s, &a0, &a1, &a2)) {
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a2);
            note_turn(&m_max_turn, turn);
            (void)m_tl[idx].add(turn, LuckyWarEv::FormFail, static_cast<u16>(a1), 0u);
            continue;
        }
        if (LOG_WAR_ARMY_UPGRADE::PARSE(s, &a0, &a1, &a2, &a3, &a4)) {
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a4);
            note_turn(&m_max_turn, turn);
            (void)m_tl[idx].add_upgrade(turn);
            continue;
        }
        if (LOG_WAR_DECLARE_FAIL::PARSE(s, &a0, &a1, &a2, &a3)) {
            if (a3 == 0u) {
                continue;
            }
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a2);
            note_turn(&m_max_turn, turn);
            (void)m_tl[idx].add(turn, LuckyWarEv::DeclFail, static_cast<u16>(a3), static_cast<u16>(a1));
            continue;
        }
        if (LOG_WAR_ASSAULT_FAIL_STOP::PARSE(s, &a0, &a1, &a2, &a3, &a4)) {
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a4);
            note_turn(&m_max_turn, turn);
            (void)m_tl[idx].add(turn, LuckyWarEv::AssaultFail, static_cast<u16>(a2), static_cast<u16>(a3));
            continue;
        }
        if (LOG_WAR_PEACE::PARSE(s, &a0, &a1, &a2)) {
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a2);
            note_turn(&m_max_turn, turn);
            if (m_at_war[idx] != 0u) {
                (void)m_tl[idx].add(turn, LuckyWarEv::End, static_cast<u16>(a1), 0u);
                m_at_war[idx] = 0u;
                m_last_army[idx] = 0u;
            }
            continue;
        }
        if (LOG_WAR_PEACE_MOCK::PARSE(s, &a0, &a1, &a2, &a3)) {
            u16 idx = 0;
            if (!ensure(static_cast<u16>(a0), &idx)) {
                continue;
            }
            const u16 turn = static_cast<u16>(a3);
            note_turn(&m_max_turn, turn);
            continue;
        }
    }
    return m_seat_n > 0u;
}

void LuckyWarLogTimelineMng::sort () {
    for (u16 i = 0; i < m_seat_n; ++i) {
        u16 best = i;
        for (u16 j = static_cast<u16>(i + 1u); j < m_seat_n; ++j) {
            if (m_seat[j] < m_seat[best]) {
                best = j;
            }
        }
        if (best == i) {
            continue;
        }
        const u16 ts = m_seat[i];
        m_seat[i] = m_seat[best];
        m_seat[best] = ts;
        const u8 tw = m_at_war[i];
        m_at_war[i] = m_at_war[best];
        m_at_war[best] = tw;
        const u16 ta = m_last_army[i];
        m_last_army[i] = m_last_army[best];
        m_last_army[best] = ta;
        m_tl[i].swap(m_tl[best]);
    }
}

u16 LuckyWarLogTimelineMng::seat_n () const {
    return m_seat_n;
}

u16 LuckyWarLogTimelineMng::max_turn () const {
    return m_max_turn;
}

u16 LuckyWarLogTimelineMng::seat (u16 i) const {
    return m_seat[i];
}

const LuckyWarLogTimeline& LuckyWarLogTimelineMng::at (u16 i) const {
    return m_tl[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
