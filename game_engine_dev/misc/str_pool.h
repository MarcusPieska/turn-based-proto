//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef STR_POOL_H
#define STR_POOL_H

#include "game_primitives.h"

//================================================================================================================================
//=> - StringPool class -
//================================================================================================================================

class StringPool {
public:
    StringPool(u32 char_capacity, u16 string_capacity);
    ~StringPool();
    StringPool(const StringPool&) = delete;
    StringPool& operator=(const StringPool&) = delete;
    bool push_string(cstr source, u16* out_id);
    char* get_string(u16 string_id);
    cstr get_string(u16 string_id) const;
    u16 get_string_count() const;
    u16 get_string_capacity() const;
    u32 get_char_count() const;
    u32 get_char_capacity() const;

private:
    void clear();
    char* m_buf;
    u32 m_buf_cap;
    u32 m_buf_n;
    u32* m_idx;
    u16 m_idx_cap;
    u16 m_idx_n;
};

#endif // STR_POOL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
