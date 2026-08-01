//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DYN_PRODUCE_REGISTER_SETUP_H
#define DYN_PRODUCE_REGISTER_SETUP_H

#include "dyn_produce_register.h"

class RuntimeStatics;

class DynProduceRegisterSetup {
public:
    static bool build (const RuntimeStatics& st, DynProduceRegister& out);

private:
    DynProduceRegisterSetup () = delete;
};

#endif // DYN_PRODUCE_REGISTER_SETUP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
