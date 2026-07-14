//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WB_2B_MULTI_VECTOR_H
#define WB_2B_MULTI_VECTOR_H

#include "game_primitives.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - WB_2BMultiVec -
//================================================================================================================================
//
//  vec_n append/read u16 segments in one Whiteboard_2B sheet; vid selects segment. cap(vid)=floor(tile_n/vec_n).
//
//================================================================================================================================

class WB_2BMultiVec {
public:
    WB_2BMultiVec (cstr cls, cstr fn, u32 turn, u16 vec_n);
    ~WB_2BMultiVec ();
    bool ok () const;
    u16 vec_n () const;
    u32 cap (u16 vid) const;
    u32 count (u16 vid) const;
    void clear (u16 vid);
    void clear_all ();
    bool push (u16 vid, u16 v);
    u16 at (u16 vid, u32 i) const;
    void set (u16 vid, u32 i, u16 v);
    cstr cls () const;
    cstr fn () const;

private:
    WB_2BMultiVec (const WB_2BMultiVec& o) = delete;
    WB_2BMultiVec& operator= (const WB_2BMultiVec& o) = delete;

    u32 seg_base (u16 vid) const;
    bool vid_ok (u16 vid) const;

    Whiteboard_2B m_wb;
    u16* m_p;
    u16 m_vec_n;
    u32 m_seg_cap;
    u32 m_cnt[16];
    cstr m_cls;
    cstr m_fn;
};

#endif // WB_2B_MULTI_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
