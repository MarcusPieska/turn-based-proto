//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_WORKER_DRAFT_BOOSTER_REGISTER_H
#define CIV_WORKER_DRAFT_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivWorkerDraftBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped WORKER_DRAFT booster register.
//
//================================================================================================================================

class CivWorkerDraftBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 0;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_civ, ctx);
    }

private:
    CivWorkerDraftBoosterRegister () = delete;
    CivWorkerDraftBoosterRegister (const CivWorkerDraftBoosterRegister& other) = delete;
    CivWorkerDraftBoosterRegister (CivWorkerDraftBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::WORKER_DRAFT;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CIV_WORKER_DRAFT_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
