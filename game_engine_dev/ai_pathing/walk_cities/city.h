//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_H
#define CITY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - City class -
//================================================================================================================================
//
//  Walk_cities stub: location, owner, and quadrant road connections matching city/city.h network API.
//  m_road_conn holds four 2-bit fields (dir 0..3 = NE,NW,SE,SW): bit0 locked, bit1 built.
//
//================================================================================================================================

class alignas(8) City {
public:
    City ();
    ~City ();

    void init (u16 owner, u16 x, u16 y);

    u16 get_owner () const;
    u16 get_x () const;
    u16 get_y () const;

    void set_conn_city (u16 city_idx, u8 dir);
    u16 get_conn_city (u8 dir) const;
    bool is_conn_city_locked (u8 dir) const;
    bool is_conn_city_built (u8 dir) const;
    void conn_city_is_locked (u8 dir);
    void conn_city_is_built (u8 dir);

private:
    u16 m_owner; // Owning seat index; U16_KEY_NULL until init
    u16 m_x; // Map column; U16_KEY_NULL until init
    u16 m_y; // Map row; U16_KEY_NULL until init
    u16 m_conn_city_nw; // Connection city NW; U16_KEY_NULL if none
    u16 m_conn_city_ne; // Connection city NE; U16_KEY_NULL if none
    u16 m_conn_city_sw; // Connection city SW; U16_KEY_NULL if none
    u16 m_conn_city_se; // Connection city SE; U16_KEY_NULL if none
    u8 m_road_conn; // Per-dir 2-bit: locked (low), built (high)
};

#endif // CITY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
