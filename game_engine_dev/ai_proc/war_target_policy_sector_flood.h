//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WAR_TARGET_POLICY_SECTOR_FLOOD_H
#define WAR_TARGET_POLICY_SECTOR_FLOOD_H

#include "war_target_policy.h"

//================================================================================================================================
//=> - Sector/flood orchestrator -
//================================================================================================================================
//
//  SectorFlood: only sectors you control or that border them. Reach T via own city in T, or a
//  two-sector probe from controlled neighbor S (own/free/non-lucky steps) that hits a non-lucky
//  foe city in T; then flood T only for order. No deep look-ahead. Fallback: nearest non-lucky.
//
//================================================================================================================================

bool war_target_pick_sector_flood (GameState& st, u16 seat, u16 from_x, u16 from_y, WarTargetPlan* io);

#endif // WAR_TARGET_POLICY_SECTOR_FLOOD_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
