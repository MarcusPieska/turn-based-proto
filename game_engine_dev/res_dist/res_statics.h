//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RES_STATICS_H
#define RES_STATICS_H

#include "runtime_static_loader.h"
#include "res_placement.h"

//================================================================================================================================
//=> - ResStatics -
//================================================================================================================================

class ResStatics {
public:
    ResStatics ();
    ~ResStatics ();

    ResStatics (const ResStatics& o) = delete;
    ResStatics (ResStatics&& o) = delete;

    bool load (const char* lib_path, const char* data_root);
    void unload ();
    bool is_loaded () const;
    const RuntimeStatics& s () const;
    u16 res_n () const;
    cstr res_name (u16 i) const;
    bool res_has_plc (u16 i) const;
    const ResPlacement& res_plc (u16 i) const;

private:
    RuntimeStaticLoader m_loader;
};

#endif // RES_STATICS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
