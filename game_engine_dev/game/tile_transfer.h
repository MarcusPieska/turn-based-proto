//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_TRANSFER_H
#define TILE_TRANSFER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - TileTransfer -
//================================================================================================================================
//
//  After a city capture: flip loser-owned tiles in the culture radius when the captured city is
//  at least as near as every remaining defender city that can claim the tile (inside that city's
//  culture disc). Attacker and third-party cities never block. Competitors are gathered by growing
//  discs r=1,2,.. to 1.5x R with dedup. Own and competitor R are at least CityBorder::radius_for
//  (founding claim culture=25), since CircularTileAreas r=0 and r=1 are center-only. Equal distance
//  to a defender city favors the capturer. City tiles are never flipped.
//
//================================================================================================================================

class TileTransfer {
public:
    static u32 apply (GameState& state, u16 city_idx, u8 from_owner, u8 to_owner, u16* out_near = nullptr);

private:
    TileTransfer () = delete;
};

#endif // TILE_TRANSFER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
