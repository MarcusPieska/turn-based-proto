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
    BuildingIO (const BuildingIO& other) = delete;
    BuildingIO (BuildingIO&& other) = delete;

    std::string filename;
};

//================================================================================================================================
//=> - BuildingVector class -
//================================================================================================================================

class BuildingVector {
public:

    BuildingVector (uint32_t num_buildings, BitArrayCL* buildings_unlocked);
    ~BuildingVector ();

    BuildingData get_building (int index) const;
    int get_count () const;
    
    void save (const std::string& filename) const;
    void load (const std::string& filename);
    
    void toggle_built (int index);
    void set_available (int index);

private:
    BuildingVector (const BuildingVector& other) = delete;
    BuildingVector (BuildingVector&& other) = delete;

    BitArrayCL* m_built_flags;
    BitArrayCL* m_buildings_unlocked;
    uint32_t m_num_buildings;
};

//================================================================================================================================
//=> - BuildableBuildingVector class -
//================================================================================================================================

class BuildableVector {
public:
    friend class BuildableAssessor;

    BuildableVector (uint32_t num_buildings, const BuildingVector* researched_buildings);
    ~BuildableVector ();

    bool is_buildable (int index) const;
    BuildingData get_building (int index) const;
    int get_count () const;

protected:
    void set_buildable (int index);

private:
    BuildableVector (const BuildableVector& other) = delete;
    BuildableVector (BuildableVector&& other) = delete;

    const BuildingVector* m_researched_buildings;
    BuildingVector* m_buildable_buildings;
};

#endif // BUILDING_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
