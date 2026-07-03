//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef STATIC_STRING_POOL_H
#define STATIC_STRING_POOL_H

#include "game_primitives.h"

//================================================================================================================================
//=> - StaticStringPool -
//================================================================================================================================

class StaticStringPool {
public:
    StaticStringPool ();
    StaticStringPool (u16 str_cap, u32 char_n);
    StaticStringPool (const StaticStringPool& o);
    ~StaticStringPool ();
    StaticStringPool& operator= (const StaticStringPool& o);
    bool add (cstr s, u16* out_k);
    cstr get (u16 k) const;
    u16 get_str_n () const;
    u16 get_str_cap () const;
    u32 get_char_n () const;
    u32 get_char_cap () const;
    void reset ();

private:
    void clr ();
    char* m_buf;
    u32 m_buf_cap;
    u32 m_buf_n;
    u32* m_off;
    u16 m_off_cap;
    u16 m_off_n;
};

#endif // STATIC_STRING_POOL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
