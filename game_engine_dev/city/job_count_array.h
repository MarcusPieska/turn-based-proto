//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef JOB_COUNT_ARRAY_H
#define JOB_COUNT_ARRAY_H

#include "game_primitives.h"

class GeneralBitBank;
class RuntimeStatics;

//================================================================================================================================
//=> - JobCountArray -
//================================================================================================================================
//
//  Per-city packed counts for each city_job catalog row. Setup builds a shared offset-width lay from
//  catalog base slots plus theoretical DynJobSlotRegister bonuses (all enablers on). Storage is one
//  GeneralBitBank batch per city; city_idx is the batch index after claim().
//
//================================================================================================================================

struct JobCountSlot {
    u16 m_off; // Bit start within batch
    u8 m_wid; // Bit width
    u8 m_max; // Max storable count (0 .. m_max)
};

class JobCountArray {
public:
    JobCountArray () = delete;

    static bool setup (const RuntimeStatics& st);
    static void clear ();
    static bool ready ();

    static u16 claim ();
    static u16 job_n ();
    static u16 bits ();
    static u8 max_n (u16 job_id);
    static u8 get (u16 city_idx, u16 job_id);
    static void set (u16 city_idx, u16 job_id, u8 n);

    static GeneralBitBank* bank ();
    static const JobCountSlot* slots ();

private:
    static GeneralBitBank* s_bank;
    static JobCountSlot* s_slots;
    static u16 s_job_n;
    static u16 s_bits;
    static bool s_ready;
};

#endif // JOB_COUNT_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
