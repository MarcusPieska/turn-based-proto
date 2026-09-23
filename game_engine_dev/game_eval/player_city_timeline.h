//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_CITY_TIMELINE_H
#define PLAYER_CITY_TIMELINE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - PlayerCityTimeline -
//================================================================================================================================
//
//  Per-seat city founding rate by turn. note(turn) increments that turn's bucket; m_count is total
//  foundings (sort key). cum() prefix-sums the rate into cities-founded-by-turn.
//
//================================================================================================================================

class PlayerCityTimeline {
public:
    PlayerCityTimeline ();
    ~PlayerCityTimeline ();

    void clr ();
    bool setup (u16 turn_n);
    bool note (u16 turn);
    void recount ();

    u16 turn_n () const;
    u16 count () const;
    u16 rate_at (u16 turn) const;

    bool cum (u16* y, u16 turn_n) const;
    bool rate (u16* y, u16 turn_n) const;
    void swap (PlayerCityTimeline& o);

private:
    PlayerCityTimeline (const PlayerCityTimeline&) = delete;
    PlayerCityTimeline& operator= (const PlayerCityTimeline&) = delete;

    u16* m_inc;
    u16 m_turn_n;
    u16 m_count;
};

#endif // PLAYER_CITY_TIMELINE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
