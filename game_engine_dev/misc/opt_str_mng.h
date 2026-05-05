//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef OPT_STR_MNG_H
#define OPT_STR_MNG_H

#include "game_primitives.h"

//================================================================================================================================
//=> - StringManager class -
//================================================================================================================================

class StringManager {
public:
    StringManager();
    ~StringManager();
    StringManager(const StringManager&) = delete;
    StringManager& operator=(const StringManager&) = delete;
    bool load_file_content(cstr file_path);
    bool load_cstr_content(cstr content);
    void split_string_by_char(u32 string_index, char split_char);
    void trim_head_char(u32 string_index, char trim_char);
    void trim_tail_char(u32 string_index, char trim_char);
    void cull_empty_strings();
    cstr get_string_content(u32 string_index) const;
    u32 get_string_count() const;

private:
    void rst();
    char* m_buf;
    u32 m_buf_sz;
    u32* m_idx;
    u32 m_idx_n;
};

#endif // OPT_STR_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
