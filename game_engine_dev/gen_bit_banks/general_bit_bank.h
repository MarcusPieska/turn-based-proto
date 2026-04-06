//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERAL_BIT_BANK_H
#define GENERAL_BIT_BANK_H

#include "game_primitives.h"

//================================================================================================================================
//=> - GeneralBitBank class -
//================================================================================================================================

class GeneralBitBank {
public:
    GeneralBitBank (u16 batch_size);

private:
    friend class City;

    GeneralBitBank (const GeneralBitBank& other) = delete;
    GeneralBitBank (GeneralBitBank&& other) = delete;
    ~GeneralBitBank ();

    static const u16 MAX_PAGES = 256;
    static const u16 GENERAL_BATCHES_PER_PAGE = 256;

    u16 claim_batch ();
    bool is_flagged (u16 batch_idx, u16 flag_idx) const;
    void set_flag (u16 batch_idx, u16 flag_idx);
    void clear_flag (u16 batch_idx, u16 flag_idx);

    u8* m_pages[MAX_PAGES];
    u16 m_batch_size;
    u16 m_claimed_batch_count;
    u8 m_allocated_page_count;
};

#endif // GENERAL_BIT_BANK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
