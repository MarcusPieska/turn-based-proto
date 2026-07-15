//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EFFECT_INDEX_SELECTOR_H
#define EFFECT_INDEX_SELECTOR_H

#include "item_effects.h"

//================================================================================================================================
//=> - EffectIndexSelector -
//================================================================================================================================
//
//  Base for static booster registers. Codegen will populate derived register tables by booster type and scope.
//
//================================================================================================================================

class EffectIndexSelector {
protected:
    EffectIndexSelector () = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::NONE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::NONE;
    }

private:
    EffectIndexSelector (const EffectIndexSelector& other) = delete;
    EffectIndexSelector (EffectIndexSelector&& other) = delete;
};

#endif // EFFECT_INDEX_SELECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
