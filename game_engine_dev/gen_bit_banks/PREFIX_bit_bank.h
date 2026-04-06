//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef [MACRO_PREFIX]_BIT_BANK_H
#define [MACRO_PREFIX]_BIT_BANK_H

#include "game_primitives.h"

//================================================================================================================================
//=> - [CLASS_NAME_PREFIX]BitBank class -
//================================================================================================================================

class [CLASS_NAME_PREFIX]BitBank {
public:
    [CLASS_NAME_PREFIX]BitBank (u16 batch_size);

private:
    friend class [USER_CLASS_NAME];

    [CLASS_NAME_PREFIX]BitBank (const [CLASS_NAME_PREFIX]BitBank& other) = delete;
    [CLASS_NAME_PREFIX]BitBank ([CLASS_NAME_PREFIX]BitBank&& other) = delete;
    ~[CLASS_NAME_PREFIX]BitBank ();

    static const u16 MAX_PAGES = 256;
    static const u16 [MACRO_PREFIX]_BATCHES_PER_PAGE = 256;

    u16 claim_batch ();
    bool is_flagged (u16 batch_idx, u16 flag_idx) const;
    void set_flag (u16 batch_idx, u16 flag_idx);
    void clear_flag (u16 batch_idx, u16 flag_idx);

    u8* m_pages[MAX_PAGES];
    u16 m_batch_size;
    u16 m_claimed_batch_count;
    u8 m_allocated_page_count;
};

#endif // [MACRO_PREFIX]_BIT_BANK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
