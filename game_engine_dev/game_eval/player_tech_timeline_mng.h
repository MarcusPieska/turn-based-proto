//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_TECH_TIMELINE_MNG_H
#define PLAYER_TECH_TIMELINE_MNG_H

#include "game_primitives.h"
#include "player_tech_timeline.h"

class EvalLog;

//================================================================================================================================
//=> - PlayerTechTimelineMng -
//================================================================================================================================
//
//  One PlayerTechTimeline per seat. fill() walks EvalLog NEW_TURN + tech-discover lines.
//  sort() orders seats by unlock count descending (selection sort).
//
//================================================================================================================================

class PlayerTechTimelineMng {
public:
    PlayerTechTimelineMng ();
    ~PlayerTechTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 tech_n);
    bool fill (const EvalLog& log);
    void sort ();

    u16 player_n () const;
    u16 tech_n () const;
    u16 max_turn () const;
    u16 turn_n () const;

    PlayerTechTimeline& at (u16 i);
    const PlayerTechTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    PlayerTechTimelineMng (const PlayerTechTimelineMng&) = delete;
    PlayerTechTimelineMng& operator= (const PlayerTechTimelineMng&) = delete;

    PlayerTechTimeline* m_tl;
    u16* m_seat;
    u16 m_player_n;
    u16 m_tech_n;
    u16 m_max_turn;
};

#endif // PLAYER_TECH_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
