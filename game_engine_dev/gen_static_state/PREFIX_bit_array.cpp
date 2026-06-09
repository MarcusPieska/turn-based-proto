//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "[MEMBER_TAG]_bit_array.h"

//================================================================================================================================
//=> - [BIT_ARRAY_CLASS] implementation -
//================================================================================================================================

[BIT_ARRAY_CLASS]::[BIT_ARRAY_CLASS] (u16 count) :
    m_flags(count) {
}

[BIT_ARRAY_CLASS]::~[BIT_ARRAY_CLASS] () {
}

bool [BIT_ARRAY_CLASS]::is_flagged ([DATA_KEY] key) const {
    return m_flags.get_bit(key.value()) != 0;
}

void [BIT_ARRAY_CLASS]::set_flag ([DATA_KEY] key) {
    m_flags.set_bit(key.value());
}

void [BIT_ARRAY_CLASS]::clear_flag ([DATA_KEY] key) {
    m_flags.clear_bit(key.value());
}

bool [BIT_ARRAY_CLASS]::can_access (u16 idx, [DATA_KEY]& out_key) const {
    if (idx >= m_flags.get_count()) {
        return false;
    }
    if (m_flags.get_bit(idx) == 0) {
        return false;
    }
    out_key = [DATA_KEY]::from_raw(idx);
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
