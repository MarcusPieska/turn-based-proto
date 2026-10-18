//================================================================================================================================
//= - Include guards -
//================================================================================================================================

#ifndef TILE_ATTR_TABLES_H
#define TILE_ATTR_TABLES_H

#include "game_primitives.h"
#include "map_terrain_static_data.h"

class RuntimeStatics;

//================================================================================================================================
//= - TileAttrTables -
//================================================================================================================================
//
//  Hot-path attr rows copied from RuntimeStatics map_terrain / map_climate / map_overlay /
//  map_attribute catalogs. Indexes match those catalog ids (and m_terr / m_clim / m_ov / m_road_typ).
//  Waterway "River" is stored once as riv(); roads are kind-local among Road rows.
//
//================================================================================================================================

typedef MapTerrainStaticDataStruct TileAttrRow;

class TileAttrTables {
public:
    static bool setup (const RuntimeStatics& st);
    static void clear ();
    static bool ready ();
    static bool unpack_cond (u16 packed, u8* out_kind, u8* out_id);
    static u16 pack_cond (u8 kind, u16 id);

    static u16 terr_n ();
    static u16 clim_n ();
    static u16 ov_n ();
    static u16 road_n ();

    static const TileAttrRow& terr (u8 id);
    static const TileAttrRow& clim (u8 id);
    static const TileAttrRow& ov (u8 id);
    static const TileAttrRow& road (u8 id);
    static const TileAttrRow& riv ();

    static const u8 k_kind_terr = 0u;
    static const u8 k_kind_clim = 1u;
    static const u8 k_kind_ov = 2u;
    static const u8 k_kind_road = 3u;
    static const u8 k_kind_riv = 4u;

private:
    TileAttrTables () = delete;

    static const u16 k_terr_n = 16u;
    static const u16 k_clim_n = 8u;
    static const u16 k_ov_n = 16u;
    static const u16 k_road_n = 8u;

    static bool m_ready;
    static TileAttrRow m_terr[k_terr_n];
    static TileAttrRow m_clim[k_clim_n];
    static TileAttrRow m_ov[k_ov_n];
    static TileAttrRow m_road[k_road_n];
    static TileAttrRow m_riv;
    static TileAttrRow m_zero;
    static u16 m_terr_n;
    static u16 m_clim_n;
    static u16 m_ov_n;
    static u16 m_road_n;
};

#endif // TILE_ATTR_TABLES_H

//================================================================================================================================
//= - End of file -
//================================================================================================================================
