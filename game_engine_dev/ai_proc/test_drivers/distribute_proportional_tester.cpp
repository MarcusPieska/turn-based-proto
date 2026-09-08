//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "continent_size_indexer.h"
#include "distribute_proportional.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    DistPropEnt ents[ContSizeList::k_cap];
    u16 n = 0u;
    ents[n].m_tiles = 117848u;
    ents[n].m_rem = 54u;
    ++n;
    ents[n].m_tiles = 38954u;
    ents[n].m_rem = 21u;
    ++n;
    ents[n].m_tiles = 37918u;
    ents[n].m_rem = 16u;
    ++n;
    ents[n].m_tiles = 3405u;
    ents[n].m_rem = 1u;
    ++n;
    ents[n].m_tiles = 1750u;
    ents[n].m_rem = 1u;
    ++n;
    ents[n].m_tiles = 1014u;
    ents[n].m_rem = 0u;
    ++n;
    ents[n].m_tiles = 448u;
    ents[n].m_rem = 0u;
    ++n;
    const u16 quota = 3u;
    i32 counts[ContSizeList::k_cap];
    for (u16 i = 0; i < ContSizeList::k_cap; ++i) {
        counts[i] = 0;
    }
    if (!DistributeProportional::run(ents, n, quota, counts)) {
        std::printf("*** FAILED DistributeProportional::run\n");
        return 1;
    }
    u32 tot_tiles = 0u;
    i32 tot_take = 0;
    std::printf("quota=%u continents=%u\n", static_cast<unsigned>(quota), static_cast<unsigned>(n));
    for (u16 i = 0; i < n; ++i) {
        tot_tiles += ents[i].m_tiles;
        tot_take += counts[i];
        std::printf("  cont=%u tiles=%u rem=%u take=%d\n",
            static_cast<unsigned>(i + 1u),
            static_cast<unsigned>(ents[i].m_tiles),
            static_cast<unsigned>(ents[i].m_rem),
            counts[i]);
    }
    std::printf("sum_tiles=%u sum_take=%d\n", static_cast<unsigned>(tot_tiles), tot_take);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
