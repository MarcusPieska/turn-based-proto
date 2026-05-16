//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "map_composer.h"

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    const char* outpath = "terr_lim_shift_sweep.temp";
    if (argc >= 2 && argv[1][0] != '\0') {
        outpath = argv[1];
    }
    const size_t nl = std::strlen(outpath);
    if (nl < 6 || std::strcmp(outpath + nl - 5, ".temp") != 0) {
        std::fprintf(stderr, "output path must end with .temp\n");
        return 1;
    }
    FILE* fp = std::fopen(outpath, "w");
    if (fp == nullptr) {
        return 2;
    }
    std::fprintf(fp, "arg ocean sea coastal plains hills mountains\n");
    for (int i = 0; i <= 1000; ++i) {
        const f64 arg = static_cast<f64>(i) * 0.001;
        ContinentMakerPnParams p;
        if (MapComposer::shift_land_by_coastal_limit(p, arg)) {
            std::fprintf(
                fp,
                "%.17g %.17g %.17g %.17g %.17g %.17g %.17g\n",
                arg,
                p.m_terr_lim_ocean,
                p.m_terr_lim_sea,
                p.m_terr_lim_coastal,
                p.m_terr_lim_plains,
                p.m_terr_lim_hills,
                p.m_terr_lim_mountains);
        } else {
            const f64 nanv = std::nan("");
            std::fprintf(
                fp,
                "%.17g %.17g %.17g %.17g %.17g %.17g %.17g\n",
                arg,
                nanv,
                nanv,
                nanv,
                nanv,
                nanv,
                nanv);
        }
    }
    std::fclose(fp);
    char cmd[1024];
    std::snprintf(cmd, sizeof cmd, "python3 plot_map_composer_landshift.py \"%s\"", outpath);
    const int pr = std::system(cmd);
    if (pr != 0) {
        return 3;
    }
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
