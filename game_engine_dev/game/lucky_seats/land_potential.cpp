//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "land_potential.h"

#include "game_array_simple.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - LandPotential -
//================================================================================================================================

bool LandPotential::score (
    const GameArraySimple& map,
    const SpgCoordPair* starts,
    u16 n,
    i32* out_scores)
{
    if (starts == nullptr || out_scores == nullptr || n == 0u) {
        return false;
    }
    if (map.width() == 0u || map.height() == 0u) {
        return false;
    }
    TileYieldCtx ctx = {};
    ctx.m_tech = nullptr;
    TileYields::bind_map(&map);
    TileYields::bind_ctx(&ctx);
    const i32 half = static_cast<i32>(k_box / 2u);
    const i32 mw = static_cast<i32>(map.width());
    const i32 mh = static_cast<i32>(map.height());
    for (u16 i = 0; i < n; ++i) {
        const i32 cx = static_cast<i32>(starts[i].x);
        const i32 cy = static_cast<i32>(starts[i].y);
        i32 tot = 0;
        for (i32 dy = -half; dy <= half; ++dy) {
            for (i32 dx = -half; dx <= half; ++dx) {
                const i32 x = cx + dx;
                const i32 y = cy + dy;
                if (x < 0 || y < 0 || x >= mw || y >= mh) {
                    continue;
                }
                const TileYield yld = TileYields::get(static_cast<u16>(x), static_cast<u16>(y));
                tot += static_cast<i32>(yld.m_food);
                tot += static_cast<i32>(yld.m_production);
                tot += static_cast<i32>(yld.m_commerce);
            }
        }
        out_scores[i] = tot;
    }
    TileYields::bind_ctx(nullptr);
    TileYields::bind_map(nullptr);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
