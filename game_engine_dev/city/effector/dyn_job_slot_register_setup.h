//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DYN_JOB_SLOT_REGISTER_SETUP_H
#define DYN_JOB_SLOT_REGISTER_SETUP_H

#include "dyn_job_slot_register.h"

class RuntimeStatics;

class DynJobSlotRegisterSetup {
public:
    static bool build (const RuntimeStatics& st, DynJobSlotRegister& out);

private:
    DynJobSlotRegisterSetup () = delete;
};

#endif // DYN_JOB_SLOT_REGISTER_SETUP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
