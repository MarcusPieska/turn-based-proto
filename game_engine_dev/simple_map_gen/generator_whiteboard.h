//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATOR_WHITEBOARD_H
#define GENERATOR_WHITEBOARD_H

#include "game_primitives.h"

//================================================================================================================================
//=> - GeneratorWhiteboard -
//================================================================================================================================

class GeneratorWhiteboard {
public:
    static u16* alloc (i32 tile_n);
    static void release (u16* ptr);
    static void dealloc ();
    static u32 chkout ();

private:
    GeneratorWhiteboard () = delete;
    GeneratorWhiteboard (const GeneratorWhiteboard& other) = delete;
    GeneratorWhiteboard (GeneratorWhiteboard&& other) = delete;

    static const u32 k_slot_n = 100u;
    static u16* m_mem[k_slot_n];
    static bool m_out[k_slot_n];
    static u32 m_word_n;
    static u32 m_chkout;
};

#endif // GENERATOR_WHITEBOARD_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
