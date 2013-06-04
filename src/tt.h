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

#if !defined(TT_H_INCLUDED)
#define TT_H_INCLUDED

#include "misc.h"
#include "types.h"
#if PA_GTB
#include "phash.h"
#include "ucioption.h"
#endif

/// The TTEntry is the class of transposition table entries
///
/// A TTEntry needs 128 bits to be stored
///
/// bit  0-31: key
/// bit 32-63: data
/// bit 64-79: value
/// bit 80-95: depth
/// bit 96-111: static value
/// bit 112-127: margin of static value
///
/// the 32 bits of the data field are so defined
///
/// bit  0-15: move
/// bit 16-20: not used
/// bit 21-22: value type
/// bit 23-31: generation

class TTEntry {

public:
  void save(Key k64, uint32_t k, Value v, Bound b, Depth d, Move m, int g, Value ev, Value em, bool interested) {

    key32        = (uint32_t)k;
    move16       = (uint16_t)m;
    bound        = (uint8_t)b;
    generation8  = (uint8_t)g;
    value16      = (int16_t)v;
    depth16      = (int16_t)d;
    evalValue    = (int16_t)ev;
    evalMargin   = (int16_t)em;
    if (interested && b == BOUND_EXACT && m != MOVE_NONE && Options["Use Persistent Hash"]) {
      if (d >= Options["Persistent Hash Depth"]) {
        phash_store(k64, v, b, d, m, ev, em);
      }
    }
  }

  void set_generation(int g) { generation8 = (uint8_t)g; }

  uint32_t key() const      { return key32; }
  Depth depth() const       { return (Depth)depth16; }
  Move move() const         { return (Move)move16; }
  Value value() const       { return (Value)value16; }
  Bound type() const        { return (Bound)bound; }
  int generation() const    { return (int)generation8; }
  Value eval_value() const  { return (Value)evalValue; }
  Value eval_margin() const { return (Value)evalMargin; }

private:
  void phash_store(Key k64, Value v, Bound b, Depth d, Move m, Value ev, Value em);

  uint32_t key32;
  uint16_t move16;
  uint8_t bound, generation8;
  int16_t value16, depth16, evalValue, evalMargin;
};


class PHEntry {

public:
  void save(Key k64, uint32_t UNUSED(k), Value UNUSED(v), Bound b, Depth d, Move m, int g, Value UNUSED(ev), Value UNUSED(em), bool UNUSED(nterested)) {

    key64        = (Key)k64;
    move16       = (uint16_t)m;
    bound        = (uint8_t)b;
    generation8  = (uint8_t)g;
    depth16      = (int16_t)d;
  }

  void set_generation(int g) { generation8 = (uint8_t)g; }

  uint32_t key() const      { return key64 >> 32; }
  Key fullkey() const       { return key64; }
  Depth depth() const       { return (Depth)depth16; }
  Move move() const         { return (Move)move16; }
  Bound type() const        { return (Bound)bound; }
  int generation() const    { return (int)generation8; }

private:
  Key key64;
  uint16_t move16;
  uint8_t bound, generation8;
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
  void new_search() { generation++; }

  T* probe(const Key key) const;
  T* first_entry(const Key key) const;
  void refresh(const T* tte) const;
  void set_size(size_t mbSize);
  void clear();
  void store(const Key key, Value v, Bound type, Depth d, Move m, Value statV, Value kingD);
  void store(const Key key, Value v, Bound type, Depth d, Move m, Value statV, Value kingD, bool interested);
  void from_phash() {}
  void to_phash() {}

private:
  uint32_t hashMask;
  T* table;
  void* mem;
  uint8_t generation; // Size must be not bigger then TTEntry::generation8
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

#endif // !defined(TT_H_INCLUDED)
