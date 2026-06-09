//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_RIVER_LINES_H
#define GENERATE_RIVER_LINES_H

#include "game_primitives.h"
#include "generate_river_network.h"
#include "generate_river_sectors.h"

//================================================================================================================================
//=> - RiverLinesResult -
//================================================================================================================================

struct RiverLinesResult {
    u16 w;
    u16 h;
    u8* overlay;
};

//================================================================================================================================
//=> - Generate_RiverLines -
//================================================================================================================================

class Generate_RiverLines {
public:
    static RiverLinesResult* generate (
        const u8* terrain,
        u16 w,
        u16 h,
        const RiverSectorsResult* sectors,
        const RiverNetworkResult* network,
        u32 seed);
    static void free_result (RiverLinesResult* res);

private:
    Generate_RiverLines () = delete;
    Generate_RiverLines (const Generate_RiverLines& other) = delete;
    Generate_RiverLines (Generate_RiverLines&& other) = delete;
};

#endif // GENERATE_RIVER_LINES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
