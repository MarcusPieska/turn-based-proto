//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RIVER_SECT_ADJ_H
#define P1_GEN_RIVER_SECT_ADJ_H

#include "game_primitives.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_map_size.h"

class Whiteboard_1B;
class Whiteboard_2B;

//================================================================================================================================
//=> - P1 river sector adjacency limits -
//================================================================================================================================

#define P1_RIVER_SECT_ADJ_MAX 15u

//================================================================================================================================
//=> - P1_Gen_RiverSectAdjRslt -
//================================================================================================================================
//
//  Packed sector adjacency; m_nb[si * P1_RIVER_SECT_ADJ_MAX + k] is neighbor sector id.
//
//================================================================================================================================

struct P1_Gen_RiverSectAdjRslt {
    u16 m_w;
    u16 m_h;
    u16 m_sector_n;
    const u8* m_nb_n;
    const u16* m_nb;
};

static inline u16 p1_sect_adj_nb (const P1_Gen_RiverSectAdjRslt& adj, u16 si, u8 k) {
    return adj.m_nb[static_cast<u32>(si) * static_cast<u32>(P1_RIVER_SECT_ADJ_MAX) + static_cast<u32>(k)];
}

//================================================================================================================================
//=> - P1_Gen_RiverSectAdj -
//================================================================================================================================

class P1_Gen_RiverSectAdj {
public:
    explicit P1_Gen_RiverSectAdj (const P1_RunPrm& prm);
    ~P1_Gen_RiverSectAdj ();

    bool generate (const P1_Gen_RiverSectorsRslt& sectors);
    bool is_valid () const;
    const P1_Gen_RiverSectAdjRslt& result () const;

private:
    P1_Gen_RiverSectAdj (const P1_Gen_RiverSectAdj& other) = delete;
    P1_Gen_RiverSectAdj (P1_Gen_RiverSectAdj&& other) = delete;

    void clear_rslt ();

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_RiverSectAdjRslt m_rslt;
    Whiteboard_1B* m_wb_nb_n;
    Whiteboard_2B* m_wb_nb;
};

#endif // P1_GEN_RIVER_SECT_ADJ_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
