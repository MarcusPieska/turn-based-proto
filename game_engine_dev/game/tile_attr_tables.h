//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_ATTR_TABLES_H
#define TILE_ATTR_TABLES_H

#include "game_primitives.h"
#include "tile_attribute_static_data.h"

class RuntimeStatics;

//================================================================================================================================
//=> - TileAttrTables -
//================================================================================================================================
//
//  Unpacks RuntimeStatics tile_attribute rows into ID-indexed tables matching game_map_defs class ids.
//  Hot path indexes terr/clim/ov/road by map cell ids; OV_RIVERS is stored once as m_riv.
//
//================================================================================================================================

class TileAttrTables {
public:
    static bool setup (const RuntimeStatics& st);
    static void clear ();
    static bool ready ();
    static bool map_name (cstr name, u8* out_kind, u8* out_id);

    static u16 terr_n ();
    static u16 clim_n ();
    static u16 ov_n ();
    static u16 road_n ();

    static const TileAttributeStaticDataStruct& terr (u8 id);
    static const TileAttributeStaticDataStruct& clim (u8 id);
    static const TileAttributeStaticDataStruct& ov (u8 id);
    static const TileAttributeStaticDataStruct& road (u8 id);
    static const TileAttributeStaticDataStruct& riv ();

    static const u8 k_kind_terr = 0u;
    static const u8 k_kind_clim = 1u;
    static const u8 k_kind_ov = 2u;
    static const u8 k_kind_road = 3u;
    static const u8 k_kind_riv = 4u;

private:
    TileAttrTables () = delete;

    static const u16 k_terr_n = 16u; // TERR_TILE_SENTINEL id + 1
    static const u16 k_clim_n = 5u; // CLIMATE_BLACK_SOIL + 1
    static const u16 k_ov_n = 16u; // OV_MAX id + 1
    static const u16 k_road_n = 5u; // ROAD_RAIL + 1

    static bool m_ready; // True after a successful setup
    static TileAttributeStaticDataStruct m_terr[k_terr_n]; // Terrain attrs by TERR_*[0]
    static TileAttributeStaticDataStruct m_clim[k_clim_n]; // Climate attrs by CLIMATE_*
    static TileAttributeStaticDataStruct m_ov[k_ov_n]; // Overlay attrs by OV_*[0]
    static TileAttributeStaticDataStruct m_road[k_road_n]; // Road attrs by ROAD_*
    static TileAttributeStaticDataStruct m_riv; // OV_RIVERS attrs (not an overlay class id)
    static TileAttributeStaticDataStruct m_zero; // Returned for out-of-range ids
};

#endif // TILE_ATTR_TABLES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
