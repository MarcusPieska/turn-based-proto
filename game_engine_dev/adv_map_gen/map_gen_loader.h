//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_GEN_LOADER_H
#define MAP_GEN_LOADER_H

#include "map_gen_api.h"

//================================================================================================================================
//=> - MapGenLoader -
//================================================================================================================================

class MapGenLoader {
public:
    MapGenLoader ();
    ~MapGenLoader ();

    bool load (const char* lib_path);
    void unload ();
    bool is_loaded () const;
    MakeMapRslt generate (const MapGenReq& req);
    void free_rslt (MakeMapRslt* rslt);

private:
    MapGenLoader (const MapGenLoader& o) = delete;
    MapGenLoader (MapGenLoader&& o) = delete;
    MapGenLoader& operator= (const MapGenLoader& o) = delete;
    MapGenLoader& operator= (MapGenLoader&& o) = delete;

    void* m_lib;
    MakeMapRslt (*m_fn_gen)(const MapGenReq*);
    void (*m_fn_free)(MakeMapRslt*);
};

#endif // MAP_GEN_LOADER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
