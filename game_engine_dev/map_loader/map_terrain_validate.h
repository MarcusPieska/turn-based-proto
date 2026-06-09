//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_TERRAIN_VALIDATE_H
#define MAP_TERRAIN_VALIDATE_H

#include "game_primitives.h"

//================================================================================================================================
//=> - MapTerrainValidate -
//================================================================================================================================

class MapTerrainValidate {
public:
    static bool chk_classes (const u8* rows, u32 n);
    static bool chk_rgb (const u8* rgb, u32 px_n);
    static u8 class_from_rgb (u8 r, u8 g, u8 b, bool* matched);
    static void rgb_from_class (u8 cls, u8* r, u8* g, u8* b);

private:
    MapTerrainValidate () = delete;
    MapTerrainValidate (const MapTerrainValidate& other) = delete;
    MapTerrainValidate (MapTerrainValidate&& other) = delete;
    MapTerrainValidate& operator= (const MapTerrainValidate& other) = delete;
    MapTerrainValidate& operator= (MapTerrainValidate&& other) = delete;
};

#endif // MAP_TERRAIN_VALIDATE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
