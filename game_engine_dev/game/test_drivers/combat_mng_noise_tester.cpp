//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstdio>

#include "combat_mng.h"

//================================================================================================================================
//=> - Config -
//================================================================================================================================

static const u32 k_n = 100000u;
static const u32 k_bins = 21u;

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    CombatMng::set_dials(85u, 0u, 1u);

    const double sig = CombatMng::noise_sig();
    const double cap = CombatMng::noise_cap();
    std::printf("mk02 noise dials: sig=%.3f pp  cap=+/-%.3f pp\n", sig, cap);

    double mean = 0.0;
    double sd = 0.0;
    double us = 0.0;
    CombatMng::noise_run(k_n, &mean, &sd, &us);
    std::printf("noise_run n=%u  mean=%.4f pp  sd=%.4f pp  total=%.1f us  per=%.4f us\n",
        k_n, mean, sd, us, us / static_cast<double>(k_n));

    CombatMng::set_dials(85u, 0u, 1u);
    u32 hist[k_bins] = {};
    u32 clipped = 0u;
    u32 n_max = static_cast<u32>(std::ceil(cap));
    if (n_max < 1u) {
        n_max = 1u;
    }
    u32* flip = new u32[n_max + 1u]();
    const double lo = -cap;
    const double hi = cap;
    const double span = hi - lo;
    for (u32 i = 0u; i < k_n; ++i) {
        const double v = CombatMng::noise_draw();
        if (v >= cap || v <= -cap) {
            clipped++;
        }
        double t = (v - lo) / span;
        if (t < 0.0) {
            t = 0.0;
        } else if (t >= 1.0) {
            t = 0.999999;
        }
        hist[static_cast<u32>(t * static_cast<double>(k_bins))]++;
        for (u32 n = 1u; n <= n_max; ++n) {
            if (v < -static_cast<double>(n)) {
                flip[n]++;
            }
        }
    }

    std::printf("histogram (%u bins over [%.1f, %.1f] pp), at-cap count=%u (%.2f%%)\n",
        k_bins, lo, hi, clipped, 100.0 * static_cast<double>(clipped) / static_cast<double>(k_n));
    u32 peak = 1u;
    for (u32 b = 0u; b < k_bins; ++b) {
        if (hist[b] > peak) {
            peak = hist[b];
        }
    }
    for (u32 b = 0u; b < k_bins; ++b) {
        const double a = lo + span * static_cast<double>(b) / static_cast<double>(k_bins);
        const double c = lo + span * static_cast<double>(b + 1u) / static_cast<double>(k_bins);
        const double pct = 100.0 * static_cast<double>(hist[b]) / static_cast<double>(k_n);
        const u32 bar_n = (hist[b] * 40u) / peak;
        std::printf("[%6.1f,%6.1f) %6.2f%% |", a, c, pct);
        for (u32 k = 0u; k < bar_n; ++k) {
            std::putchar('#');
        }
        std::putchar('\n');
    }

    std::printf("\nraw win chance -> P(lose) = P(noise < -(win-50) pp), N=1..%u (ceil cap)\n", n_max);
    for (u32 n = 1u; n <= n_max; ++n) {
        const double pct = 100.0 * static_cast<double>(flip[n]) / static_cast<double>(k_n);
        std::printf("  %2u%% win chance -> lose %6.2f%%\n", n + 50u, pct);
    }
    delete[] flip;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
