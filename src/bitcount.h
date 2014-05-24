/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2014 Marco Costalba, Joona Kiiski, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.


  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BITCOUNT_H_INCLUDED
#define BITCOUNT_H_INCLUDED

#include <cassert>
#include "types.h"

enum BitCountType {
  CNT_64,
  CNT_64_MAX15,
  CNT_32,
  CNT_32_MAX15,
  CNT_HW_POPCNT
#ifdef PA_GTB
  ,
  CNT_PA_GTB_FULL,
  CNT_PA_GTB_MAX15
#endif
};

/// Determine at compile time the best popcount<> specialization according to
/// whether the platform is 32 or 64 bit, the maximum number of non-zero
/// bits to count and if the hardware popcnt instruction is available.
#ifdef PA_GTB
const BitCountType Full  = CNT_PA_GTB_FULL;
const BitCountType Max15  = CNT_PA_GTB_MAX15;
#else
const BitCountType Full  = HasPopCnt ? CNT_HW_POPCNT : Is64Bit ? CNT_64 : CNT_32;
const BitCountType Max15 = HasPopCnt ? CNT_HW_POPCNT : Is64Bit ? CNT_64_MAX15 : CNT_32_MAX15;
#endif


/// popcount() counts the number of non-zero bits in a bitboard
template<BitCountType> inline int popcount(Bitboard);

template<>
inline int popcount<CNT_64>(Bitboard b) {
  b -=  (b >> 1) & 0x5555555555555555ULL;
  b  = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
  b  = ((b >> 4) + b) & 0x0F0F0F0F0F0F0F0FULL;
  return (b * 0x0101010101010101ULL) >> 56;
}

template<>
inline int popcount<CNT_64_MAX15>(Bitboard b) {
  b -=  (b >> 1) & 0x5555555555555555ULL;
  b  = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
  return (b * 0x1111111111111111ULL) >> 60;
}

template<>
inline int popcount<CNT_32>(Bitboard b) {
  unsigned w = unsigned(b >> 32), v = unsigned(b);
  v -=  (v >> 1) & 0x55555555; // 0-2 in 2 bits
  w -=  (w >> 1) & 0x55555555;
  v  = ((v >> 2) & 0x33333333) + (v & 0x33333333); // 0-4 in 4 bits
  w  = ((w >> 2) & 0x33333333) + (w & 0x33333333);
  v  = ((v >> 4) + v + (w >> 4) + w) & 0x0F0F0F0F;
  return (v * 0x01010101) >> 24;
}

template<>
inline int popcount<CNT_32_MAX15>(Bitboard b) {
  unsigned w = unsigned(b >> 32), v = unsigned(b);
  v -=  (v >> 1) & 0x55555555; // 0-2 in 2 bits
  w -=  (w >> 1) & 0x55555555;
  v  = ((v >> 2) & 0x33333333) + (v & 0x33333333); // 0-4 in 4 bits
  w  = ((w >> 2) & 0x33333333) + (w & 0x33333333);
  return ((v + w) * 0x11111111) >> 28;
}

template<>
inline int popcount<CNT_HW_POPCNT>(Bitboard b) {

#ifndef USE_POPCNT

  assert(false);
  return b != 0; // Avoid 'b not used' warning

#elif defined(_MSC_VER) && defined(__INTEL_COMPILER)

  return _mm_popcnt_u64(b);

#elif defined(_MSC_VER)

  return (int)__popcnt64(b);

#else

  __asm__("popcnt %1, %0" : "=r" (b) : "r" (b));
  return b;

#endif
}

#ifdef PA_GTB
inline void initPopCnt();
inline int popCntAvailable();

// http://www.gregbugaj.com/?p=348
inline void cpuinfo(int code, int *eax, int *ebx, int *ecx, int *edx) {
  __asm__ volatile(
                   "cpuid;" //  call cpuid instruction
                   :"=a"(*eax),"=b"(*ebx),"=c"(*ecx), "=d"(*edx)// output equal to "movl  %%eax %1"
                   :"a"(code)// input equal to "movl %1, %%eax"
                   //:"%eax","%ebx","%ecx","%edx"// clobbered register
                   );
}

inline int popCntAvailable()
{
  int eax, ebx, ecx, edx;
  
  if (Is64Bit) {
    cpuinfo(1, &eax, &ebx, &ecx, &edx);
    if (ecx & (1 << 23)) {
      return 1;
    }
  }
  return 0;
}

template<>
inline int popcount<CNT_PA_GTB_FULL>(Bitboard b) {
  if (HasPopCnt) {
    return popcount<CNT_HW_POPCNT>(b);
  } else if (Is64Bit) {
    return popcount<CNT_64>(b);
  } else {
    return popcount<CNT_32>(b);
  }
}

template<>
inline int popcount<CNT_PA_GTB_MAX15>(Bitboard b) {
  if (HasPopCnt) {
    return popcount<CNT_HW_POPCNT>(b);
  } else if (Is64Bit) {
    return popcount<CNT_64_MAX15>(b);
  } else {
    return popcount<CNT_32_MAX15>(b);
  }
}

inline void initPopCnt()
{
#ifdef USE_POPCNT
  HasPopCnt = popCntAvailable();
#else
  HasPopCnt = 0;
#endif
}
#endif

#endif // #ifndef BITCOUNT_H_INCLUDED
