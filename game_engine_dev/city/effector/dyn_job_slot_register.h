//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DYN_JOB_SLOT_REGISTER_H
#define DYN_JOB_SLOT_REGISTER_H

#include "effect_enabler.h"
#include "game_primitives.h"
#include "item_effects.h"

struct EffectCtx;

//================================================================================================================================
//=> - DynJobSlotEntry -
//================================================================================================================================

struct DynJobSlotEntry {
    EffectEnabler m_enabler; // Source that must be active
    u16 m_job_id; // city_job catalog index
    i16 m_amount; // COUNT addend, or PERCENTAGE of base
    ItemEffectsScope m_scope; // LOCAL treated as CITY at query time
    ItemEffectAmountMode m_mode; // COUNT or PERCENTAGE
};

//================================================================================================================================
//=> - DynJobSlotRegister -
//================================================================================================================================
//
//  jobSlots(...) effects keyed by job_id. Tables owned after take_ownership;
//  built by DynJobSlotRegisterSetup in statics SO. Same query shape as
//  JobSlotRegister.
//
//================================================================================================================================

class DynJobSlotRegister {
public:
    DynJobSlotRegister ();
    ~DynJobSlotRegister ();

    void take_ownership ();
    void clear ();

    i16 bonus (u16 job_id, u16 base_slots, const EffectCtx& ctx) const;
    u16 capacity (u16 job_id, u16 base_slots, const EffectCtx& ctx) const;

    const DynJobSlotEntry* entries () const;
    u16 entry_count () const;
    u16 job_count () const;

private:
    friend class DynJobSlotRegisterSetup;

    DynJobSlotEntry* m_entry = nullptr; // Flat entries grouped by job_id
    u16* m_off = nullptr; // Prefix offsets; length job_n + 1
    u16 m_entry_n = 0; // Number of jobSlots rows
    u16 m_job_n = 0; // city_job catalog size

    DynJobSlotRegister (const DynJobSlotRegister& other) = delete;
    DynJobSlotRegister (DynJobSlotRegister&& other) = delete;
};

#endif // DYN_JOB_SLOT_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
