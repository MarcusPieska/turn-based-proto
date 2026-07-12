//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_RPRINT_H
#define P1_RPRINT_H

#include "game_primitives.h"

#include <cstdio>

//================================================================================================================================
//=> - P1_RPrint -
//================================================================================================================================
//
//  Colored tester output; every line is prefixed with "*** " and fields are colon-separated.
//  Capitalization: info is the stage class name (P1_Gen_ContOutlines) or a PascalCase label (Map, Output);
//  param is PascalCase (Continents, W, H); rprint_info text is a capitalized phrase or label: detail.
//  rprint_result_* is for failed asserts only (red/green pass-fail); use rprint_state_* for normal metrics.
//
//================================================================================================================================

class P1_RPrint {
public:
    static void rprint_info (cstr info);
    static void rprint_result_u16 (cstr info, cstr param, u16 result, bool condition);
    static void rprint_state (cstr info, cstr param, u16 result);
    static void rprint_result_u32 (cstr info, cstr param, u32 result, bool condition);
    static void rprint_state_u32 (cstr info, cstr param, u32 result);

private:
    P1_RPrint () = delete;

    static cstr pre ();
    static cstr nm (cstr s);
    static void kv_u16 (cstr param, u16 result, cstr color);
    static void kv_u32 (cstr param, u32 result, cstr color);
};

inline cstr P1_RPrint::pre () {
    return "*** ";
}

inline cstr P1_RPrint::nm (cstr s) {
    return s != nullptr ? s : "";
}

inline void P1_RPrint::kv_u16 (cstr param, u16 result, cstr color) {
    std::printf("%s%s: %u\033[0m", color, nm(param), static_cast<u32>(result));
}

inline void P1_RPrint::kv_u32 (cstr param, u32 result, cstr color) {
    std::printf("%s%s: %u\033[0m", color, nm(param), result);
}

inline void P1_RPrint::rprint_info (cstr info) {
    std::printf("%s%s\n", pre(), nm(info));
}

inline void P1_RPrint::rprint_result_u16 (cstr info, cstr param, u16 result, bool condition) {
    const cstr color = condition ? "\033[92m" : "\033[91m";
    std::printf("%s%s: ", pre(), nm(info));
    kv_u16(param, result, color);
    std::printf("\n");
}

inline void P1_RPrint::rprint_state (cstr info, cstr param, u16 result) {
    std::printf("%s%s: ", pre(), nm(info));
    kv_u16(param, result, "\033[94m");
    std::printf("\n");
}

inline void P1_RPrint::rprint_result_u32 (cstr info, cstr param, u32 result, bool condition) {
    const cstr color = condition ? "\033[92m" : "\033[91m";
    std::printf("%s%s: ", pre(), nm(info));
    kv_u32(param, result, color);
    std::printf("\n");
}

inline void P1_RPrint::rprint_state_u32 (cstr info, cstr param, u32 result) {
    std::printf("%s%s: ", pre(), nm(info));
    kv_u32(param, result, "\033[94m");
    std::printf("\n");
}

#endif // P1_RPRINT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
