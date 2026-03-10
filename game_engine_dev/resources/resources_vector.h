//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCES_VECTOR_H
#define RESOURCES_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "resource_data.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

class BitArrayCL;

//================================================================================================================================
//=> - ResourceVector class -
//================================================================================================================================

class ResourceVector {
public:

    ResourceVector (uint32_t num_resources, BitArrayCL* resources_available);
    ~ResourceVector ();

    ResourceTypeStats get_resource (uint32_t index) const;
    bool is_available (uint32_t index) const;
    uint32_t get_count () const;
    void set_available (uint32_t index);

private:
    ResourceVector (const ResourceVector& other) = delete;
    ResourceVector (ResourceVector&& other) = delete;

    BitArrayCL* m_resources_available;
};

#endif // RESOURCES_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
