//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLAYER_CITY_TIMELINE_MNG_H
#define PLAYER_CITY_TIMELINE_MNG_H

#include "game_primitives.h"
#include "player_city_timeline.h"

class EvalLog;

//================================================================================================================================
//=> - PlayerCityTimelineMng -
//================================================================================================================================
//
//  One PlayerCityTimeline per seat. fill() walks NEW_TURN + city-foundation (player only; x,y used
//  to skip duplicate capital replays). sort() by founding count descending.
//
//================================================================================================================================

class PlayerCityTimelineMng {
public:
    PlayerCityTimelineMng ();
    ~PlayerCityTimelineMng ();

    void clr ();
    bool setup (u16 player_n, u16 turn_n);
    bool fill (const EvalLog& log);
    void sort ();

    u16 player_n () const;
    u16 max_turn () const;
    u16 turn_n () const;

    PlayerCityTimeline& at (u16 i);
    const PlayerCityTimeline& at (u16 i) const;
    u16 seat (u16 i) const;

private:
    PlayerCityTimelineMng (const PlayerCityTimelineMng&) = delete;
    PlayerCityTimelineMng& operator= (const PlayerCityTimelineMng&) = delete;

    PlayerCityTimeline* m_tl;
    u16* m_seat;
    u16 m_player_n;
    u16 m_turn_n;
    u16 m_max_turn;
};

#endif // PLAYER_CITY_TIMELINE_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
