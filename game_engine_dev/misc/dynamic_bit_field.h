//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DYNAMIC_BIT_FIELD_H
#define DYNAMIC_BIT_FIELD_H

#include "game_primitives.h"

class GeneralBitBank;

//================================================================================================================================
//=> - DynamicBitFieldLay -
//================================================================================================================================
//
//  Shared field layout. Built once; many DynamicBitField instances borrow it via ctor.
//  mk2/mk3 use m_slots (bit offset + width per field). mk1 may leave m_slots null.
//  mk3 also needs m_bits as GeneralBitBank batch_size.
//
//================================================================================================================================

struct DynamicBitFieldSlot {
    u16 m_off; // Bit start of field
    u8 m_wid; // Bit width of field
};

struct DynamicBitFieldLay {
    u16 m_n; // Field count
    u8* m_vals; // Values-per-field (0 .. vals[i]-1)
    DynamicBitFieldSlot* m_slots; // Offset-width tuples; mk2/mk3
    u32 m_bytes; // Packed byte size (mk1/mk2 storage)
    u16 m_bits; // Packed bit count; mk3 batch_size
};

//================================================================================================================================
//=> - DynamicBitField -
//================================================================================================================================
//
//  Runtime multi-field store. Layout is external (DynamicBitFieldLay*), bound via ctor.
//  mk1: one byte per field. mk2: owned packed bytes. mk3: GeneralBitBank batch via u16 index.
//  Body from impl/dynamic_bit_field_impl_mkNN.cpp.
//
//================================================================================================================================

class DynamicBitField {
public:
    explicit DynamicBitField (const DynamicBitFieldLay* lay, GeneralBitBank* bank = nullptr);
    ~DynamicBitField ();
    DynamicBitField (const DynamicBitField&) = delete;
    DynamicBitField& operator= (const DynamicBitField&) = delete;

    u16 n () const;
    u8 vals (u16 i) const;
    u32 bytes () const;
    u16 ix () const;
    u8 get (u16 i) const;
    void set (u16 i, u8 v);

private:
    u8* m_data;                                                         // mk1/mk2 storage; owned (null in mk3)
    u16 m_ix;                                                           // mk3 bank batch index
};

#endif // DYNAMIC_BIT_FIELD_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
