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

#include <cstring>
#include <iostream>

#include "bitboard.h"
#ifdef PA_GTB
#include "phash.h"
#endif
#include "tt.h"

TranspositionTable<TTEntry> TT; // Our global transposition table
TranspositionTable<PHEntry> PH;


void TTEntry::phash_store(Key k64, Value v, Bound b, Depth d, Move m, Value ev) {

  PH.store(k64, v, b, d, m, ev, true);
}

/// TranspositionTable::resize() sets the size of the transposition table,
/// measured in megabytes. Transposition table consists of a power of 2 number
/// of clusters and each cluster consists of ClusterSize number of TTEntry.

template<class T>
void TranspositionTable<T>::resize(uint64_t mbSize) {

  assert(msb((mbSize << 20) / sizeof(T)) < 32);

  uint32_t size = ClusterSize << msb((mbSize << 20) / sizeof(T[ClusterSize]));

  if (hashMask == size - ClusterSize)
      return;

  hashMask = size - ClusterSize;
  free(mem);
  mem = calloc(size * sizeof(T) + CACHE_LINE_SIZE - 1, 1);

  if (!mem)
  {
      std::cerr << "Failed to allocate " << mbSize
                << "MB for transposition table." << std::endl;
      exit(EXIT_FAILURE);
  }

  table = (T*)((uintptr_t(mem) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
}


/// TranspositionTable::clear() overwrites the entire transposition table
/// with zeroes. It is called whenever the table is resized, or when the
/// user asks the program to clear the table (from the UCI interface).

template<class T>
void TranspositionTable<T>::clear() {

  std::memset(table, 0, (hashMask + ClusterSize) * sizeof(T));
}


/// TranspositionTable::probe() looks up the current position in the
/// transposition table. Returns a pointer to the TTEntry or NULL if
/// position is not found.

template<class T>
const T* TranspositionTable<T>::probe(const Key key) const {

  T* tte = first_entry(key);
  uint32_t key32 = key >> 32;

  for (unsigned i = 0; i < ClusterSize; ++i, ++tte)
      if (tte->key() == key32)
      {
          tte->generation8 = generation; // Refresh
          return tte;
      }

  return NULL;
}


/// TranspositionTable::store() writes a new entry containing position key and
/// valuable information of current position. The lowest order bits of position
/// key are used to decide in which cluster the position will be placed.
/// When a new entry is written and there are no empty entries available in the
/// cluster, it replaces the least valuable of the entries. A TTEntry t1 is considered
/// to be more valuable than a TTEntry t2 if t1 is from the current search and t2
/// is from a previous search, or if the depth of t1 is bigger than the depth of t2.

template<class T>
void TranspositionTable<T>::store(const Key key, Value v, Bound b, Depth d, Move m, Value statV) {

  TranspositionTable<T>::store(key, v, b, d, m, statV, true);
}


template<class T>
void TranspositionTable<T>::store(const Key key, Value v, Bound b, Depth d, Move m, Value statV, bool interested) {

  T *tte, *replace;
  uint32_t key32 = key >> 32; // Use the high 32 bits as key inside the cluster
  uint32_t ttekey;

  tte = replace = first_entry(key);

  for (unsigned i = 0; i < ClusterSize; ++i, ++tte)
  {
      ttekey = tte->key();
      if (!ttekey || ttekey == key32) // Empty or overwrite old
      {
          if (!m)
              m = tte->move(); // Preserve any existing ttMove

          replace = tte;
          break;
      }

      // Implement replace strategy
      if (  (    tte->generation8 == generation || tte->bound() == BOUND_EXACT)
          - (replace->generation8 == generation)
          - (tte->depth16 < replace->depth16) < 0)
          replace = tte;
  }
  replace->save(key, key32, v, b, d, m, generation, statV, interested);
}

// TranspositionTable<PHEntry> overrides for TranspositionTable::to_phash() 

template<>
void TranspositionTable<PHEntry>::to_phash() {

#ifdef PHASH_DEBUG
  struct timeval tv1, tv2;
  unsigned entries = 0;
  unsigned count = 0;
  unsigned rootcount = 0;

  gettimeofday(&tv1, NULL);
#endif
  int minDepth = Options["Persistent Hash Depth"] * ONE_PLY;

  PHInst.starttransaction_phash(PHASH_MODE_WRITE);
  for (unsigned i = 0; i < (hashMask + ClusterSize); i++) {
    PHEntry *phe = table + i;
    Key key;
    if ((key = phe->fullkey())) {
      const TTEntry *tte = TT.probe(key);
      if ( tte &&
          (tte->bound() == BOUND_EXACT) &&
          (DEPTH_IS_VALID(tte->depth(), minDepth)))
      {
        if (PHInst.store_phash(key, tte->value(), phe->fulltype(), tte->depth(), tte->move(), tte->eval_value())) {
#ifdef PHASH_DEBUG
          if (phe->fulltype() & BOUND_ROOT) rootcount++;
          count++;
#endif
        }
      }
    }
#ifdef PHASH_DEBUG
    entries++;
#endif
  }
  PHInst.endtransaction_phash();
  clear(); // clear the hash each time
#ifdef PHASH_DEBUG
  if (count) {
    gettimeofday(&tv2, NULL);
    sync_cout << "\nTranspositionTable<PHEntry>::to_phash stored "
              << count << " entries (" << rootcount << " root, " << entries << " total) in "
              << ((tv2.tv_sec * 1000.) + (tv2.tv_usec / 1000.) - (tv1.tv_sec * 1000.) + (tv1.tv_usec / 1000.))
              << " milliseconds.\n" << sync_endl;
  }
#endif
}

/// TranspositionTable::from_phash()

template<>
void TranspositionTable<PHEntry>::from_phash() {

#ifdef PHASH_DEBUG
  struct timeval tv1, tv2;

  gettimeofday(&tv1, NULL);
#endif
  PHInst.starttransaction_phash(PHASH_MODE_READ);
  PHInst.to_tt_phash();
  PHInst.endtransaction_phash();
#ifdef PHASH_DEBUG
  gettimeofday(&tv2, NULL);
  sync_cout << "\nTranspositionTable<PHEntry>::from_phash executed in "
            << ((tv2.tv_sec * 1000.) + (tv2.tv_usec / 1000.) - (tv1.tv_sec * 1000.) + (tv1.tv_usec / 1000.))
            << " milliseconds.\n" << sync_endl;
#endif
}

template class TranspositionTable<TTEntry>;
template class TranspositionTable<PHEntry>;
