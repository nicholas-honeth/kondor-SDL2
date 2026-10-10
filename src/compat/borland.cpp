// borland.cpp - Borland runtime replacements (see borland.h).
#include "borland.h"
#include "../platform/platform.h"

#include <dirent.h>
#include <string>

uint32_t borland_seed = 1;

void randomize()
{
    if (const char *s = std::getenv("KONDOR_SEED")) {
        bc_srand((unsigned)std::strtoul(s, nullptr, 0));
        return;
    }
    bc_srand((unsigned)std::time(nullptr));
}

static char *utoa_radix(unsigned long value, char *string, int radix, bool negative)
{
    char tmp[40];
    int n = 0;
    do {
        int d = (int)(value % radix);
        tmp[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        value /= radix;
    } while (value);
    char *p = string;
    if (negative) *p++ = '-';
    while (n) *p++ = tmp[--n];
    *p = 0;
    return string;
}

char *itoa(int value, char *string, int radix)
{
    if (radix == 10 && value < 0) return utoa_radix(0ul - (unsigned long)(long)value, string, radix, true);
    return utoa_radix((unsigned)value, string, radix, false);
}

char *ultoa(unsigned long value, char *string, int radix)
{
    return utoa_radix(value, string, radix, false);
}

char *strupr(char *s)
{
    for (char *p = s; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
    return s;
}

void delay(unsigned ms)
{
    plat::delay_ms(ms);
}

void sound(unsigned hz)
{
    plat::speaker_on(hz);
}

void nosound()
{
    plat::speaker_off();
}

static std::string g_last_file;

const char *last_file_name()
{
    return g_last_file.c_str();
}

FILE *fopen_ci(const char *name, const char *mode)
{
    std::string dir = ".";
    if (const char *d = std::getenv("KONDOR_DATA")) dir = d;
    g_last_file = name;
    std::string path = dir + "/" + name;
    if (FILE *f = std::fopen(path.c_str(), mode)) return f;
    if (DIR *dp = opendir(dir.c_str())) {
        while (dirent *de = readdir(dp)) {
            if (strcasecmp(de->d_name, name) == 0) {
                path = dir + "/" + de->d_name;
                closedir(dp);
                return std::fopen(path.c_str(), mode);
            }
        }
        closedir(dp);
    }
    // writing a file that does not exist yet (KONDOR.CFG / KONDOR.HSC): keep DOS-style upper case
    if (mode[0] == 'w') {
        std::string upper = name;
        for (char &c : upper) c = (char)std::toupper((unsigned char)c);
        return std::fopen((dir + "/" + upper).c_str(), mode);
    }
    return nullptr;
}

size_t fread_z(void *ptr, size_t size, size_t n, FILE *f)
{
    size_t total = size * n, got = std::fread(ptr, 1, total, f);
    if (got < total) std::memset((char *)ptr + got, 0, total - got);
    return size ? got / size : 0;
}

int read_le16(FILE *f)
{
    unsigned char b[2] = {0, 0};
    if (std::fread(b, 1, 2, f) != 2) return 0;
    return (int16_t)(b[0] | b[1] << 8);
}

void write_le16(FILE *f, int value)
{
    unsigned char b[2] = {(unsigned char)value, (unsigned char)(value >> 8)};
    std::fwrite(b, 1, 2, f);
}
