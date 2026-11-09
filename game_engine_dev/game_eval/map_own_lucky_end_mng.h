//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_OWN_LUCKY_END_MNG_H
#define MAP_OWN_LUCKY_END_MNG_H

#include "game_primitives.h"

class EvalPaths;

//================================================================================================================================
//=> - MapOwnLuckyEndMng -
//================================================================================================================================
//
//  Loads the last save map and writes one ownership PPM per lucky seat under out_dir.
//
//================================================================================================================================

class MapOwnLuckyEndMng {
public:
    MapOwnLuckyEndMng ();
    ~MapOwnLuckyEndMng ();

    void clr ();
    bool setup (u16 player_n, u16 save_n);
    bool fill (const EvalPaths& paths, cstr out_dir);

    u16 wrote_n () const;
    u16 lucky_n () const;

private:
    MapOwnLuckyEndMng (const MapOwnLuckyEndMng&) = delete;
    MapOwnLuckyEndMng& operator= (const MapOwnLuckyEndMng&) = delete;

    u16* m_lucky;
    u16 m_lucky_n;
    u16 m_player_n;
    u16 m_save_n;
    u16 m_wrote_n;
};

#endif // MAP_OWN_LUCKY_END_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
