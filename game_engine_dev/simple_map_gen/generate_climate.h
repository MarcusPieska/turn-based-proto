//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_CLIMATE_H
#define GENERATE_CLIMATE_H

#include "generator_constants.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - Climate -
//================================================================================================================================

class Generate_Climate {
public:
    explicit Generate_Climate (u32 seed);

    bool generate ();
    bool is_valid () const;
    void save_output (cstr path) const;

private:
    Generate_Climate (const Generate_Climate& other) = delete;
    Generate_Climate (Generate_Climate&& other) = delete;

    u32 m_seed;
    bool m_valid_generation;
};

#endif // GENERATE_CLIMATE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
