//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_GEN_API_H
#define MAP_GEN_API_H

#include "game_primitives.h"
#include "map_config.h"

struct RuntimeStatics;

enum MapGenType : u8 {
    MAP_CONTINENTAL = 1u,
};

struct MapGenReq {
    u32 m_seed;
    MapGenType m_type;
    u16 m_w;
    u16 m_h;
    MapConfig m_cfg;
    const RuntimeStatics* m_statics;
};

struct MakeMapRslt {
    bool m_ok;
    u16 m_w;
    u16 m_h;
    u8* m_terrain;
    u8* m_climate;
    u8* m_rivers;
    u8* m_overlay;
    u16* m_resources;
};

#ifdef __cplusplus
extern "C" {
#endif

MakeMapRslt map_gen_generate (const MapGenReq* req);
void map_gen_free_rslt (MakeMapRslt* rslt);

#ifdef __cplusplus
}
#endif

#endif // MAP_GEN_API_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
