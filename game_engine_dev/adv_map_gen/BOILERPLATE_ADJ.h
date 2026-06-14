//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef [PIPE_MACRO_PREFIX][MACRO_TAG]_H
#define [PIPE_MACRO_PREFIX][MACRO_TAG]_H

#include "game_primitives.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - [PIPE_CLASS_PREFIX][MAIN_CLASS_TAG] -
//================================================================================================================================

class [PIPE_CLASS_PREFIX][MAIN_CLASS_TAG] {
public:
    explicit [PIPE_CLASS_PREFIX][MAIN_CLASS_TAG] (const P1_RunPrm& prm);

    bool adjust (u8* terrain, u16 w, u16 h);
    bool is_valid () const;

private:
    [PIPE_CLASS_PREFIX][MAIN_CLASS_TAG] (const [PIPE_CLASS_PREFIX][MAIN_CLASS_TAG]& other) = delete;
    [PIPE_CLASS_PREFIX][MAIN_CLASS_TAG] ([PIPE_CLASS_PREFIX][MAIN_CLASS_TAG]&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_adjust;
};

#endif // [PIPE_MACRO_PREFIX][MACRO_TAG]_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
