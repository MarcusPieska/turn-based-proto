//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DYN_BOOSTER_REGISTER_SETUP_H
#define DYN_BOOSTER_REGISTER_SETUP_H

#include "dyn_booster_register.h"

class RuntimeStatics;

class DynBoosterRegisterSetup {
public:
    static bool build (const RuntimeStatics& st, DynBoosterRegister& out);

private:
    DynBoosterRegisterSetup () = delete;
};

#endif // DYN_BOOSTER_REGISTER_SETUP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
