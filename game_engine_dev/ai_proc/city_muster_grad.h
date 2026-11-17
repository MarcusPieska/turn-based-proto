//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_MUSTER_GRAD_H
#define CITY_MUSTER_GRAD_H

#include "game_primitives.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "walk_p2p.h"

class GameState;

//================================================================================================================================
//=> - CityMusterGrad -
//================================================================================================================================
//
//  One shared WalkP2P muster board for all seats. Tile ownership (GameTileSimple::m_civ_owner)
//  clips each capital flood to own land and selects which seat owns a cell. Peripheral cities off
//  the capital component do not receive a gradient. m_muster_city on PlayerState holds the muster
//  city index (capital). flood(seat) refreshes one seat and dirties that seat's city reach flags;
//  build() clears the board then floods the lucky seat for the current turn (LuckySeatTurn).
//  walk_muster() follows the gradient downhill to that seat's muster city. city_reach() verify-walks
//  when new_muster_flood is set. begin/handle/clear drive the ongoing capital feed from the game loop.
//
//================================================================================================================================

class CityMusterGrad {
public:
    CityMusterGrad ();
    ~CityMusterGrad ();

    static bool begin (GameState& st);
    static void clear ();
    static void handle (GameState& st);

    bool bind (GameState* st);
    void clr ();
    bool set_caps ();
    bool build ();
    bool flood (u16 seat);
    bool walk_muster (u16 x, u16 y) const;
    bool city_reach (u16 city_idx);
    bool ok () const;
    const WalkP2P& board () const;
    u16 muster_city (u16 seat) const;

private:
    CityMusterGrad (const CityMusterGrad& o) = delete;
    CityMusterGrad& operator= (const CityMusterGrad& o) = delete;

    bool flood_seat (u16 seat);
    void mark_flood (u16 seat);
    void init_sent ();
    void pull_cities ();
    void step_heads ();
    bool step_one (UnitAddKey key);
    bool feed_unit (const UnitAddStruct& u) const;

    GameState* m_st; // Bound match state
    WalkP2P m_board; // Shared turn/rem muster field
    bool m_ok; // True after successful build
};

#endif // CITY_MUSTER_GRAD_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
