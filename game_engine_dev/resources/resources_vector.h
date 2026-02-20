//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCES_VECTOR_H
#define RESOURCES_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

//================================================================================================================================
//=> - ResourceData struct -
//================================================================================================================================

class BitArrayCL;

typedef struct ResourceTypeStats {
    std::string name;
    uint16_t food;
    uint16_t shields;
    uint16_t income;
} ResourceTypeStats;

typedef struct ResourceData {
    const ResourceTypeStats& stats;
} ResourceData;

//================================================================================================================================
//=> - ResourceIO class -
//================================================================================================================================

class ResourceIO {
public:

    ResourceIO (const std::string& filename) : m_filename(filename) {}

    int validate_and_count () const;
    void print_content () const;
    void parse_and_allocate () const;

private:
    ResourceIO (const ResourceIO& other) = delete;
    ResourceIO (ResourceIO&& other) = delete;

    std::string m_filename;
};

//================================================================================================================================
//=> - ResourceVector class -
//================================================================================================================================

class ResourceVector {
public:

    ResourceVector (uint32_t num_resources, BitArrayCL* resources_available);
    ~ResourceVector ();

    ResourceData get_resource (uint32_t index) const;
    bool is_available (uint32_t index) const;
    uint32_t get_count () const;
    void set_available (uint32_t index);

    static const ResourceTypeStats* get_resource_data_array ();
    static uint32_t get_resource_data_count ();

private:
    ResourceVector (const ResourceVector& other) = delete;
    ResourceVector (ResourceVector&& other) = delete;

    BitArrayCL* m_resources_available;
};

#endif // RESOURCES_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
