//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_BIT_OVERLAY_H
#define MAP_BIT_OVERLAY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - MapBitOverlay -
//================================================================================================================================

class MapBitOverlay {
public:
    explicit MapBitOverlay (u16 w, u16 h);
    ~MapBitOverlay ();
    void clear ();
    u16 width () const;
    u16 height () const;
    u32 get (u16 x, u16 y) const;
    bool set (u16 x, u16 y);
    bool clr (u16 x, u16 y);

protected:
    u32 tidx (u16 x, u16 y) const;
    u8 rd_byte (u32 bi) const;
    u32 byte_count () const;

private:
    MapBitOverlay (const MapBitOverlay& other) = delete;
    MapBitOverlay& operator= (const MapBitOverlay& other) = delete;
    u16 m_w;
    u16 m_h;
    u8* m_bits;
    u32 m_bytes;
};

#endif // MAP_BIT_OVERLAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
