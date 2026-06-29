//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_BIT_ARRAY_OVERLAY_H
#define MAP_BIT_ARRAY_OVERLAY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - MapBitArrayOverlay -
//================================================================================================================================

class MapBitArrayOverlay {
public:
    MapBitArrayOverlay (u16 w, u16 h, u8 bits_per_val);
    ~MapBitArrayOverlay ();
    void clear ();
    u16 width () const;
    u16 height () const;
    u8 bits_per_val () const;
    u32 max_val () const;
    u32 get (u16 x, u16 y) const;
    bool set (u16 x, u16 y, u32 val);
    u32 get_uint (u16 x, u16 y) const;
    bool set_uint (u16 x, u16 y, u32 val);
    void fill_all (u32 val);
    void assign_u8 (const u8* vals);

protected:
    u32 tidx (u16 x, u16 y) const;
    u8 rd_byte (u32 bi) const;
    u32 byte_count () const;

private:
    MapBitArrayOverlay (const MapBitArrayOverlay& other) = delete;
    MapBitArrayOverlay& operator= (const MapBitArrayOverlay& other) = delete;
    u32 val_mask () const;
    u16 m_w;
    u16 m_h;
    u8 m_bpv;
    u8* m_bits;
    u32 m_bytes;
};

#endif // MAP_BIT_ARRAY_OVERLAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
