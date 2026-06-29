//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef AI_WHITEBOARD_H
#define AI_WHITEBOARD_H

#include "game_primitives.h"

//================================================================================================================================
//=> - AiWhiteboard -
//================================================================================================================================

class AiWhiteboard {
public:
    static u16* alloc (i32 tile_n);
    static void release (u16* ptr);
    static void dealloc ();
    static u32 chkout ();

private:
    AiWhiteboard () = delete;
    AiWhiteboard (const AiWhiteboard& other) = delete;
    AiWhiteboard (AiWhiteboard&& other) = delete;

    static const u32 k_slot_n = 100u;
    static u16* m_mem[k_slot_n];
    static bool m_out[k_slot_n];
    static u32 m_word_n;
    static u32 m_chkout;
};

class AiWbSheet {
public:
    AiWbSheet (i32 tile_n);
    ~AiWbSheet ();
    u16* get ();
    u16 rd (u32 i) const;
    void wr (u32 i, u16 v);
    bool ok () const;

private:
    AiWbSheet (const AiWbSheet& other) = delete;
    AiWbSheet (AiWbSheet&& other) = delete;
    AiWbSheet& operator= (const AiWbSheet& other) = delete;
    AiWbSheet& operator= (AiWbSheet&& other) = delete;

    u16* m_p;
};

#endif // AI_WHITEBOARD_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
