//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_WAR_LOG_TIMELINE_H
#define LUCKY_WAR_LOG_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - LuckyWarEv -
//================================================================================================================================

enum class LuckyWarEv : u8 {
    Start = 0,
    End = 1,
    Muster = 2,
    Capture = 3,
    Army = 4,
    Loss = 5,
    Targets = 6,
    FormFail = 7,
    DeclFail = 8,
    AssaultFail = 9,
    Upgrade = 10
};

//================================================================================================================================
//=> - LuckyWarEvt -
//================================================================================================================================
//
//  One war-log event for a seat. m_a/m_b meaning depends on kind: Start/End enemy; Muster/Army
//  unit_n + size_sum; Loss size; Capture city (x,y); Targets remaining n; FormFail enemy;
//  DeclFail reason; AssaultFail city (x,y); Upgrade count on that turn.
//
//================================================================================================================================

struct LuckyWarEvt {
    u16 m_turn; // Event turn
    LuckyWarEv m_kind; // Event kind
    u16 m_a; // Enemy, size, city x, or upgrade count
    u16 m_b; // City y or unused
};

//================================================================================================================================
//=> - LuckyWarLogTimeline -
//================================================================================================================================
//
//  Append-only war event list for one lucky seat, sourced from trace PARSE lines.
//
//================================================================================================================================

class LuckyWarLogTimeline {
public:
    static const u16 k_cap = 1024u;

    LuckyWarLogTimeline ();

    void clr ();
    bool add (u16 turn, LuckyWarEv kind, u16 a, u16 b);
    bool add_upgrade (u16 turn);
    void swap (LuckyWarLogTimeline& o);
    u16 n () const;
    const LuckyWarEvt& at (u16 i) const;

private:
    LuckyWarEvt m_ev[k_cap];
    u16 m_n;
};

#endif // LUCKY_WAR_LOG_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
