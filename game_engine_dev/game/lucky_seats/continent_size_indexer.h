//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CONTINENT_SIZE_INDEXER_H
#define CONTINENT_SIZE_INDEXER_H

#include "game_primitives.h"

class Whiteboard_2B;
class Whiteboard_4B;

//================================================================================================================================
//=> - ContSizeEnt / ContSizeList -
//================================================================================================================================
//
//  Top land masses by tile count (largest first). m_id is the flood label used while indexing.
//
//================================================================================================================================

struct ContSizeEnt {
    u16 m_id; // Flood mass id (1-based); 0 unused
    u32 m_tiles; // Tile count for this mass
};

struct ContSizeList {
    static const u16 k_cap = 20u; // Max masses retained

    ContSizeEnt m_e[k_cap]; // Ranked masses, largest first
    u16 m_n; // Valid entries in m_e
};

//================================================================================================================================
//=> - ContinentSizeIndexer -
//================================================================================================================================
//
//  4-neighbor land flood (ocean-index style, inverted). Keeps the 20 largest masses and paints them
//  into out_rgb as distinct half-bright colors; all other tiles stay black. Also writes remapped
//  ranks 1..m_n into out_idx (0 = not in the top list). Needs WhiteboardMng init.
//
//================================================================================================================================

class ContinentSizeIndexer {
public:
    static const u16 k_top = ContSizeList::k_cap; // Alias for callers

    static bool index (
        const u8* terr,
        u16 w,
        u16 h,
        ContSizeList* out,
        Whiteboard_4B& out_rgb,
        Whiteboard_2B& out_idx);

private:
    ContinentSizeIndexer () = delete;
};

#endif // CONTINENT_SIZE_INDEXER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
