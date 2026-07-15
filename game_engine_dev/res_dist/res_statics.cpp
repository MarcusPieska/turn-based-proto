//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "res_statics.h"
#include "resource_static_key.h"
#include "res_dist_static_key.h"

#include <cstdio>

#include <fcntl.h>
#include <unistd.h>

static ResStatics g_res_statics;
static bool g_res_statics_ok = false;
static bool g_res_statics_done = false;

static bool load_with_null_stdout (const char* lib_path, const char* data_root) {
    std::fflush(stdout);
    const int saved_out = dup(STDOUT_FILENO);
    const int null_fd = open("/dev/null", O_WRONLY);
    if (null_fd >= 0) {
        dup2(null_fd, STDOUT_FILENO);
        close(null_fd);
    }
    const bool ok = g_res_statics.load(lib_path, data_root);
    std::fflush(stdout);
    if (saved_out >= 0) {
        dup2(saved_out, STDOUT_FILENO);
        close(saved_out);
    }
    return ok;
}

//================================================================================================================================
//=> - ResStatics -
//================================================================================================================================

ResStatics::ResStatics () {
}

ResStatics::~ResStatics () {
    unload();
}

bool ResStatics::load (const char* lib_path, const char* data_root) { 
    unload();
    return m_loader.load(lib_path, data_root);
}

void ResStatics::unload () {
    m_loader.unload();
}

void res_statics_session_reset () {
    g_res_statics.unload();
    g_res_statics_done = false;
    g_res_statics_ok = false;
}

bool ResStatics::is_loaded () const {
    return m_loader.is_loaded();
}

const RuntimeStatics& ResStatics::s () const {
    return m_loader.statics();
}

u16 ResStatics::res_n () const {
    if (!m_loader.is_loaded()) {
        return 0;
    }
    return m_loader.statics().resource().get_item_count();
}

cstr ResStatics::res_name (u16 i) const {
    return m_loader.statics().resource().get_name(ResourceStaticDataKey::from_raw(i));
}

bool ResStatics::res_has_plc (u16 i) const {
    const ResourceStaticDataStruct& r = m_loader.statics().resource().get_item(
        ResourceStaticDataKey::from_raw(i));
    const ResDistStaticDataStruct& d = m_loader.statics().res_dist().get_item(
        ResDistStaticDataKey::from_raw(r.res_dist_idx));
    return d.has_plc != 0;
}

const ResPlacement& ResStatics::res_plc (u16 i) const {
    const ResourceStaticDataStruct& r = m_loader.statics().resource().get_item(
        ResourceStaticDataKey::from_raw(i));
    return m_loader.statics().res_dist().get_item(
        ResDistStaticDataKey::from_raw(r.res_dist_idx)).plc;
}

bool ResStatics::ensure_loaded (const char* lib_path, const char* data_root) {
    if (g_res_statics_done) {
        return g_res_statics_ok;
    }
    g_res_statics_done = true;
    g_res_statics_ok = load_with_null_stdout(lib_path, data_root);
    return g_res_statics_ok;
}

bool ResStatics::is_ready () {
    return g_res_statics_done && g_res_statics_ok;
}

const RuntimeStatics& ResStatics::shared () {
    return g_res_statics.s();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
