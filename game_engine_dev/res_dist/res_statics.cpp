//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "res_statics.h"
#include "resource_static_key.h"
#include "res_dist_static_key.h"

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

//================================================================================================================================
//=> - End -
//================================================================================================================================
