//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_STAGING_LANDMASS_H
#define GENERATE_STAGING_LANDMASS_H

#include "game_primitives.h"

class GameState;
class Whiteboard_4B;

//================================================================================================================================
//=> - GenerateStagingLandmass -
//================================================================================================================================
//
//  Labels 4-connected landmasses of own-or-unowned passable tiles (no water/mountains/none).
//  Writes a component id per tile into out; unreached stay k_none. out_n receives component count.
//
//================================================================================================================================

class GenerateStagingLandmass {
public:
    static const u32 k_none = 0xFFFFFFFFu;

    static bool generate (const GameState& s, u16 seat, Whiteboard_4B& out, u32* out_n);

private:
    GenerateStagingLandmass () = delete;
};

#endif // GENERATE_STAGING_LANDMASS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
