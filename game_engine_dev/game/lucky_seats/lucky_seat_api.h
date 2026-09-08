//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_SEAT_API_H
#define LUCKY_SEAT_API_H

#include "game_primitives.h"
#include "starting_point_generator.h"

class GameArraySimple;
struct RuntimeStatics;

//================================================================================================================================
//=> - LuckySeatReq / LuckySeatRslt -
//================================================================================================================================
//
//  Args for lucky_seat_run. Caller owns map, statics, start list, and lucky-seat index buffer.
//  Select writes m_lucky_n / m_lucky_seats; boost reads them (and may skip select when m_do_select is 0).
//
//================================================================================================================================

struct LuckySeatReq {
    GameArraySimple* m_map; // Mutated when boosting resources
    const RuntimeStatics* m_statics; // Resource typing for river boost
    const SpgCoordPair* m_starts; // All start coords
    u16 m_start_n; // Length of m_starts
    u16* m_lucky_seats; // Out (and in if select skipped): indices into m_starts
    u16 m_lucky_cap; // Capacity of m_lucky_seats
    u16 m_lucky_n; // In/out lucky count
    u8 m_do_select; // Nonzero: fill lucky seats from map + starts
    u8 m_do_boost; // Nonzero: local + river resource boost around lucky seats
};

struct LuckySeatRslt {
    bool m_ok; // False on bad args or internal failure
    u32 m_local_n; // Resources added by local boost
    u32 m_river_n; // Resources added by river boost
};

#ifdef __cplusplus
extern "C" {
#endif

LuckySeatRslt lucky_seat_run (LuckySeatReq* req);

#ifdef __cplusplus
}
#endif

#endif // LUCKY_SEAT_API_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
