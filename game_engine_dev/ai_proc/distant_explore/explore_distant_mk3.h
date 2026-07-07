//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EXPLORE_DISTANT_MK3_H
#define EXPLORE_DISTANT_MK3_H

#include "explore_distant_mk1.h"
#include "near_path_mk1.h"

//================================================================================================================================
//=> - ExploreDistantMk3 -
//================================================================================================================================

class ExploreDistantMk3 : public ExploreAi {
public:
    ExploreDistantMk3 (const GameArraySimple& map, MapBitOverlay& explored, u16 sx, u16 sy, u16 sight, u8 player);
    
    ~ExploreDistantMk3 () override;
    void move (u16 moves) override;
    bool done () const;
    u16 path_n () const;
    u16 wp_i () const;
    u16 wp_x (u16 i) const;
    u16 wp_y (u16 i) const;
    double derive_sec () const;

private:
    static const u16 k_path_max = 1000u;
    u16* m_wpx;
    u16* m_wpy;
    u16 m_wp_n;
    u16 m_wp_i;
    bool m_done;
    double m_derive_sec;
    NearPathMk1 m_path;
    void note_step (MapBitOverlay& exp, u16 x, u16 y, u16& st_n, u16* stx, u16* sty);
    void reveal_around ();
    void derive_path ();
};

#endif // EXPLORE_DISTANT_MK3_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
