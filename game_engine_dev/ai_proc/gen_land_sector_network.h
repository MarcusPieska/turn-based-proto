//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_LAND_SECTOR_NETWORK_H
#define GEN_LAND_SECTOR_NETWORK_H

#include "game_primitives.h"
#include "land_sector_network.h"

class Whiteboard_2B;

//================================================================================================================================
//=> - GenLandSectorNetwork -
//================================================================================================================================
//
//  Builds undirected adjacency for GenLandSectors paint (tag = sector id + 1). Two sectors link when
//  they share a 4-neighbor walkable border. Output is a LandSectorNetwork link array.
//
//================================================================================================================================

class GenLandSectorNetwork {
public:
    GenLandSectorNetwork () = delete;

    static bool build (const Whiteboard_2B& sec, u16 sec_n, LandSectorNetwork* out);

private:
};

#endif // GEN_LAND_SECTOR_NETWORK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
