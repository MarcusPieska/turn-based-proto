//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_VECTOR_H
#define UNIT_VECTOR_H

#include <string>
#include <iosfwd>
#include <cstdint>

//================================================================================================================================
//=> - UnitData struct -
//================================================================================================================================

class BitArrayCL;

typedef struct UnitTypeStats {
    std::string name;
    uint16_t cost;
    uint16_t attack;
    uint16_t defense;
    uint16_t moves;
} UnitTypeStats;

typedef struct UnitData {
    const UnitTypeStats& stats;
    uint16_t strength;
} UnitData;

//================================================================================================================================
//=> - UnitIO class -
//================================================================================================================================

class UnitIO {
public:

    UnitIO (const std::string& filename) : m_filename(filename) {}

    int validate_and_count () const;
    void print_content () const;
    void parse_and_allocate () const;

private:
    UnitIO (const UnitIO& other) = delete;
    UnitIO (UnitIO&& other) = delete;

    std::string m_filename;
};

//================================================================================================================================
//=> - UnitVector class -
//================================================================================================================================

class UnitVector {
public:
    friend class TrainableAssessor;

    UnitVector (const BitArrayCL* researched_units);
    ~UnitVector ();

    UnitData get_unit (uint32_t index) const;
    bool is_trainable (uint32_t index) const;
    uint32_t get_count () const;

    static uint32_t get_unit_data_count ();
    static const UnitTypeStats* get_unit_data_array ();

protected:
    void set_trainable (uint32_t index);

private:
    UnitVector (const UnitVector& other) = delete;
    UnitVector (UnitVector&& other) = delete;
    UnitVector () = delete;

    const BitArrayCL* m_researched_units;
    BitArrayCL* m_units_unlocked;
};

#endif // UNIT_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
