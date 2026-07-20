//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_ARRAY_H
#define CITY_ARRAY_H

#include "game_primitives.h"

class City;
class RuntimeStatics;
class GeneralBitBank;
class GameIo;

//================================================================================================================================
//=> - CityArray class -
//================================================================================================================================

class CityArray {
public:
    CityArray();
    ~CityArray();

    bool bind_statics (const RuntimeStatics& st);

    City* get_city(u16 city_idx);
    const City* get_city(u16 city_idx) const;
    u16 get_next_new_city_idx();
    u16 get_page_count() const;
    u16 get_city_count() const;
    City* get_page(u16 page_idx);
    const City* get_page(u16 page_idx) const;
    void set_building_flag (u16 city_idx, u16 bld_idx);

    static const u16 MAX_PAGES = 256;
    static const u16 CITIES_PER_PAGE = 256;

protected:
    GeneralBitBank* get_flag_bank ();
    const GeneralBitBank* get_flag_bank () const;
    GeneralBitBank* get_res_bank ();
    const GeneralBitBank* get_res_bank () const;
    GeneralBitBank* get_bld_bank ();
    const GeneralBitBank* get_bld_bank () const;

private:
    friend class GameIo;

    void clear_banks ();

    City* m_pages[MAX_PAGES];
    u16 m_city_count;
    u16 m_page_count;
    GeneralBitBank* m_flag_bank;
    GeneralBitBank* m_res_bank;
    GeneralBitBank* m_bld_bank;
};

#endif // CITY_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
