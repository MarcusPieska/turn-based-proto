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
//  Derived CSR index: map_overlay catalog row -> ordered list of worker_job_imp indices.
//  Built from WorkerJobImpStaticData.map_overlay_idx; tables owned after take_ownership.
//
//================================================================================================================================

class WorkerJobImpIndexSetup;

class WorkerJobImpIndex {
public:
    WorkerJobImpIndex ();
    ~WorkerJobImpIndex ();

    void take_ownership ();
    void clear ();

    const u16* imps (u16 ov_idx) const;
    u16 imp_n (u16 ov_idx) const;
    u16 ov_n () const;
    u16 imp_total () const;

private:
    friend class WorkerJobImpIndexSetup;

    u16* m_idx; // Flat imp indices grouped by mother map_overlay
    u16* m_off; // Prefix offsets; length ov_n + 1
    u16 m_ov_n; // map_overlay catalog size
    u16 m_imp_n; // Total indexed imps

    WorkerJobImpIndex (const WorkerJobImpIndex& other) = delete;
    WorkerJobImpIndex (WorkerJobImpIndex&& other) = delete;
};

#endif // WORKER_JOB_IMP_INDEX_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
