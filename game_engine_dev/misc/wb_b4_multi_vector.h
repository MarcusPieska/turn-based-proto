//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WB_B4_MULTI_VECTOR_H
#define WB_B4_MULTI_VECTOR_H

#include "game_primitives.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - WB_B4MultiVec -
//================================================================================================================================
//
//  vec_n append/read u32 segments in one Whiteboard_4B sheet; vid selects segment. cap(vid)=floor(tile_n/vec_n).
//
//================================================================================================================================

class WB_B4MultiVec {
public:
    WB_B4MultiVec (cstr cls, cstr fn, u32 turn, u16 vec_n);
    ~WB_B4MultiVec ();
    bool ok () const;
    u16 vec_n () const;
    u32 cap (u16 vid) const;
    u32 count (u16 vid) const;
    void clear (u16 vid);
    void clear_all ();
    bool push (u16 vid, u32 v);
    u32 at (u16 vid, u32 i) const;
    void set (u16 vid, u32 i, u32 v);
    cstr cls () const;
    cstr fn () const;

private:
    WB_B4MultiVec (const WB_B4MultiVec& o) = delete;
    WB_B4MultiVec& operator= (const WB_B4MultiVec& o) = delete;

    u32 seg_base (u16 vid) const;
    bool vid_ok (u16 vid) const;

    Whiteboard_4B m_wb;
    u32* m_p;
    u16 m_vec_n;
    u32 m_seg_cap;
    u32 m_cnt[16];
    cstr m_cls;
    cstr m_fn;
};

#endif // WB_B4_MULTI_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
