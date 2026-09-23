//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_TECH_TIMELINE_H
#define PLAYER_TECH_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - PlayerTechTimeline -
//================================================================================================================================
//
//  Per-seat tech discovery times. m_disc[tech] = turn first unlocked, or UINT16_MAX if never.
//  m_count is unlock total (sort key). cum() builds unlocks-so-far vs turn; rate() builds per-turn unlocks.
//
//================================================================================================================================

class PlayerTechTimeline {
public:
    PlayerTechTimeline ();
    ~PlayerTechTimeline ();

    void clr ();
    bool setup (u16 tech_n);
    bool note (u16 tech, u16 turn);
    void recount ();

    u16 tech_n () const;
    u16 count () const;
    u16 disc (u16 tech) const;

    bool cum (u16* y, u16 turn_n) const;
    bool rate (u16* y, u16 turn_n) const;
    void swap (PlayerTechTimeline& o);

private:
    PlayerTechTimeline (const PlayerTechTimeline&) = delete;
    PlayerTechTimeline& operator= (const PlayerTechTimeline&) = delete;

    u16* m_disc;
    u16 m_tech_n;
    u16 m_count;
};

#endif // PLAYER_TECH_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
