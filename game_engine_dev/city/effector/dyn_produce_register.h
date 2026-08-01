//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DYN_PRODUCE_REGISTER_H
#define DYN_PRODUCE_REGISTER_H

#include "effect_enabler.h"
#include "game_primitives.h"
#include "item_effects.h"

struct EffectCtx;

//================================================================================================================================
//=> - DynProduceSlot / DynProduceGroup -
//================================================================================================================================

struct DynProduceSlot {
    ItemProduceKind m_kind; // RESOURCE -> dense inv; YIELD -> city yields
    u16 m_target; // Dense inv idx (RESOURCE) or ItemProduceYield
    i16 m_amount; // Signed; negative consumes
};

struct DynProduceGroup {
    EffectEnabler m_enabler; // Source that must be active for this city apply
    u8 m_slot_n; // Produce legs on this source ( <= MAX_EFFECT_COUNT )
    DynProduceSlot m_slots[MAX_EFFECT_COUNT]; // Fixed slots mirroring effect capacity
};

//================================================================================================================================
//=> - DynProduceRegister -
//================================================================================================================================
//
//  Produce groups with a dense inventory over resources that appear in produce(...)
//  legs. Tables owned after take_ownership; built by DynProduceRegisterSetup in
//  statics SO. apply gathers active hosts first, pay-checks only those, and
//  clamps only slots that were written.
//
//================================================================================================================================

class DynProduceRegister {
public:
    static constexpr u16 YIELD_N = static_cast<u16>(ItemProduceYield::SANITATION) + 1u;

    struct Inventory {
        i16* m_v = nullptr; // Dense stock; length m_n
        u16 m_n = 0; // Dense slot count
    };

    struct CityYields {
        i16 m_v[YIELD_N]; // Indexed by ItemProduceYield; slot 0 unused
    };

    DynProduceRegister ();
    ~DynProduceRegister ();

    void take_ownership ();
    void clear ();

    void clear_yld (CityYields& yld) const;
    void apply (Inventory& inv, CityYields& yld, const EffectCtx& ctx) const;

    const DynProduceGroup* groups () const;
    u16 group_count () const;
    u16 inv_n () const;
    u16 res_n () const;
    u16 res_id (u16 dense) const;
    u16 dense_of (u16 catalog_id) const;

private:
    friend class DynProduceRegisterSetup;

    DynProduceGroup* m_group = nullptr; // One group per producing host
    u16* m_act = nullptr; // Scratch: active group indices
    u8* m_touch_res = nullptr; // Scratch: dense inv slots written
    u16* m_res_ids = nullptr; // dense -> catalog resource id
    u16* m_res_map = nullptr; // catalog id -> dense (or U16_KEY_NULL)
    u16 m_group_n = 0; // Group count
    u16 m_inv_n = 0; // Dense inventory size
    u16 m_res_n = 0; // Full resource catalog size

    DynProduceRegister (const DynProduceRegister& other) = delete;
    DynProduceRegister (DynProduceRegister&& other) = delete;
};

#endif // DYN_PRODUCE_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
