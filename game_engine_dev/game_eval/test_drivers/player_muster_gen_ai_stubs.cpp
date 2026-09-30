//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_ai_helpers.h"

//================================================================================================================================
//=> - GenAiHelpers stubs (eval-only; avoid full AI helper link graph) -
//================================================================================================================================

GenAiHelpers::GenAiHelpers ()
    : m_map(nullptr),
      m_ord(nullptr),
      m_jobs(nullptr),
      m_roads(nullptr),
      m_ok(false) {
}

GenAiHelpers::~GenAiHelpers () {
}

bool GenAiHelpers::begin (GameArraySimple&) {
    return false;
}

bool GenAiHelpers::build (const SpgCoordPair*, u32, GenAiHelpersRslt*) {
    return false;
}

void GenAiHelpers::clr () {
}

bool GenAiHelpers::ok () const {
    return false;
}

const GenSettlementOrder& GenAiHelpers::order () const {
    return *m_ord;
}

GenSettlementOrder& GenAiHelpers::order () {
    return *m_ord;
}

const WorkerCityJobs& GenAiHelpers::jobs () const {
    return *m_jobs;
}

WorkerCityJobs& GenAiHelpers::jobs () {
    return *m_jobs;
}

const GenRoadNetwork& GenAiHelpers::roads () const {
    return *m_roads;
}

GenRoadNetwork& GenAiHelpers::roads () {
    return *m_roads;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
