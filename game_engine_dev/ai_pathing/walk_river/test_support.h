//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include "game_array_simple.h"
#include "game_primitives.h"
#include "map_bit_overlay.h"

//================================================================================================================================
//=> - ExploredRiverOverlay -
//================================================================================================================================

class ExploredRiverOverlay {
public:
    explicit ExploredRiverOverlay (u16 w, u16 h);
    u16 width () const;
    u16 height () const;
    u32 get (u16 x, u16 y) const;
    bool set (u16 x, u16 y);
    void pull (const GameArraySimple& map, const MapBitOverlay& exp);
    u32 cnt () const;

private:
    MapBitOverlay m_ov;
};

//================================================================================================================================
//=> - RiverSystemFinder -
//================================================================================================================================

class RiverSystemFinder {
public:
    RiverSystemFinder (
        const GameArraySimple& map,
        const ExploredRiverOverlay& riv_exp);
    bool find_next (u16& ox, u16& oy) const;
    u32 riv_tot () const;

private:
    const GameArraySimple& m_map;
    const ExploredRiverOverlay& m_re;
};

//================================================================================================================================
//=> - RivSysStat -
//================================================================================================================================

struct RivSysStat {
    u32 m_tiles;
    u32 m_red;
    u32 m_grn;
    u32 m_gry;
};

//================================================================================================================================
//=> - RivSysClrs -
//================================================================================================================================

struct RivSysClrs {
    u8 m_riv_b[3];
    u8 m_riv_w[3];
    u8 m_red[3];
    u8 m_grn[3];
    u8 m_gry[3];
};

//================================================================================================================================
//=> - RivSysImageAudit -
//================================================================================================================================

class RivSysImageAudit {
public:
    RivSysImageAudit ();
    ~RivSysImageAudit ();
    void set_clrs (const RivSysClrs& clrs);
    bool load (const char* path);
    bool analyze ();
    void print_stats () const;
    bool save_sys_map (const char* path) const;
    bool save_bad_map (const char* path) const;
    u32 bad_st_n () const;
    u32 sys_n () const;
    const RivSysStat& stat (u32 i) const;

private:
    static const u16 k_unm = 0xFFFFu;
    RivSysClrs m_clrs;
    u8* m_rgb;
    u16* m_sid;
    u8* m_riv;
    RivSysStat* m_stats;
    u16 m_w;
    u16 m_h;
    u32 m_sys_n;
    bool rgb_eq (u16 x, u16 y, const u8 c[3]) const;
    bool is_riv_px (u16 x, u16 y) const;
    bool is_red_px (u16 x, u16 y) const;
    bool is_grn_px (u16 x, u16 y) const;
    bool is_gry_px (u16 x, u16 y) const;
    void flood_sys (u16 sx, u16 sy, u16 id);
    void clr_buf ();
    static void pal_rgb (u32 id, u8& r, u8& g, u8& b);
};

bool re_riv_audit_run (const char* src, const char* sys_map, const char* bad_map);

#endif // TEST_SUPPORT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
