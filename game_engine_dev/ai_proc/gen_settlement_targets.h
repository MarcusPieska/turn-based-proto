//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_SETTLEMENT_TARGETS_H
#define GEN_SETTLEMENT_TARGETS_H

#include "game_primitives.h"
#include "starting_point_generator.h"

class GameArraySimple;

//================================================================================================================================
//=> - GenSettlementTargetsRslt -
//================================================================================================================================

struct GenSettlementTargetsRslt {
    u32 m_n; // Total planned sites written
    u32 m_seed_n; // Sites taken from start args
    u32 m_river_n; // Sites added on river walk
    u32 m_pack_n; // Sites added in land pack pass
};

//================================================================================================================================
//=> - GenSettlementTargets -
//================================================================================================================================
//
//  One-shot settle-site planner: seed starts, walk rivers from those seeds, then pack remaining land.
//  Spacing uses CityBlockingMask stamps into m_settler_blocked; sites are marked on m_planned_city.
//
//================================================================================================================================

class GenSettlementTargets {
public:
    static bool generate (GameArraySimple& map, const SpgCoordPair* starts, u32 start_n, GenSettlementTargetsRslt* out);

private:
    GenSettlementTargets () = delete;
};

#endif // GEN_SETTLEMENT_TARGETS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
