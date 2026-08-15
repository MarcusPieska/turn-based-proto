//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORKER_JOB_IMP_INDEX_SETUP_H
#define WORKER_JOB_IMP_INDEX_SETUP_H

#include "worker_job_imp_index.h"

class RuntimeStatics;

class WorkerJobImpIndexSetup {
public:
    static bool build (const RuntimeStatics& st, WorkerJobImpIndex& out);

private:
    WorkerJobImpIndexSetup () = delete;
};

#endif // WORKER_JOB_IMP_INDEX_SETUP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
