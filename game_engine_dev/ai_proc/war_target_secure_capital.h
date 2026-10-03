//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WAR_TARGET_SECURE_CAPITAL_H
#define WAR_TARGET_SECURE_CAPITAL_H

#include "war_target_policy.h"

//================================================================================================================================
//=> - Secure-capital orchestrator -
//================================================================================================================================
//
//  Flood from capital over walkable tiles to the nearest non-self non-lucky city; that owner is the
//  enemy. Then flood from that city (own/neutral corridors allowed) for that owner's city queue.
//  Called only from war_target_pick; pushes fronts outward from the capital.
//
//================================================================================================================================

bool war_target_secure_capital (GameState& st, u16 seat, u16 from_x, u16 from_y, WarTargetPlan* io);

#endif // WAR_TARGET_SECURE_CAPITAL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
