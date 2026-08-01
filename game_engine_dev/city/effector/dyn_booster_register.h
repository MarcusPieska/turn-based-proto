//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DYN_BOOSTER_REGISTER_H
#define DYN_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "game_primitives.h"
#include "item_effects.h"

struct EffectCtx;

//================================================================================================================================
//=> - DynBoosterRegister -
//================================================================================================================================
//
//  Booster effects keyed by (booster_type, scope). Tables owned after
//  take_ownership; built by DynBoosterRegisterSetup in statics SO. Query via
//  determine(type, scope, ctx).
//
//================================================================================================================================

class DynBoosterRegister {
public:
    static constexpr u16 TYPE_N = static_cast<u16>(ItemEffectBoosterType::SANITATION) + 1u;
    static constexpr u16 SCOPE_N = static_cast<u16>(ItemEffectsScope::GLOBAL) + 1u;
    static constexpr u16 KEY_N = TYPE_N * SCOPE_N;

    DynBoosterRegister ();
    ~DynBoosterRegister ();

    void take_ownership ();
    void clear ();

    BoosterRegisterResult determine (ItemEffectBoosterType tp, ItemEffectsScope sc, const EffectCtx& ctx) const;

    const BoosterRegisterEntry* entries (ItemEffectBoosterType tp, ItemEffectsScope sc) const;
    u16 entry_count (ItemEffectBoosterType tp, ItemEffectsScope sc) const;
    u16 entry_count () const;
    u16 key_count () const;

private:
    friend class DynBoosterRegisterSetup;

    BoosterRegisterEntry* m_entry = nullptr; // Flat rows grouped by (type, scope)
    u16* m_off = nullptr; // Prefix offsets; length KEY_N + 1
    u16 m_entry_n = 0; // Total booster rows kept

    static u16 key_of (ItemEffectBoosterType tp, ItemEffectsScope sc);
    static bool src_on (ItemEffectsScope sc, const EffectEnabler& en, const EffectCtx& ctx);

    DynBoosterRegister (const DynBoosterRegister& other) = delete;
    DynBoosterRegister (DynBoosterRegister&& other) = delete;
};

#endif // DYN_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
