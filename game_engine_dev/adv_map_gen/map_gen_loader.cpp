//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_gen_loader.h"

#include <dlfcn.h>

//================================================================================================================================
//=> - MapGenLoader -
//================================================================================================================================

MapGenLoader::MapGenLoader () :
    m_lib(nullptr),
    m_fn_gen(nullptr),
    m_fn_free(nullptr) {
}

MapGenLoader::~MapGenLoader () {
    unload();
}

bool MapGenLoader::load (const char* lib_path) {
    unload();
    m_lib = dlopen(lib_path, RTLD_NOW | RTLD_GLOBAL);
    if (m_lib == nullptr) {
        return false;
    }
    m_fn_gen = reinterpret_cast<MakeMapRslt (*)(const MapGenReq*)>(dlsym(m_lib, "map_gen_generate"));
    m_fn_free = reinterpret_cast<void (*)(MakeMapRslt*)>(dlsym(m_lib, "map_gen_free_rslt"));
    if (m_fn_gen == nullptr || m_fn_free == nullptr) {
        unload();
        return false;
    }
    return true;
}

void MapGenLoader::unload () {
    m_fn_gen = nullptr;
    m_fn_free = nullptr;
    if (m_lib != nullptr) {
        dlclose(m_lib);
        m_lib = nullptr;
    }
}

bool MapGenLoader::is_loaded () const {
    return m_lib != nullptr;
}

MakeMapRslt MapGenLoader::generate (const MapGenReq& req) {
    MakeMapRslt r = {};
    r.m_ok = false;
    if (m_fn_gen == nullptr) {
        return r;
    }
    return m_fn_gen(&req);
}

void MapGenLoader::free_rslt (MakeMapRslt* rslt) {
    if (m_fn_free != nullptr && rslt != nullptr) {
        m_fn_free(rslt);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
