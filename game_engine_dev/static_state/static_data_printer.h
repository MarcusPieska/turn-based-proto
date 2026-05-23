//================================================================================================================================
//=> - WARNING -
//================================================================================================================================
//
//  - Template for static_data_printer (filled by generate_static_data_and_test.py)
//  - Do not edit generated output manually.
//
//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef STATIC_DATA_PRINTER_H
#define STATIC_DATA_PRINTER_H

#include "game_primitives.h"

class RuntimeStatics;

//================================================================================================================================
//=> - StaticDataPrintKind enum -
//================================================================================================================================

enum class StaticDataPrintKind : u8 {
    BUILDING = 0,
    CITY_FLAG = 1,
    CIV = 2,
    CIV_TRAIT = 3,
    RESOURCE = 4,
    SMALL_WONDER = 5,
    TECH = 6,
    UNIT = 7,
    UNIT_ACTION = 8,
    UNIT_TYPE = 9,
    WONDER = 10
};

//================================================================================================================================
//=> - StaticDataPrinter class -
//================================================================================================================================

class StaticDataPrinter {
public:
    StaticDataPrinter () = delete;
    StaticDataPrinter (const StaticDataPrinter& other) = delete;
    StaticDataPrinter (StaticDataPrinter&& other) = delete;

    static void print_selected_items (
        const RuntimeStatics& statics,
        StaticDataPrintKind kind,
        u16 idx,
        i32 print_lvl,
        i32 max_lvl);

private:
    static void print_indent (i32 print_lvl);
    static bool can_recurse (i32 print_lvl, i32 max_lvl);
    static void print_u16 (const char* label, u16 value, i32 print_lvl);
    static void print_u32 (const char* label, u32 value, i32 print_lvl);
};

#endif // STATIC_DATA_PRINTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
