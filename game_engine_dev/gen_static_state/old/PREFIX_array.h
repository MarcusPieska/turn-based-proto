//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef [MACRO_PREFIX]_ARRAY_H
#define [MACRO_PREFIX]_ARRAY_H

#include "[MEMBER_TAG]_array_key.h"
#include "bit_array.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - [CLASS_NAME_PREFIX]Array class -
//================================================================================================================================

class [CLASS_NAME_PREFIX]Array {
public:
    [CLASS_NAME_PREFIX]Array (u16 entry_count);
    ~[CLASS_NAME_PREFIX]Array ();

    bool is_flagged ([CLASS_NAME_PREFIX]Key key) const;
    void set_flag ([CLASS_NAME_PREFIX]Key key);
    void clear_flag ([CLASS_NAME_PREFIX]Key key);

private:
    [CLASS_NAME_PREFIX]Array (const [CLASS_NAME_PREFIX]Array& other) = delete;
    [CLASS_NAME_PREFIX]Array ([CLASS_NAME_PREFIX]Array&& other) = delete;

    BitArrayCL m_flags;
};

#endif // [MACRO_PREFIX]_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
