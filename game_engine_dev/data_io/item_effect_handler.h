//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ITEM_EFFECT_HANDLER_H
#define ITEM_EFFECT_HANDLER_H

#include <string>

#include "item_effects.h"

struct NameToIdxCbs;

//================================================================================================================================
//=> - ItemEffectHandler -
//================================================================================================================================

class ItemEffectHandler {
public:
    explicit ItemEffectHandler (const NameToIdxCbs* name_to_idx_cbs);

    ItemEffectsStruct parse_effects_line (const std::string& line) const;

    u32 get_error_count () const;
    void reset_error_count ();

private:
    const NameToIdxCbs* m_cbs;
    mutable u32 m_error_count;
};

#endif // ITEM_EFFECT_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
