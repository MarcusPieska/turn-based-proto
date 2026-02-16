//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_VECTOR_H
#define BUILDING_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

//================================================================================================================================
//=> - BuildingData struct -
//================================================================================================================================

class BitArrayCL;

struct BuildingData {
    std::string name;
    int cost;
    std::string effect;
    bool exists;
};

//================================================================================================================================
//=> - BuildingIO class -
//================================================================================================================================

class BuildingIO {
public:

    BuildingIO (const std::string& filename) : filename(filename) {}

    int validate_and_count () const;
    void print_content () const;
    void parse_and_allocate () const;

private:
    std::string filename;
};

//================================================================================================================================
//=> - BuildingVector class -
//================================================================================================================================

class BuildingVector {
public:

    BuildingVector (uint32_t num_buildings);
    ~BuildingVector ();

    BuildingData get_building (int index) const;
    int get_count () const;
    
    void save (const std::string& filename) const;
    void load (const std::string& filename);
    
    void toggle_built (int index);
    void set_available (int index);

private:
    BitArrayCL* built_flags;
    BitArrayCL* buildings_available;
    uint32_t num_buildings;
};

#endif // BUILDING_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
