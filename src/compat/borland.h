// borland.h - the parts of the Borland C++ 3.x runtime Kondor relied on, for modern C++.
#pragma once

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef unsigned char uchar;

// Borland's F_FTOL@: float -> integer, truncating toward zero (the FPU was set to chop).
inline int ftol(double v)
{
    return (int)(long long)v;
}

// Borland's rand(): 32-bit LCG, multiplier 0x015A4E35, returns bits 16..30.
// Reproduced exactly so random sequences match the original for a given seed.
extern uint32_t borland_seed;
inline int bc_rand()
{
    borland_seed = borland_seed * 0x015A4E35u + 1u;
    return (int)((borland_seed >> 16) & 0x7FFF);
}
inline void bc_srand(unsigned seed)
{
    borland_seed = (uint16_t)seed;   // srand() set the low word and cleared the high word
}
// <stdlib.h> random(num) of Borland C++ (inline function): rand() * num / 32768.
inline int random(int num)
{
    return (int)(((long)bc_rand() * num) / 0x8000);
}
void randomize();   // srand(time()) - or KONDOR_SEED from the environment

char *itoa(int value, char *string, int radix);
char *ultoa(unsigned long value, char *string, int radix);
char *strupr(char *s);

void delay(unsigned ms);
void sound(unsigned hz);
void nosound();

// Opens a game data file. DOS file names are case-insensitive, so the data directory
// ($KONDOR_DATA, or the current directory) is searched case-insensitively.
FILE *fopen_ci(const char *name, const char *mode);
const char *last_file_name();   // for error messages

// fread() that zero-fills whatever a short read leaves, so a truncated data file can't
// leave a buffer uninitialised. (The original never checked fread's result.)
size_t fread_z(void *ptr, size_t size, size_t n, FILE *f);

int read_le16(FILE *f);                 // signed 16-bit little-endian word
void write_le16(FILE *f, int value);
