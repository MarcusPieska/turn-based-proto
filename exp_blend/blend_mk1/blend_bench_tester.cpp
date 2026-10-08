//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>

#include "blend_cfg.h"
#include "blend_core.h"
#include "map_loader.h"
#include "map_terrain_data.h"

//================================================================================================================================
//=> - Config -
//================================================================================================================================

static const char *G_TERR = "/home/w/Projects/simple-map-gen/p1-seed-43/terrain.ppm";
static const int WIN = 20;
static const int STEP = 1;
static const int HIST_BINS = 40;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void print_hist (const std::vector<double> &us) {
    if (us.empty ()) return;
    double lo = *std::min_element (us.begin (), us.end ());
    double hi = *std::max_element (us.begin (), us.end ());
    if (hi <= lo) hi = lo + 1.0;
    std::vector<int> bins (HIST_BINS, 0);
    for (double v : us) {
        int b = (int)((v - lo) / (hi - lo) * (HIST_BINS - 1));
        if (b < 0) b = 0;
        if (b >= HIST_BINS) b = HIST_BINS - 1;
        bins[b]++;
    }
    int peak = 1;
    for (int n : bins) if (n > peak) peak = n;
    std::printf ("\ntime distribution (us), %d bins [%g .. %g]:\n", HIST_BINS, lo, hi);
    for (int i = 0; i < HIST_BINS; i++) {
        double a = lo + (hi - lo) * i / HIST_BINS;
        double b = lo + (hi - lo) * (i + 1) / HIST_BINS;
        int bar = (bins[i] * 50) / peak;
        std::printf ("%8.1f-%8.1f | ", a, b);
        for (int k = 0; k < bar; k++) std::putchar ('#');
        std::printf (" (%d)\n", bins[i]);
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char **argv) {
    const char *terr_path = G_TERR;
    int win = WIN;
    int step = STEP;
    if (argc > 1) terr_path = argv[1];
    if (argc > 2) win = std::atoi (argv[2]);
    if (argc > 3) step = std::atoi (argv[3]);
    if (win <= 0 || step <= 0) {
        std::fprintf (stderr, "usage: %s [terrain.ppm] [win=20] [step=1]\n", argv[0]);
        return 1;
    }

    MapTerrainData terr;
    if (!MapLoader::load_terrain_ppm (terr_path, terr)) {
        std::fprintf (stderr, "failed to load terrain: %s\n", terr_path);
        return 1;
    }
    const int full_w = (int)terr.width ();
    const int full_h = (int)terr.height ();
    const uint8_t *full = terr.data ();
    std::printf ("loaded %s (%dx%d), window %dx%d step %d\n", terr_path, full_w, full_h, win, win, step);
    if (full_w < win || full_h < win) {
        std::fprintf (stderr, "map smaller than window\n");
        return 1;
    }

    const int n_r = (full_h - win) / step + 1;
    const int n_c = (full_w - win) / step + 1;
    const long long n_win = (long long)n_r * (long long)n_c;
    std::printf ("windows: %d x %d = %lld\n", n_r, n_c, n_win);

    BlendMap map;
    EdgeThread th;
    std::vector<double> samples;
    samples.reserve ((size_t)std::min (n_win, (long long)2000000));

    double sum = 0.0;
    double t_min = 1e300;
    double t_max = 0.0;
    long long done = 0;

    for (int r0 = 0; r0 + win <= full_h; r0 += step) {
        for (int c0 = 0; c0 + win <= full_w; c0 += step) {
            if (!map.from_win (full, full_w, full_h, r0, c0, win)) {
                std::fprintf (stderr, "from_win failed at %d,%d\n", r0, c0);
                return 1;
            }
            auto t0 = std::chrono::steady_clock::now ();
            th.run (map);
            auto t1 = std::chrono::steady_clock::now ();
            double us = std::chrono::duration<double, std::micro> (t1 - t0).count ();
            samples.push_back (us);
            sum += us;
            if (us < t_min) t_min = us;
            if (us > t_max) t_max = us;
            done++;
        }
        if ((r0 / step) % 50 == 0) {
            std::printf ("  progress %lld / %lld\n", done, n_win);
            std::fflush (stdout);
        }
    }

    double avg = sum / (double)done;
    std::printf ("\njunctions+threading only (no flood/draw)\n");
    std::printf ("samples: %lld\n", done);
    std::printf ("min: %.3f us\n", t_min);
    std::printf ("max: %.3f us\n", t_max);
    std::printf ("avg: %.3f us\n", avg);
    print_hist (samples);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
