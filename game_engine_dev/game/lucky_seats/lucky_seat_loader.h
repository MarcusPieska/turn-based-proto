//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LUCKY_SEAT_LOADER_H
#define LUCKY_SEAT_LOADER_H

#include "lucky_seat_api.h"

//================================================================================================================================
//=> - LuckySeatLoader -
//================================================================================================================================
//
//  dlopen wrapper for lucky_seats.so (same pattern as MapGenLoader).
//
//================================================================================================================================

class LuckySeatLoader {
public:
    LuckySeatLoader ();
    ~LuckySeatLoader ();

    bool load (const char* lib_path);
    void unload ();
    bool is_loaded () const;
    LuckySeatRslt run (LuckySeatReq* req);

private:
    LuckySeatLoader (const LuckySeatLoader& o) = delete;
    LuckySeatLoader (LuckySeatLoader&& o) = delete;
    LuckySeatLoader& operator= (const LuckySeatLoader& o) = delete;
    LuckySeatLoader& operator= (LuckySeatLoader&& o) = delete;

    void* m_lib;
    LuckySeatRslt (*m_fn_run)(LuckySeatReq*);
};

#endif // LUCKY_SEAT_LOADER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
