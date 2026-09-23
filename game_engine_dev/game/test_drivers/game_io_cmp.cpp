//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "game_io_cmp.h"

#include <cstdio>
#include <cstring>

#include "bit_array.h"
#include "city.h"
#include "city_array.h"
#include "game_state.h"
#include "game_array_simple.h"
#include "general_bit_bank.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u8 k_bits_per_byte = 8u;
static const u32 k_bank_batches_per_page = 256u;

static u32 bank_page_byte_n (u16 batch_size) {
    const u32 page_bit_n = static_cast<u32>(batch_size) * k_bank_batches_per_page;
    return (page_bit_n + static_cast<u32>(k_bits_per_byte - 1u)) / static_cast<u32>(k_bits_per_byte);
}

bool GameIoCmp::bank (const GeneralBitBank* a, const GeneralBitBank* b) {
    if (a == nullptr && b == nullptr) {
        return true;
    }
    if (a == nullptr || b == nullptr) {
        return false;
    }
    if (a->m_batch_size != b->m_batch_size
        || a->m_claimed_batch_count != b->m_claimed_batch_count
        || a->m_allocated_page_count != b->m_allocated_page_count) {
        std::printf("GameIoCmp bank meta mismatch\n");
        return false;
    }
    const u32 page_bytes = bank_page_byte_n(a->m_batch_size);
    for (u8 p = 0; p < a->m_allocated_page_count; ++p) {
        if (a->m_pages[p] == nullptr || b->m_pages[p] == nullptr) {
            return false;
        }
        if (std::memcmp(a->m_pages[p], b->m_pages[p], page_bytes) != 0) {
            std::printf("GameIoCmp bank page %u mismatch\n", static_cast<u32>(p));
            return false;
        }
    }
    return true;
}

bool GameIoCmp::bit_cl (const BitArrayCL* a, const BitArrayCL* b) {
    if (a == nullptr && b == nullptr) {
        return true;
    }
    if (a == nullptr || b == nullptr) {
        return false;
    }
    if (a->get_count() != b->get_count()) {
        return false;
    }
    const u32 n = a->get_count();
    for (u32 i = 0; i < n; ++i) {
        if (a->get_bit(i) != b->get_bit(i)) {
            return false;
        }
    }
    return true;
}

bool GameIoCmp::unit (const UnitAddStruct& a, const UnitAddStruct& b) {
    return std::memcmp(&a, &b, sizeof(UnitAddStruct)) == 0;
}

//================================================================================================================================
//=> - GameIoCmp -
//================================================================================================================================

bool GameIoCmp::map (const GameArraySimple& a, const GameArraySimple& b) {
    if (a.width() != b.width() || a.height() != b.height()) {
        std::printf("GameIoCmp map size %ux%u vs %ux%u\n",
            a.width(), a.height(), b.width(), b.height());
        return false;
    }
    for (u16 y = 0; y < a.height(); ++y) {
        for (u16 x = 0; x < a.width(); ++x) {
            const GameTileSimple* ta = a.tile(x, y);
            const GameTileSimple* tb = b.tile(x, y);
            if (ta == nullptr || tb == nullptr
                || std::memcmp(ta, tb, sizeof(GameTileSimple)) != 0) {
                std::printf("GameIoCmp map tile (%u,%u) mismatch\n",
                    static_cast<u32>(x), static_cast<u32>(y));
                return false;
            }
        }
    }
    return true;
}

bool GameIoCmp::units (const UnitAddVector& a, const UnitAddVector& b) {
    if (a.get_head_unit_add_idx() != b.get_head_unit_add_idx()) {
        std::printf("GameIoCmp units head %u vs %u\n",
            static_cast<u32>(a.get_head_unit_add_idx()), static_cast<u32>(b.get_head_unit_add_idx()));
        return false;
    }
    const u16 head = a.get_head_unit_add_idx();
    for (u16 i = 0; i < head; ++i) {
        const UnitAddStruct* ua = a.get_unit_add(UnitAddKey::from_raw(i));
        const UnitAddStruct* ub = b.get_unit_add(UnitAddKey::from_raw(i));
        if ((ua == nullptr) != (ub == nullptr)) {
            std::printf("GameIoCmp units key %u presence mismatch\n", static_cast<u32>(i));
            return false;
        }
        if (ua != nullptr && !unit(*ua, *ub)) {
            std::printf("GameIoCmp units key %u data mismatch\n", static_cast<u32>(i));
            return false;
        }
    }
    return true;
}

bool GameIoCmp::cities (const CityArray& a, const CityArray& b) {
    if (a.get_city_count() != b.get_city_count()) {
        std::printf("GameIoCmp cities count %u vs %u\n",
            static_cast<u32>(a.get_city_count()), static_cast<u32>(b.get_city_count()));
        return false;
    }
    const u16 cn = a.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* ca = a.get_city(i);
        const City* cb = b.get_city(i);
        if (ca == nullptr || cb == nullptr) {
            return false;
        }
        if (ca->get_owner() != cb->get_owner()
            || ca->get_x() != cb->get_x()
            || ca->get_y() != cb->get_y()
            || ca->get_current_population() != cb->get_current_population()
            || ca->get_current_food_store() != cb->get_current_food_store()
            || ca->get_current_production_store() != cb->get_current_production_store()
            || ca->get_current_culture() != cb->get_current_culture()) {
            std::printf("GameIoCmp city %u fields mismatch\n", static_cast<u32>(i));
            return false;
        }
    }
    if (!bank(a.get_flag_bank(), b.get_flag_bank())
        || !bank(a.get_res_bank(), b.get_res_bank())
        || !bank(a.get_bld_bank(), b.get_bld_bank())) {
        return false;
    }
    return true;
}

bool GameIoCmp::players (const PlayerState* a, u16 a_n, const PlayerState* b, u16 b_n) {
    if (a_n != b_n || a == nullptr || b == nullptr) {
        std::printf("GameIoCmp players n %u vs %u\n",
            static_cast<u32>(a_n), static_cast<u32>(b_n));
        return false;
    }
    for (u16 p = 0; p < a_n; ++p) {
        const PlayerState& pa = a[p];
        const PlayerState& pb = b[p];
        if (pa.m_civ_index != pb.m_civ_index
            || pa.m_research_spending_perc != pb.m_research_spending_perc
            || pa.m_current_research_target_idx != pb.m_current_research_target_idx
            || pa.m_commerce != pb.m_commerce
            || pa.m_research != pb.m_research
            || pa.m_commerce_from_turn != pb.m_commerce_from_turn) {
            std::printf("GameIoCmp player %u fields mismatch\n", static_cast<u32>(p));
            return false;
        }
        if (!bit_cl(pa.m_techs_researched, pb.m_techs_researched)) {
            std::printf("GameIoCmp player %u tech bits mismatch\n", static_cast<u32>(p));
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
