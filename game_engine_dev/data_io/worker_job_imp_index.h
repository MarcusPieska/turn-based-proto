//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORKER_JOB_IMP_INDEX_H
#define WORKER_JOB_IMP_INDEX_H

#include "game_primitives.h"

//================================================================================================================================
//=> - WorkerJobImpIndex -
//================================================================================================================================
//
//  Derived CSR index: worker_job catalog row -> ordered list of worker_job_imp indices.
//  Built from WorkerJobImpStaticData.worker_job_idx; tables owned after take_ownership.
//
//================================================================================================================================

class WorkerJobImpIndexSetup;

class WorkerJobImpIndex {
public:
    WorkerJobImpIndex ();
    ~WorkerJobImpIndex ();

    void take_ownership ();
    void clear ();

    const u16* imps (u16 job_idx) const;
    u16 imp_n (u16 job_idx) const;
    u16 job_n () const;
    u16 imp_total () const;

private:
    friend class WorkerJobImpIndexSetup;

    u16* m_idx; // Flat imp indices grouped by mother worker_job
    u16* m_off; // Prefix offsets; length job_n + 1
    u16 m_job_n; // worker_job catalog size
    u16 m_imp_n; // Total indexed imps

    WorkerJobImpIndex (const WorkerJobImpIndex& other) = delete;
    WorkerJobImpIndex (WorkerJobImpIndex&& other) = delete;
};

#endif // WORKER_JOB_IMP_INDEX_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
