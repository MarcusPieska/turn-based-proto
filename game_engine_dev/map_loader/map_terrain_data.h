//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_TERRAIN_DATA_H
#define MAP_TERRAIN_DATA_H

#include "game_primitives.h"

//================================================================================================================================
//=> - MapTerrainData -
//================================================================================================================================

class MapTerrainData {
public:
    MapTerrainData ();
    ~MapTerrainData ();
    u16 width () const;
    u16 height () const;
    const u8* data () const;
    void clear ();
    bool assign_copy (u16 w, u16 h, const u8* src);
    bool assign_raw (u16 w, u16 h, const u8* src);
    bool save_terrain_ppm (cstr path) const;

private:
    MapTerrainData (const MapTerrainData& other) = delete;
    MapTerrainData (MapTerrainData&& other) = delete;
    MapTerrainData& operator= (const MapTerrainData& other) = delete;
    MapTerrainData& operator= (MapTerrainData&& other) = delete;
    u16 m_w;
    u16 m_h;
    u8* m_rows;
};

#endif // MAP_TERRAIN_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
