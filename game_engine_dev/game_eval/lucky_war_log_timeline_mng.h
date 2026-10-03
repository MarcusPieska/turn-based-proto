//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_WAR_LOG_TIMELINE_MNG_H
#define LUCKY_WAR_LOG_TIMELINE_MNG_H

#include "game_primitives.h"
#include "lucky_war_log_timeline.h"

class EvalLog;

//================================================================================================================================
//=> - LuckyWarLogTimelineMng -
//================================================================================================================================
//
//  Discovers lucky seats from war muster/capture/peace lines and fills per-seat event timelines.
//  Start=muster, End=war peace, Muster/Army sizes, Capture, Loss, FormFail, DeclFail (skip
//  reason 0), AssaultFail-stop.
//
//================================================================================================================================

class LuckyWarLogTimelineMng {
public:
    static const u16 k_seat_cap = 32u;

    LuckyWarLogTimelineMng ();
    ~LuckyWarLogTimelineMng ();

    void clr ();
    bool fill (const EvalLog& log);
    void sort ();

    u16 seat_n () const;
    u16 max_turn () const;
    u16 seat (u16 i) const;
    const LuckyWarLogTimeline& at (u16 i) const;

private:
    LuckyWarLogTimelineMng (const LuckyWarLogTimelineMng&) = delete;
    LuckyWarLogTimelineMng& operator= (const LuckyWarLogTimelineMng&) = delete;

    i16 find (u16 seat) const;
    bool ensure (u16 seat, u16* out_i);

    LuckyWarLogTimeline m_tl[k_seat_cap];
    u16 m_seat[k_seat_cap];
    u8 m_at_war[k_seat_cap];
    u16 m_last_army[k_seat_cap];
    u16 m_seat_n;
    u16 m_max_turn;
};

#endif // LUCKY_WAR_LOG_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
