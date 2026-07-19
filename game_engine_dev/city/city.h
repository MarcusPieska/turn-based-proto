//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_H
#define CITY_H

#include "game_primitives.h"

class BitArrayCL;
class GeneralBitBank;
class RuntimeStatics;
class UnitAddVector;

struct PlayerState;

//================================================================================================================================
//=> - City class -
//================================================================================================================================
//
//  Per-city runtime state: growth/production buckets and build queue. Flag/resource/building bits live in CityArray banks.
//  Location and owner are set via init when a dormant page slot is activated; constructor only zeroes production fields.
//
//================================================================================================================================

class alignas(8) City {
public:
    City ();
    ~City ();

    static void bind_statics (const RuntimeStatics& st);
    static void clear_assess_scratch ();
    static void bind_banks (GeneralBitBank* flags, GeneralBitBank* resources, GeneralBitBank* buildings);
    static void bind_units (UnitAddVector* units);
    static void bind_wonder_cities (u16* wonder_city);
    static void bind_player_states (PlayerState* states, u16 player_n);

    void init (u16 owner, u16 x, u16 y);

    BitArrayCL* get_buildable_buildings (u16 city_idx, BitArrayCL* techs, BitArrayCL* civ) const;
    BitArrayCL* get_buildable_wonders (u16 city_idx, BitArrayCL* techs, BitArrayCL* civ) const;
    BitArrayCL* get_buildable_small_wonders (u16 city_idx, BitArrayCL* techs, BitArrayCL* civ) const;
    BitArrayCL* get_trainable_units (u16 city_idx, BitArrayCL* techs, BitArrayCL* civ) const; 

    void build_building (u16 building_idx);
    void build_wonder (u16 wonder_idx);
    void build_small_wonder (u16 small_wonder_idx);
    void build_unit (u16 unit_idx);
    void accumulate_commerce ();

    bool add_food (u16 city_idx, u16 amount);
    bool add_production (u16 city_idx, u16 amount);
    void add_commerce (u16 city_idx, u16 amount);
    void add_culture (u16 city_idx, u16 amount);

    u16 get_current_food_store () const;
    u16 get_current_production_store () const;
    u16 get_current_culture () const;

    u16 get_current_population () const;
    void set_population (u16 pop);
    u16 get_owner () const;
    u16 get_x () const;
    u16 get_y () const;
    bool is_frontier () const;
    void city_no_longer_frontier ();

    bool finish_if_ready (u16 city_idx);
    bool has_building (u16 city_idx, u16 building_idx) const;

private:
    u16 m_owner; // Owning seat index; U16_KEY_NULL until init
    u16 m_x; // Map column; U16_KEY_NULL until init
    u16 m_y; // Map row; U16_KEY_NULL until init
    u16 m_bld_idx; // Active catalog index; U16_KEY_NULL if queue empty
    
    u16 m_pop_count; // City population
    u16 m_build_cost; // Current build cost; saved to avoid constant lookups
    u16 m_accumulated_production; // Production bucket toward current build
    u16 m_culture; // Culture score for this city; determines the city's borders
    
    u8 m_accumulated_food; // Food bucket toward growth
    u8 m_build_type; // Active build category 
    u8 m_is_frontier_city; // Helper for AI settler sensing; true if city is near unclaimed territory
};

#endif // CITY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
