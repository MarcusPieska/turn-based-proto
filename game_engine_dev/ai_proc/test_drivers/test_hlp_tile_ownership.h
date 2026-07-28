//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TEST_HLP_TILE_OWNERSHIP_H
#define TEST_HLP_TILE_OWNERSHIP_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - TestHlpTileOwnership -
//================================================================================================================================
//
//  Test-driver sector flood, ownership paint, and widest shared-border pick.
//
//================================================================================================================================

class TestHlpTileOwnership {
public:
    static const u16 k_sec_none = 0xFFFFu;
    static const u16 k_seed_cap = 256u;

    static bool mock_sectors (GameState& s, u16* ov, u16* out_sec_n, u16* seed_x, u16* seed_y);
    static void apply_owners (GameState& s, const u16* ov);

    static bool find_wide_border (const u16* ov, u16 w, u16 h, u16 sec_n, u16* pa, u16* pb, u32* out_n);

private:
    TestHlpTileOwnership () = delete;
};

#endif // TEST_HLP_TILE_OWNERSHIP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
