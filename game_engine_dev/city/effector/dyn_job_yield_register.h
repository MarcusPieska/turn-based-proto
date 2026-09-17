//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DYN_JOB_YIELD_REGISTER_H
#define DYN_JOB_YIELD_REGISTER_H

#include "game_primitives.h"

struct EffectCtx;
class DynJobSlotRegister;

//================================================================================================================================
//=> - DynJobYield -
//================================================================================================================================
//
//  The six city-job base yields used for strategy grouping. Index order is fixed for m_off[].
//
//================================================================================================================================

enum class DynJobYield : u16 {
    FOOD = 0,
    PRODUCTION = 1,
    COMMERCE = 2,
    CULTURE = 3,
    SCIENCE = 4,
    RELIGION = 5,
    COUNT = 6
};

//================================================================================================================================
//=> - DynJobYieldEntry / DynJobYieldPack / DynJobYieldRow -
//================================================================================================================================

struct DynJobYieldEntry {
    u16 m_job_id; // city_job catalog index
    i16 m_score; // Yield amount for this group only (sort key)
};

struct DynJobYieldPack {
    i32 m_food; // Summed food from allocated jobs
    i32 m_production; // Summed production
    i32 m_commerce; // Summed commerce
    i32 m_culture; // Summed culture
    i32 m_science; // Summed science
    i32 m_religion; // Summed religion
    u16 m_n; // Citizens allocated so far this pass
};

struct DynJobYieldRow {
    i16 m_food; // Catalog food
    i16 m_production; // Catalog production
    i16 m_commerce; // Catalog commerce
    i16 m_culture; // Catalog culture
    i16 m_science; // Catalog science
    i16 m_religion; // Catalog religion
    u16 m_slots; // Catalog base slot count
};

//================================================================================================================================
//=> - DynJobYieldRegister -
//================================================================================================================================
//
//  Jobs grouped by base yield (multi-yield jobs in every positive group), sorted by that yield
//  descending. Prefix offsets give O(1) group start. Owns m_remain[], m_taken[], and m_pack; reset_remain
//  clears both scratch arrays (remain U16_KEY_NULL = capacity not yet queried; taken zeroed) and the pack.
//  fill JITs DynJobSlotRegister::capacity on first touch and increments pack/taken until m_n >= pop_limit.
//  Built by DynJobYieldRegisterSetup in statics SO.
//
//================================================================================================================================

class DynJobYieldRegister {
public:
    static constexpr u16 YIELD_N = static_cast<u16>(DynJobYield::COUNT);

    DynJobYieldRegister ();
    ~DynJobYieldRegister ();

    void take_ownership ();
    void clear ();

    DynJobYieldPack& reset_remain ();
    u16* remain ();
    const u16* remain () const;
    u16* taken ();
    const u16* taken () const;
    DynJobYieldPack& pack ();
    const DynJobYieldPack& pack () const;

    u16 fill (
        DynJobYield prefer,
        u16 pop_limit,
        const DynJobSlotRegister& slots,
        const EffectCtx& ctx);

    const DynJobYieldEntry* entries () const;
    u16 entry_count () const;
    u16 job_count () const;
    u16 group_begin (DynJobYield y) const;
    u16 group_end (DynJobYield y) const;
    const DynJobYieldRow* rows () const;

private:
    friend class DynJobYieldRegisterSetup;

    void clear_pack ();

    DynJobYieldEntry* m_entry = nullptr; // Flat entries grouped by DynJobYield
    u16* m_off = nullptr; // Prefix offsets; length YIELD_N + 1
    DynJobYieldRow* m_row = nullptr; // Per-job yields + base slots; length m_job_n
    u16* m_remain = nullptr; // Per-job remaining slots; U16_KEY_NULL = not looked up
    u16* m_taken = nullptr; // Per-job citizens assigned this pass
    DynJobYieldPack m_pack = {}; // Cumulative allocation result for current pass
    u16 m_entry_n = 0; // Total group memberships
    u16 m_job_n = 0; // city_job catalog size

    DynJobYieldRegister (const DynJobYieldRegister& other) = delete;
    DynJobYieldRegister (DynJobYieldRegister&& other) = delete;
};

#endif // DYN_JOB_YIELD_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
