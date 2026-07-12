//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WB_QUE_XY_H
#define WB_QUE_XY_H

#include "game_primitives.h"

class Whiteboard_2B;

//================================================================================================================================
//=> - WB_QueXY -
//================================================================================================================================
//
//  Ring queue of tile (x,y) pairs backed by two u16 whiteboard sheets (one entry per map tile).
//
//================================================================================================================================

class WB_QueXY {
public:
    WB_QueXY ();
    ~WB_QueXY ();
    bool ok () const;
    u32 cap () const;
    u32 count () const;
    void clear ();
    bool push (u16 x, u16 y);
    u16 x_at (u32 i) const;
    u16 y_at (u32 i) const;
    void drop (u32 n);
    void swap (WB_QueXY& o) noexcept;

private:
    WB_QueXY (const WB_QueXY& other) = delete;
    WB_QueXY (WB_QueXY&& other) = delete;
    WB_QueXY& operator= (const WB_QueXY& other) = delete;
    WB_QueXY& operator= (WB_QueXY&& other) = delete;

    u32 slot (u32 i) const;
    void wr_u16 (u32 wi, u16 v) const;
    u16 rd_u16 (u32 wi) const;

    Whiteboard_2B* m_wb0;
    Whiteboard_2B* m_wb1;
    u16* m_p;
    u16* m_p2;
    u32 m_ent_n;
    u32 m_head;
    u32 m_cnt;
};

#endif // WB_QUE_XY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
