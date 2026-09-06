//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCE_EFFECTOR_H
#define RESOURCE_EFFECTOR_H

#include "game_primitives.h"
#include "item_effects.h"

struct ResourceExtractCtx;
class RuntimeStatics;

//================================================================================================================================
//=> - ResourceEffectSrc -
//================================================================================================================================

enum class ResourceEffectSrc : u8 {
    BUILDING = 0,
    TECH = 1,
    SMALL_WONDER = 2,
    WONDER = 3,
    WORKER_JOB_IMP = 4
};

//================================================================================================================================
//=> - ResourceEffectEntry -
//================================================================================================================================

struct ResourceEffectEntry {
    ResourceEffectSrc m_src; // Host catalog family for m_idx
    u16 m_idx; // Host row index
    i16 m_unit; // COUNT-mode amount when active
    i16 m_perc; // PERCENTAGE-mode amount when active
    ItemEffectsScope m_scope; // LOCAL / CITY / CIV / GLOBAL
};

//================================================================================================================================
//=> - ResourceEffector -
//================================================================================================================================
//
//  Reverse index of booster(RESOURCE, ...) legs (plus reserved per-res rows). setup scans
//  catalogs once; yield walks m_any and the res_idx CSR slice, testing enablers against
//  ResourceExtractCtx (imps via tile; tech/bld/wonder via banks).
//
//================================================================================================================================

class ResourceEffector {
public:
    ResourceEffector () = delete;

    static bool setup (const RuntimeStatics& st);
    static void clear ();

    static u16 yield (u16 res_idx, u16 base, const ResourceExtractCtx& ctx);

    static u16 any_n ();
    static u16 entry_n ();
    static u16 res_n ();

private:
    static ResourceEffectEntry* m_any; // Generic RESOURCE boosters (all extracts)
    static u16 m_any_n; // Length of m_any
    static ResourceEffectEntry* m_entry; // Flat per-res rows; CSR by res_idx
    static u16* m_off; // Prefix offsets; length m_res_n + 1
    static u16 m_entry_n; // Length of m_entry
    static u16 m_res_n; // Resource catalog size

    static bool src_on (const ResourceEffectEntry& e, const ResourceExtractCtx& ctx);
    static void accum (const ResourceEffectEntry* rows, u16 n, const ResourceExtractCtx& ctx, i16& unit, i16& perc);
};

#endif // RESOURCE_EFFECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
