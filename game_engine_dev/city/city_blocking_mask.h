//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_BLOCKING_MASK_H
#define CITY_BLOCKING_MASK_H

#include "game_primitives.h"

class GameArraySimple;

//================================================================================================================================
//=> - CityBlockingMask -
//================================================================================================================================
//
//  Settle-blocking around a city. stamp writes m_settler_blocked; preview fills m_side x m_side scratch.
//  Tweaks: m_r_core (full block disk), m_r_gate[] (outer band ends), m_omit_ge[] (parallel score gates).
//  m_r_max must be >= last gate; m_side = 2*m_r_max+1. run walks each band as an annulus (no nested re-scan).
//
//================================================================================================================================

class CityBlockingMask {
public:
    static constexpr u16 m_r_core = 3u; // Full-block disk radius
    static constexpr u16 m_r_gate[] = {7u, 11u, 15u, 19u}; // Outer band ends (tweak these)
    static constexpr u8 m_omit_ge[] = {4u, 3u, 2u, 1u}; // Parallel score gates (tweak these)
    static constexpr u16 m_r_gate_n = static_cast<u16>(sizeof(m_r_gate) / sizeof(m_r_gate[0]));
    static constexpr u16 m_r_max = m_r_gate[m_r_gate_n - 1u];
    static constexpr u16 m_side = static_cast<u16>(2u * m_r_max + 1u);
    static constexpr u16 m_n = static_cast<u16>(m_side * m_side);
    static_assert(sizeof(m_omit_ge) / sizeof(m_omit_ge[0]) == sizeof(m_r_gate) / sizeof(m_r_gate[0]), "m_omit_ge/m_r_gate");
    static_assert(m_r_max >= m_r_core, "m_r_max");

    static void stamp (GameArraySimple& map, u16 cx, u16 cy);
    static const u8* preview (const GameArraySimple& map, u16 cx, u16 cy);

    static const u8* const m_mask;

private:
    static u8 m_buf[m_n];

    static u8 score (u8 clim, u8 riv);
    static void band (
        const GameArraySimple& map,
        GameArraySimple* out,
        u8* local,
        u16 cx,
        u16 cy,
        u16 w,
        u16 h,
        u16 half,
        u16 i0,
        u16 i1,
        const i8 (*brd)[2],
        u8 omit_ge);
    static void run (const GameArraySimple& map, GameArraySimple* out, u16 cx, u16 cy, u8* local);

    CityBlockingMask () = delete;
};

#endif // CITY_BLOCKING_MASK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
