//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATOR_CONSTANT_H
#define GENERATOR_CONSTANT_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Terrain rows: [0]=class id stored in map, [1..3]=RGB at save -
//================================================================================================================================

static const u8 TERR_NONE[4] = {0, 255, 0, 255};
static const u8 TERR_OCEAN[4] = {1, 14, 52, 112};
static const u8 TERR_SEA[4] = {2, 38, 102, 188};
static const u8 TERR_COASTAL[4] = {3, 118, 182, 242};
static const u8 TERR_PLAINS[4] = {4, 34, 112, 48};
static const u8 TERR_HILLS[4] = {5, 50, 140, 78};
static const u8 TERR_MOUNTAINS[4] = {6, 76, 48, 30};

//================================================================================================================================
//=> - Overlay gray levels -
//================================================================================================================================

static const u8 WL_OVERLAY_WATER_GRAY = 255;
static const u8 WL_OVERLAY_LAND_GRAY = 128;

static const u8 OVERLAY_OMITTED = 0;
static const u8 OVERLAY_SELECTED1 = 255;
static const u8 OVERLAY_SELECTED2 = 128;

//================================================================================================================================
//=> - MapArrayClimate -
//================================================================================================================================

class MapArrayClimate {
public:
    MapArrayClimate ();
    ~MapArrayClimate ();

    MapArrayClimate (const MapArrayClimate& other) = delete;
    MapArrayClimate& operator= (const MapArrayClimate& other) = delete;
    MapArrayClimate (MapArrayClimate&& other) = delete;
    MapArrayClimate& operator= (MapArrayClimate&& other) = delete;

    u16 width () const;
    u16 height () const;
    const u8* data () const;
    u8* data_w ();

    bool save (cstr path) const;

private:
    u16 m_w;
    u16 m_h;
    u8* m_d;
};

//================================================================================================================================
//=> - MapArrayTerrain -
//================================================================================================================================

class MapArrayTerrain {
public:
    MapArrayTerrain ();
    ~MapArrayTerrain ();

    MapArrayTerrain (const MapArrayTerrain& other) = delete;
    MapArrayTerrain& operator= (const MapArrayTerrain& other) = delete;
    MapArrayTerrain (MapArrayTerrain&& other) = delete;
    MapArrayTerrain& operator= (MapArrayTerrain&& other) = delete;

    u16 width () const;
    u16 height () const;
    const u8* data () const;
    u8* data_w ();

    void clear ();
    bool assign_copy (u16 w, u16 h, const u8* src);
    bool save (cstr path) const;

private:
    u16 m_w;
    u16 m_h;
    u8* m_d;
};

//================================================================================================================================
//=> - MapArrayOverlay -
//================================================================================================================================

class MapArrayOverlay {
public:
    MapArrayOverlay ();
    ~MapArrayOverlay ();

    MapArrayOverlay (const MapArrayOverlay& other) = delete;
    MapArrayOverlay& operator= (const MapArrayOverlay& other) = delete;
    MapArrayOverlay (MapArrayOverlay&& other) = delete;
    MapArrayOverlay& operator= (MapArrayOverlay&& other) = delete;

    u16 width () const;
    u16 height () const;
    const u8* data () const;
    u8* data_w ();

    void clear ();
    bool resize (u16 w, u16 h);
    bool assign_copy (u16 w, u16 h, const u8* src);
    bool save (cstr path) const;

private:
    u16 m_w;
    u16 m_h;
    u8* m_d;
};

//================================================================================================================================
//=> - MapArrayDistance -
//================================================================================================================================

class MapArrayDistance {
public:
    MapArrayDistance ();
    ~MapArrayDistance ();

    MapArrayDistance (const MapArrayDistance& other) = delete;
    MapArrayDistance& operator= (const MapArrayDistance& other) = delete;
    MapArrayDistance (MapArrayDistance&& other) = delete;
    MapArrayDistance& operator= (MapArrayDistance&& other) = delete;

    u16 width () const;
    u16 height () const;
    const u16* data () const;
    u16* data_w ();

    void clear ();
    bool resize (u16 w, u16 h);
    bool assign_copy (u16 w, u16 h, const u16* src);
    bool save (cstr path) const;

private:
    u16 m_w;
    u16 m_h;
    u16* m_d;
};

#endif // GENERATOR_CONSTANT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
