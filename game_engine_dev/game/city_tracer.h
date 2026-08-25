//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_TRACER_H
#define CITY_TRACER_H

#include "game_primitives.h"

//================================================================================================================================
//=> - CityTracer -
//================================================================================================================================
//
//  Optional per-city turn ledger. When CITY_TRACER_ENABLE is set, call sites fill one scratch line per city and
//  LOG_CITY_COMMIT appends it to an in-memory turn buffer; LOG_CITY_FLUSH writes the buffer once. Without the macro,
//  all LOG_CITY_* macros and CityTracer bodies are NOPs.
//  Line format: city:owner:turn:pop:san:food:prod:com:cult:sci:rel:name:start:finish
//
//================================================================================================================================

class CityTracer {
public:
#ifdef CITY_TRACER_ENABLE
    static void setup (cstr path);
    static void clear ();
    static void begin_city (u16 city_idx, u16 owner, u32 turn);
    static void log_pop (u16 pop);
    static void log_san (i16 san);
    static void log_food (u16 yield);
    static void log_prod (u16 yield);
    static void log_com (u16 yield);
    static void log_cult (u16 yield);
    static void log_sci (u16 yield);
    static void log_rel (u16 yield);
    static void log_build (cstr name, u8 start, u8 finish);
    static void commit ();
    static void flush_turn ();
#else
    static void setup (cstr) {}
    static void clear () {}
    static void begin_city (u16, u16, u32) {}
    static void log_pop (u16) {}
    static void log_san (i16) {}
    static void log_food (u16) {}
    static void log_prod (u16) {}
    static void log_com (u16) {}
    static void log_cult (u16) {}
    static void log_sci (u16) {}
    static void log_rel (u16) {}
    static void log_build (cstr, u8, u8) {}
    static void commit () {}
    static void flush_turn () {}
#endif

private:
    CityTracer () = delete;
};

//================================================================================================================================
//=> - Macros -
//================================================================================================================================

#ifdef CITY_TRACER_ENABLE
#define LOG_CITY_SETUP(args) CityTracer::setup args
#define LOG_CITY_CLEAR(args) CityTracer::clear args
#define LOG_CITY_BEGIN(args) CityTracer::begin_city args
#define LOG_CITY_POP(args) CityTracer::log_pop args
#define LOG_CITY_SAN(args) CityTracer::log_san args
#define LOG_CITY_FOOD(args) CityTracer::log_food args
#define LOG_CITY_PROD(args) CityTracer::log_prod args
#define LOG_CITY_COM(args) CityTracer::log_com args
#define LOG_CITY_CULT(args) CityTracer::log_cult args
#define LOG_CITY_SCI(args) CityTracer::log_sci args
#define LOG_CITY_REL(args) CityTracer::log_rel args
#define LOG_CITY_BUILD(args) CityTracer::log_build args
#define LOG_CITY_COMMIT(args) CityTracer::commit args
#define LOG_CITY_FLUSH(args) CityTracer::flush_turn args
#else
#define LOG_CITY_SETUP(args) ((void)0)
#define LOG_CITY_CLEAR(args) ((void)0)
#define LOG_CITY_BEGIN(args) ((void)0)
#define LOG_CITY_POP(args) ((void)0)
#define LOG_CITY_SAN(args) ((void)0)
#define LOG_CITY_FOOD(args) ((void)0)
#define LOG_CITY_PROD(args) ((void)0)
#define LOG_CITY_COM(args) ((void)0)
#define LOG_CITY_CULT(args) ((void)0)
#define LOG_CITY_SCI(args) ((void)0)
#define LOG_CITY_REL(args) ((void)0)
#define LOG_CITY_BUILD(args) ((void)0)
#define LOG_CITY_COMMIT(args) ((void)0)
#define LOG_CITY_FLUSH(args) ((void)0)
#endif

#endif // CITY_TRACER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
