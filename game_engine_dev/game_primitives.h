//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_PRIMITIVES_H
#define GAME_PRIMITIVES_H

#include <cstdint>

typedef const char* cstr;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef float f32;
typedef double f64;

#define U16_KEY_NULL 0xFFFF
#define U16_KEY_INVALID 0xFFFF - 1
#define U8_KEY_NULL 0xFF

#define MAX_EFFECTS_PER_ENTITY 4
typedef struct EffectIndices {
    u16 indices[MAX_EFFECTS_PER_ENTITY];
} EffectIndices;

// ProfileTime toggles (misc/profile_time.h); uncomment to enable.
//#define PTIME_ENABLE
//#define PTIME_TRACE_ENABLE

// ProfileTimeOpt toggles (misc/profile_time_opt.h); uncomment to enable.
// #define PTO_ENABLE

// AssertLog toggles (misc/assert_log.h); uncomment to enable.
//#define GAME_ASSERT_ENABLE

#endif // GAME_PRIMITIVES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
