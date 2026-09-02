//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORKER_CITY_JOBS_H
#define WORKER_CITY_JOBS_H

#include "game_primitives.h"

class GameArraySimple;

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

#define WCJ_JOBS_N 10
#define WCJ_R_MAX 20u

//================================================================================================================================
//=> - WorkerCityJobs -
//================================================================================================================================
//
//  Per planned-city site job coords (FORT / MTN_PASS intents) for early worker AI. Sites are keyed by planned
//  city tile so any seat that founds there can use the list. Built by growing CircularTileAreas rings r=0..WCJ_R_MAX;
//  same-r conflicts go to the earlier site in scan order. Cap WCJ_JOBS_N per site for now; revisit heap-grown
//  per-site lists when mid-game frees this memory for warfare AI. Resources stay off this table.
//
//================================================================================================================================

class WorkerCityJobs {
public:
    WorkerCityJobs ();
    ~WorkerCityJobs ();

    bool build (const GameArraySimple& map);
    void clr ();
    bool ok () const;
    u32 site_n () const;
    bool find_site (u16 cx, u16 cy, u32* oix) const;
    u16 job_n (u32 site) const;
    bool job_at (u32 site, u16 slot, u16* ox, u16* oy) const;

private:
    struct Site {
        u16 m_x; // Planned city x
        u16 m_y; // Planned city y
        u16 m_n; // Filled job count (<= WCJ_JOBS_N)
        u16 m_jx[WCJ_JOBS_N]; // Job tile x
        u16 m_jy[WCJ_JOBS_N]; // Job tile y 
    };

    WorkerCityJobs (const WorkerCityJobs& other) = delete;
    WorkerCityJobs& operator= (const WorkerCityJobs& other) = delete;
    WorkerCityJobs (WorkerCityJobs&& other) = delete;
    WorkerCityJobs& operator= (WorkerCityJobs&& other) = delete;

    Site* m_sites; // Heap site table; null when empty / cleared
    u32 m_sn; // Site count
    bool m_ok; // True after a successful build (including zero sites)
};

#endif // WORKER_CITY_JOBS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
