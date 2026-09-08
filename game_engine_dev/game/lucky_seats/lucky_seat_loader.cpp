//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_seat_loader.h"

#include <dlfcn.h>

//================================================================================================================================
//=> - LuckySeatLoader -
//================================================================================================================================

LuckySeatLoader::LuckySeatLoader () :
    m_lib(nullptr),
    m_fn_run(nullptr) {
}

LuckySeatLoader::~LuckySeatLoader () {
    unload();
}

bool LuckySeatLoader::load (const char* lib_path) {
    unload();
    m_lib = dlopen(lib_path, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
    if (m_lib == nullptr) {
        return false;
    }
    m_fn_run = reinterpret_cast<LuckySeatRslt (*)(LuckySeatReq*)>(dlsym(m_lib, "lucky_seat_run"));
    if (m_fn_run == nullptr) {
        unload();
        return false;
    }
    return true;
}

void LuckySeatLoader::unload () {
    m_fn_run = nullptr;
    if (m_lib != nullptr) {
        dlclose(m_lib);
        m_lib = nullptr;
    }
}

bool LuckySeatLoader::is_loaded () const {
    return m_lib != nullptr;
}

LuckySeatRslt LuckySeatLoader::run (LuckySeatReq* req) {
    LuckySeatRslt r = {};
    r.m_ok = false;
    if (m_fn_run == nullptr || req == nullptr) {
        return r;
    }
    return m_fn_run(req);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
