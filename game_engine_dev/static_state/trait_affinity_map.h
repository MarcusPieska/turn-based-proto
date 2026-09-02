//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TRAIT_AFFINITY_MAP_H
#define TRAIT_AFFINITY_MAP_H

#include "game_primitives.h"
#include "static_string_pool.h"

#define TRAIT_AFFINITY_ROW_TRAIT_MAX 4

//================================================================================================================================
//=> - TraitAffinityRowStruct -
//================================================================================================================================

struct TraitAffinityRowStruct {
    u16 m_base;
    u8 m_trait_n;
    u16 m_traits[TRAIT_AFFINITY_ROW_TRAIT_MAX];
};

//================================================================================================================================
//=> - TraitAffinityMap -
//================================================================================================================================
//
//  Token-keyed trait-affinity rows loaded from game_config.trait_affinity.
//
//================================================================================================================================

class TraitAffinityMap {
public:
    TraitAffinityMap () = default;
    ~TraitAffinityMap ();
    void set_rows (TraitAffinityRowStruct* rows, u16 row_n, StaticStringPool pool);
    void adopt (TraitAffinityMap& src);
    void take_ownership ();
    void release_rows ();
    u16 get_row_count () const;
    u16 name_to_idx (cstr token) const;
    cstr get_token_name (u16 idx) const;
    const TraitAffinityRowStruct& get_row (u16 idx) const;

private:
    TraitAffinityMap (const TraitAffinityMap& o) = delete;
    TraitAffinityMap (TraitAffinityMap&& o) = delete;
    TraitAffinityMap& operator= (const TraitAffinityMap& o) = delete;
    TraitAffinityMap& operator= (TraitAffinityMap&& o) = delete;

    TraitAffinityRowStruct* m_rows = nullptr;
    u16 m_row_n = 0;
    bool m_owns_rows = false;
    StaticStringPool m_pool;
};

#endif // TRAIT_AFFINITY_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
