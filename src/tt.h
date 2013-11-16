/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2013 Marco Costalba, Joona Kiiski, Tord Romstad

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

#ifndef TT_H_INCLUDED
#define TT_H_INCLUDED

#include "misc.h"
#include "types.h"
#ifdef PA_GTB
#include "phash.h"
#include "ucioption.h"
#endif

/// The TTEntry is the 128 bit transposition table entry, defined as below:
///
/// key: 32 bit
/// move: 16 bit
/// bound type: 8 bit
/// generation: 8 bit
/// value: 16 bit
/// depth: 16 bit
/// static value: 16 bit
/// static margin: 16 bit

struct TTEntry {

  void save(Key k64, uint32_t k, Value v, Bound b, Depth d, Move m, int g, Value ev, bool interested) {

    key32        = (uint32_t)k;
    move16       = (uint16_t)m;
    bound8       = (uint8_t)b;
    generation8  = (uint8_t)g;
    value16      = (int16_t)v;
    depth16      = (int16_t)d;
    evalValue    = (int16_t)ev;
    if (interested && (b & ~BOUND_ROOT) == BOUND_EXACT && m != MOVE_NONE && Options["Use Persistent Hash"]) {
      if (DEPTH_IS_VALID(d, (Options["Persistent Hash Depth"] * ONE_PLY))) {
        phash_store(k64, v, b, d, m, ev);
      }
    }
  }
  void set_generation(uint8_t g) { generation8 = g; }

  uint32_t key() const      { return key32; }
  Depth depth() const       { return (Depth)depth16; }
  Move move() const         { return (Move)move16; }
  Value value() const       { return (Value)value16; }
  Bound bound() const       { return (Bound)(bound8 & ~BOUND_ROOT); }
  int generation() const    { return (int)generation8; }
  Value eval_value() const  { return (Value)evalValue; }

private:
  void phash_store(Key k64, Value v, Bound b, Depth d, Move m, Value ev);

  uint32_t key32;
  uint16_t move16;
  uint8_t bound8, generation8;
  int16_t value16, depth16, evalValue;
};


struct PHEntry {

  void save(Key k64, uint32_t UNUSED(k), Value UNUSED(v), Bound b, Depth d, Move m, int g, Value UNUSED(ev), bool UNUSED(interested)) {

    key64        = (Key)k64;
    move16       = (uint16_t)m;
    bound8       = (uint8_t)b;
    generation8  = (uint8_t)g;
    depth16      = (int16_t)d;
  }

  void set_generation(uint8_t g) { generation8 = g; }

  uint32_t key() const      { return key64 >> 32; }
  Key fullkey() const       { return key64; }
  Depth depth() const       { return (Depth)depth16; }
  Move move() const         { return (Move)move16; }
  Bound bound() const       { return (Bound)(bound8 & ~BOUND_ROOT); }
  Bound fulltype() const    { return (Bound)bound8; }
  int generation() const    { return (int)generation8; }

private:
  Key key64;
  uint16_t move16;
  uint8_t bound8, generation8;
  int16_t depth16;
  uint8_t padding[2];
};
  


/// A TranspositionTable consists of a power of 2 number of clusters and each
/// cluster consists of ClusterSize number of TTEntry. Each non-empty entry
/// contains information of exactly one position. Size of a cluster shall not be
/// bigger than a cache line size. In case it is less, it should be padded to
/// guarantee always aligned accesses.

template<class T>
class TranspositionTable {

  static const unsigned ClusterSize = 4; // A cluster is 64 Bytes

public:
 ~TranspositionTable() { free(mem); }
  void new_search() { ++generation; }

  const T* probe(const Key key) const;
  T* first_entry(const Key key) const;
  void refresh(const T* tte) const;
  void set_size(size_t mbSize);
  void clear();
  void store(const Key key, Value v, Bound type, Depth d, Move m, Value statV);
  void store(const Key key, Value v, Bound type, Depth d, Move m, Value statV, bool interested);
  void from_phash() {}
  void to_phash() {}

private:
  uint32_t hashMask;
  T* table;
  void* mem;
  uint8_t generation; // Size must be not bigger than TTEntry::generation8
};

extern TranspositionTable<TTEntry> TT;
extern TranspositionTable<PHEntry> PH;


/// TranspositionTable::first_entry() returns a pointer to the first entry of
/// a cluster given a position. The lowest order bits of the key are used to
/// get the index of the cluster.

template<class T>
inline T* TranspositionTable<T>::first_entry(const Key key) const {

  return table + ((uint32_t)key & hashMask);
}


/// TranspositionTable::refresh() updates the 'generation' value of the TTEntry
/// to avoid aging. Normally called after a TT hit.

template<class T>
inline void TranspositionTable<T>::refresh(const T* tte) const {

  const_cast<T*>(tte)->set_generation(generation);
}

// These declarations are mandatory, or the compiler will optimize the specialization out!
template<>
void TranspositionTable<PHEntry>::from_phash();
template<>
void TranspositionTable<PHEntry>::to_phash();

#endif // #ifndef TT_H_INCLUDED
