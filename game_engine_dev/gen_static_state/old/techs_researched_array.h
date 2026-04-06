//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TECHS_RESEARCHED_ARRAY_H
#define TECHS_RESEARCHED_ARRAY_H

#include "techs_researched_array_key.h"
#include "bit_array.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - TechsResearchedArray class -
//================================================================================================================================

class TechsResearchedArray {
public:
    TechsResearchedArray (u16 entry_count);
    ~TechsResearchedArray ();

    bool is_flagged (TechsResearchedKey key) const;
    void set_flag (TechsResearchedKey key);
    void clear_flag (TechsResearchedKey key);

private:
    TechsResearchedArray (const TechsResearchedArray& other) = delete;
    TechsResearchedArray (TechsResearchedArray&& other) = delete;

    BitArrayCL m_flags;
};

#endif // TECHS_RESEARCHED_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
