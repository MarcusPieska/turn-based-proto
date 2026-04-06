//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCES_AVAILABLE_ARRAY_H
#define RESOURCES_AVAILABLE_ARRAY_H

#include "resources_available_array_key.h"
#include "bit_array.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - ResourcesAvailableArray class -
//================================================================================================================================

class ResourcesAvailableArray {
public:
    ResourcesAvailableArray (u16 entry_count);
    ~ResourcesAvailableArray ();

    bool is_flagged (ResourcesAvailableKey key) const;
    void set_flag (ResourcesAvailableKey key);
    void clear_flag (ResourcesAvailableKey key);

private:
    ResourcesAvailableArray (const ResourcesAvailableArray& other) = delete;
    ResourcesAvailableArray (ResourcesAvailableArray&& other) = delete;

    BitArrayCL m_flags;
};

#endif // RESOURCES_AVAILABLE_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
