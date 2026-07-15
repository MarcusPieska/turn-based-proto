//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef STARTING_POINT_GENERATOR_H
#define STARTING_POINT_GENERATOR_H

#include "game_primitives.h"
#include "map_terrain_data.h"

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

#define SPG_MAX_LATT_PTS 10000
#define SPG_MAX_PICK_PTS 256

//================================================================================================================================
//=> - SpgPt -
//================================================================================================================================

struct SpgPt {
    f32 x;
    f32 y;
};

//================================================================================================================================
//=> - SpgCoordPair -
//================================================================================================================================

struct SpgCoordPair {
    u16 x;
    u16 y;
};

//================================================================================================================================
//=> - SpgPickCoords -
//================================================================================================================================

struct SpgPickCoords {
    SpgCoordPair pts[SPG_MAX_PICK_PTS];
    u32 n;
};

//================================================================================================================================
//=> - StartingPointGeneratorParams -
//================================================================================================================================

struct StartingPointGeneratorParams {
    const MapTerrainData* map;
    const u8* climate = nullptr; // Per-tile climate; same w*h as map; null skips climate check
    const u8* overlay = nullptr; // Per-tile overlay; same w*h as map; null skips overlay check
    u16 pick_n; 

    u16 latt_rows = 100;
    u16 latt_cols = 100;
    f32 jitter_frac = 0.5f;
    u32 seed = 0;
};

//================================================================================================================================
//=> - StartingPointGenerator -
//================================================================================================================================

class StartingPointGenerator {
public:
    explicit StartingPointGenerator (const StartingPointGeneratorParams& par);
    ~StartingPointGenerator ();
    bool generate ();
    u32 lattice_count () const;
    u32 land_count () const;
    u32 candidate_count () const;
    u32 pick_count () const;
    u16 dist_max () const;
    bool pick_skipped () const;
    SpgPickCoords picks_coords () const;
    u16 pick_target () const;
    const SpgPt* candidates () const;
    const SpgPt* picks () const;
    u16 map_width () const;
    u16 map_height () const;
    const u8* terrain () const;
    const u8* dist_gray () const;
    bool picks_are_start_land () const;

private:
    StartingPointGenerator (const StartingPointGenerator& other) = delete;
    StartingPointGenerator& operator= (const StartingPointGenerator& other) = delete;
    StartingPointGenerator (StartingPointGenerator&& other) = delete;
    StartingPointGenerator& operator= (StartingPointGenerator&& other) = delete;
    bool chk_par () const;
    bool is_water (u8 cls) const;
    bool is_land (u8 cls) const;
    bool is_start_tile (u8 terr, u8 clim, u8 ov) const;
    bool adj_water (u16 px, u16 py) const;
    bool bfs_dist ();
    void mk_gray ();
    u8 gray_at (f32 x, f32 y) const;
    u8 flip_norm (u16 d) const;
    void mk_lattice ();
    void filt_land ();
    void filt_prob ();
    void pick_spaced ();
    void free_bufs ();
    StartingPointGeneratorParams m_p;
    u16 m_w;
    u16 m_h;
    const u8* m_cls;
    const u8* m_clim;
    const u8* m_ov;
    u16* m_dist;
    u8* m_gray;
    u16 m_dist_max;
    SpgPt* m_latt;
    u32 m_latt_n;
    SpgPt* m_land;
    u32 m_land_n;
    SpgPt* m_cand;
    u32 m_cand_n;
    SpgPt* m_pick;
    u32 m_pick_n;
    bool m_pick_skip;
    bool m_ok;
};

#endif // STARTING_POINT_GENERATOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
