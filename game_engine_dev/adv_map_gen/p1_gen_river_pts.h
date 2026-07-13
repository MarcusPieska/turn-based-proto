//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_RIVER_PTS_H
#define P1_GEN_RIVER_PTS_H

#include "game_primitives.h"
#include "p1_gen_ocean_index.h"
#include "p1_map_size.h"
#include "wb_que_xy.h"

//================================================================================================================================
//=> - P1 river pt limits -
//================================================================================================================================

#define P1_RIVER_LATTICE_STEP_DEF 10u
#define P1_RIVER_LATTICE_STEP_MIN 4u
#define P1_RIVER_PTS_MAX \
    ((static_cast<u32>(P1_MAP_W_DEF) / static_cast<u32>(P1_RIVER_LATTICE_STEP_MIN)) \
    * (static_cast<u32>(P1_MAP_H_DEF) / static_cast<u32>(P1_RIVER_LATTICE_STEP_MIN)))

inline u32 p1_river_pts_max (u16 w, u16 h, u16 step) {
    if (step == 0u) {
        return 0u;
    }
    return static_cast<u32>(w / step) * static_cast<u32>(h / step);
}

inline u32 p1_step_pt_anchor_i (u16 alx, u16 aly, u16 step, u16 w) {
    return static_cast<u32>(aly / step) * static_cast<u32>(w / step) + static_cast<u32>(alx / step);
}

//================================================================================================================================
//=> - P1_StepPtLay -
//================================================================================================================================

struct P1_StepPtLay {
    u16 m_step;
    u16 m_w;
    u32 m_cap;
    u8* m_mrk;
    u16* m_ax;
    u16* m_ay;
};

bool p1_step_pt_lay_init (P1_StepPtLay* lay, u16 w, u16 h, u16 step);
void p1_step_pt_lay_free (P1_StepPtLay* lay);
bool p1_stamp_land_step_lay (const u8* terrain, u16 w, u16 h, u32 seed, P1_StepPtLay* lay);
bool p1_step_pt_lay_at (const P1_StepPtLay* lay, u16 alx, u16 aly, u16* ox, u16* oy);
bool p1_push_ocean_pts (
    const u16* ocn,
    u16 w,
    u16 h,
    u16 ocean_n,
    u16 largest_idx,
    WB_QueXY* que,
    u16* ocn_sec_n);

//================================================================================================================================
//=> - P1_RiverPt -
//================================================================================================================================

struct P1_RiverPt {
    u16 m_id;
    u16 m_x;
    u16 m_y;
};

//================================================================================================================================
//=> - P1_Gen_RiverPtsRslt -
//================================================================================================================================

struct P1_Gen_RiverPtsRslt {
    u16 m_w;
    u16 m_h;
    u16 m_ocn_sec_n;
    u32 m_n;
    WB_QueXY m_que;
};

//================================================================================================================================
//=> - P1_Gen_RiverPts -
//================================================================================================================================

class P1_Gen_RiverPts {
public:
    explicit P1_Gen_RiverPts (const P1_RunPrm& prm, u16 lattice_step = P1_RIVER_LATTICE_STEP_DEF);
    ~P1_Gen_RiverPts ();

    bool generate ();
    bool generate (const u8* terrain, u16 w, u16 h);
    bool generate (const u8* terrain, u16 w, u16 h, const P1_OceanIndexRef& ocean);
    bool is_valid () const;
    const P1_Gen_RiverPtsRslt& result () const;
    P1_Gen_RiverPtsRslt& rslt_mut ();
    u16 lattice_step () const;

private:
    P1_Gen_RiverPts (const P1_Gen_RiverPts& other) = delete;
    P1_Gen_RiverPts (P1_Gen_RiverPts&& other) = delete;

    void clear_rslt ();

    P1_RunPrm m_prm;
    u16 m_lattice_step;
    bool m_valid_generation;
    P1_Gen_RiverPtsRslt m_rslt;
};

#endif // P1_GEN_RIVER_PTS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
