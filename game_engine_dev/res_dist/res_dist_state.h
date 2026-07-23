//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RES_DIST_STATE_H
#define RES_DIST_STATE_H

#include "game_primitives.h"

//================================================================================================================================
//= - ResDistState -
//================================================================================================================================
//=
//= Per-resource placement counts for the resource-distribution pipeline. Adjustors add what they
//= place; later steps skip a resource when its count already meets the target quota.
//=
//================================================================================================================================

#define RES_DIST_STATE_CAP 256

class ResDistState {
public:
    ResDistState ();

    ResDistState (const ResDistState& o) = delete;
    ResDistState (ResDistState&& o) = delete;

    bool reset (u16 res_n);
    u16 res_n () const;
    u32 get (u16 res_i) const;
    void add (u16 res_i, u32 n);
    bool met (u16 res_i, u32 target) const;

private:
    u16 m_n; // Resource slot count
    u32 m_cnt[RES_DIST_STATE_CAP]; // Placed count per resource index
};

#endif // RES_DIST_STATE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
