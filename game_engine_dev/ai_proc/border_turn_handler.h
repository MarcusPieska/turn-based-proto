//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BORDER_TURN_HANDLER_H
#define BORDER_TURN_HANDLER_H

#include "game_primitives.h"

class CityArray;
class GameArraySimple;

//================================================================================================================================
//=> - BorderFront -
//================================================================================================================================
//
//  One muster slot: m_city is idle/need/city index; m_pl is optional matched foreign seat
//  (mk06); unused mks leave m_pl as U16_KEY_NULL.
//
//================================================================================================================================

struct BorderFront {
    u16 m_city; // k_front_idle / k_front_need / owned city index
    u16 m_pl; // matched foreign seat, or U16_KEY_NULL
};

//================================================================================================================================
//=> - BorderTurnHandler -
//================================================================================================================================
//
//  Fills per-seat front muster slots. Caller owns fronts[0..n): m_city idle/need/city;
//  m_pl matched foe when known. n in 1..k_front_cap. handle only writes need slots.
//  Impl via BORDER_TURN_HANDLER_IMPL (default mk06).
//
//================================================================================================================================

class BorderTurnHandler {
public:
    static const u16 k_front_cap = 10u;
    static const u16 k_front_idle = U16_KEY_NULL;
    static const u16 k_front_need = static_cast<u16>(U16_KEY_NULL - 1u);

    BorderTurnHandler ();
    ~BorderTurnHandler ();

    bool begin (const GameArraySimple& map, const CityArray& cities);
    bool handle (u16 seat, BorderFront* fronts, u16 n);
    void clr ();

    bool ok () const;
    u16 capital_city () const;
    u16 front_n () const;
    u16 front_city (u16 i) const;
    u16 front_x (u16 i) const;
    u16 front_y (u16 i) const;

    static bool slot_idle (u16 v);
    static bool slot_need (u16 v);
    static bool slot_city (u16 v);

private:
    BorderTurnHandler (const BorderTurnHandler& o) = delete;
    BorderTurnHandler& operator= (const BorderTurnHandler& o) = delete;
    BorderTurnHandler (BorderTurnHandler&& o) = delete;
    BorderTurnHandler& operator= (BorderTurnHandler&& o) = delete;

    u16 count_need (const BorderFront* fronts, u16 n) const;
    void gather_used (const BorderFront* fronts, u16 n, u16* used, u16* used_n) const;
    void fill_need (BorderFront* fronts, u16 n, const BorderFront* picks, u16 pick_n);
    void fallback_cap (BorderFront* fronts, u16 n, u16 cap);
    void sync_from (const BorderFront* fronts, u16 n);

    const GameArraySimple* m_map;
    const CityArray* m_cities;
    u16 m_cap;
    u16 m_front[k_front_cap];
    u16 m_fx[k_front_cap];
    u16 m_fy[k_front_cap];
    u16 m_front_n;
    bool m_ok;
};

#endif // BORDER_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
