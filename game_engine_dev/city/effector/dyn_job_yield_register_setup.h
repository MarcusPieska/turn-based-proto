//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DYN_JOB_YIELD_REGISTER_SETUP_H
#define DYN_JOB_YIELD_REGISTER_SETUP_H

#include "dyn_job_yield_register.h"

class RuntimeStatics;

class DynJobYieldRegisterSetup {
public:
    static bool build (const RuntimeStatics& st, DynJobYieldRegister& out);

private:
    DynJobYieldRegisterSetup () = delete;
};

#endif // DYN_JOB_YIELD_REGISTER_SETUP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
