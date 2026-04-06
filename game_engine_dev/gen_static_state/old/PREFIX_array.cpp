//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "[MEMBER_TAG]_array.h"

//================================================================================================================================
//=> - [CLASS_NAME_PREFIX]Array implementation -
//================================================================================================================================

[CLASS_NAME_PREFIX]Array::[CLASS_NAME_PREFIX]Array (u16 flag_count) :
    m_flags(flag_count) {
}

[CLASS_NAME_PREFIX]Array::~[CLASS_NAME_PREFIX]Array () {
}

bool [CLASS_NAME_PREFIX]Array::is_flagged ([CLASS_NAME_PREFIX]Key key) const {
    return m_flags.get_bit(key.value()) != 0;
}

void [CLASS_NAME_PREFIX]Array::set_flag ([CLASS_NAME_PREFIX]Key key) {
    m_flags.set_bit(key.value());
}

void [CLASS_NAME_PREFIX]Array::clear_flag ([CLASS_NAME_PREFIX]Key key) {
    m_flags.clear_bit(key.value());
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
