//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SECTOR_NETWORK_ROUTER_H
#define SECTOR_NETWORK_ROUTER_H

#include "game_primitives.h"

//================================================================================================================================
//=> - SectorPath -
//================================================================================================================================
//
//  Land hop sequence from a source sector toward a destination. Each step is a neighbor bit
//  index (0..5), stored two per byte (low nibble = even step, high = odd).
//
//================================================================================================================================

#define SN_PATH_BYTES 64u
#define SN_PATH_MAX (SN_PATH_BYTES * 2u)

struct SectorPath {
    u8 m_n; // Hop count (0 if already at destination)
    u8 m_pack[SN_PATH_BYTES]; // Packed neighbor bits

    void set (u8 i, u8 v);
    u8 get (u8 i) const;
};

//================================================================================================================================
//=> - SectorNetworkRouter -
//================================================================================================================================
//
//  Builds a SectorPath via an unweighted BFS on land links from `to` until `from` (early
//  exit). Scratch buffers are allocated once in begin; find itself does no heap work.
//
//================================================================================================================================

class SectorNetwork;

class SectorNetworkRouter {
public:
    SectorNetworkRouter ();
    ~SectorNetworkRouter ();

    bool begin (const SectorNetwork& net);
    bool find (u16 from, u16 to, SectorPath* out) const;
    bool is_valid () const;

private:
    SectorNetworkRouter (const SectorNetworkRouter& other) = delete;
    SectorNetworkRouter (SectorNetworkRouter&& other) = delete;

    void clear ();
    bool build_to (u16 to, u16 stop) const;
    static u8 bit_to (const SectorNetwork& net, u16 from, u16 nxt);

    bool m_ok; // True after successful begin
    u16 m_n; // Sector count mirrored from network
    const SectorNetwork* m_net; // Non-owning network
    u16* m_q; // BFS queue (length m_n)
    u16* m_next; // Next hop toward goal during build; U16_KEY_NULL if unseen
    u32* m_seen; // Visit stamp per sector
    mutable u32 m_tick; // Current BFS stamp
};

#endif // SECTOR_NETWORK_ROUTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
