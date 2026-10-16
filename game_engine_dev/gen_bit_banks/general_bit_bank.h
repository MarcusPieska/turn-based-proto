//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERAL_BIT_BANK_H
#define GENERAL_BIT_BANK_H

#include "game_primitives.h"

class AddAccessHelper;
class City;
class CityArray;
class DynamicBitField;
class GeneralAssessor;
class JobCountArray;
class GameIo;
class GameIoCmp;

//================================================================================================================================
//=> - GeneralBitBank class -
//================================================================================================================================

class GeneralBitBank {
public:
    GeneralBitBank (u16 batch_size);

    bool is_flagged (u16 batch_idx, u16 flag_idx) const;
    void set_flag (u16 batch_idx, u16 flag_idx);
    void clear_flag (u16 batch_idx, u16 flag_idx);
    u32 get_slice (u16 batch_idx, u16 bit_off, u8 width) const;
    void set_slice (u16 batch_idx, u16 bit_off, u8 width, u32 val);

private:
    friend class AddAccessHelper;
    friend class City;
    friend class CityArray;
    friend class DynamicBitField;
    friend class GeneralAssessor;
    friend class JobCountArray;
    friend class GameIo;
    friend class GameIoCmp;

    GeneralBitBank (const GeneralBitBank& other) = delete;
    GeneralBitBank (GeneralBitBank&& other) = delete;
    ~GeneralBitBank ();

    static const u16 MAX_PAGES = 256;
    static const u16 GENERAL_BATCHES_PER_PAGE = 256;

    u16 claim_batch ();

    u8* m_pages[MAX_PAGES];
    u16 m_batch_size;
    u16 m_claimed_batch_count;
    u8 m_allocated_page_count;
};

#endif // GENERAL_BIT_BANK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
