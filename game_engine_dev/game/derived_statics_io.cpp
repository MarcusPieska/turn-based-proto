//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "derived_statics_io.h"

#include <cstdio>

//================================================================================================================================
//=> - DerivedStaticsIO -
//================================================================================================================================

bool DerivedStaticsIO::write (cstr path, const u16* paint, u16 w, u16 h, u16 sec_n) {
    if (path == nullptr || paint == nullptr || w == 0u || h == 0u || sec_n == 0u) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    const u32 magic = k_magic;
    const u32 ver = k_ver;
    if (std::fwrite(&magic, sizeof(magic), 1, fp) != 1
        || std::fwrite(&ver, sizeof(ver), 1, fp) != 1
        || std::fwrite(&w, sizeof(w), 1, fp) != 1
        || std::fwrite(&h, sizeof(h), 1, fp) != 1
        || std::fwrite(&sec_n, sizeof(sec_n), 1, fp) != 1
        || std::fwrite(paint, sizeof(u16), n, fp) != n) {
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    return true;
}

bool DerivedStaticsIO::read (cstr path, u16* w, u16* h, u16* sec_n, u16** paint) {
    if (path == nullptr || w == nullptr || h == nullptr || sec_n == nullptr || paint == nullptr) {
        return false;
    }
    *w = 0;
    *h = 0;
    *sec_n = 0;
    *paint = nullptr;
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 rw = 0;
    u16 rh = 0;
    u16 rn = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || magic != k_magic
        || ver != k_ver
        || std::fread(&rw, sizeof(rw), 1, fp) != 1
        || std::fread(&rh, sizeof(rh), 1, fp) != 1
        || std::fread(&rn, sizeof(rn), 1, fp) != 1
        || rw == 0u
        || rh == 0u
        || rn == 0u) {
        std::fclose(fp);
        return false;
    }
    const u32 n = static_cast<u32>(rw) * static_cast<u32>(rh);
    u16* buf = new u16[n];
    if (buf == nullptr) {
        std::fclose(fp);
        return false;
    }
    if (std::fread(buf, sizeof(u16), n, fp) != n) {
        delete[] buf;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    *w = rw;
    *h = rh;
    *sec_n = rn;
    *paint = buf;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
