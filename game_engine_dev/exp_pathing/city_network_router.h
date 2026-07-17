//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_NETWORK_ROUTER_H
#define CITY_NETWORK_ROUTER_H

#include "game_primitives.h"

//================================================================================================================================
//=> - CityNetworkRouter -
//================================================================================================================================
//
//  Hop routing on a CityNetwork. next_hop returns the next city toward `to` on an
//  unweighted BFS tree grown from `to` until `from` is reached (early exit). Scratch
//  buffers are allocated once in begin; each call is heap-free. The table is reused for
//  the same `to` while `from` (and path nodes) remain covered.
//
//================================================================================================================================

class CityNetwork;

class CityNetworkRouter {
public:
    CityNetworkRouter ();
    ~CityNetworkRouter ();

    bool begin (const CityNetwork& net);
    u16 next_hop (u16 from, u16 to) const;
    bool is_valid () const;

private:
    CityNetworkRouter (const CityNetworkRouter& other) = delete;
    CityNetworkRouter (CityNetworkRouter&& other) = delete;

    void clear ();
    void build_to (u16 to, u16 stop) const;

    bool m_ok; // True after successful begin
    u16 m_n; // City count mirrored from network
    mutable u16 m_goal; // Destination for which m_next is valid; U16_KEY_NULL if none
    const CityNetwork* m_net; // Non-owning network
    u16* m_q; // BFS queue (length m_n)
    u16* m_next; // Next hop toward m_goal; U16_KEY_NULL if unreachable
    u32* m_seen; // Visit stamp per city
    mutable u32 m_tick; // Current BFS stamp
};

#endif // CITY_NETWORK_ROUTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
