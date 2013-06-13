/*
 Stockfish, a UCI chess playing engine derived from Glaurung 2.1
 Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
 Copyright (C) 2008-2010 Marco Costalba, Joona Kiiski, Tord Romstad
 
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

////
//// Includes
////

#include "phash.h"

#ifdef USE_KYOTO

#include "phash_kyoto.h"
#include "phash_qdbm.h"

using namespace kyotocabinet;

KYOTO_PersistentHash KYOTO;


////
//// Local definitions
////

typedef struct _phash_data_old
{
  Value   v;
  Bound   t;
  Depth   d;
  Move    m;
  Value   statV;
  Value   kingD;
} t_phash_data_old;

////
//// Functions
////

KYOTO_PersistentHash::KYOTO_PersistentHash() :
  PersHashFile(NULL),
  PersHashWantsClear(false),
  PersHashWantsPrune(false),
  PersHashWantsMerge(false)
{
  ;
}

void KYOTO_PersistentHash::init_phash()
{
  bool usePersHash = Options["Use Persistent Hash"];
  
  if (usePersHash) {
    std::string persHashFilePath = Options["Persistent Hash File"];
    std::string persHashMergePath = Options["Persistent Hash Merge File"];

    // convert from QDBM if necessary
    QDBM.init_phash();
    QDBM.quit_phash();

    starttransaction_phash(PHASH_MODE_READ); // in case we asked for a clear, purge or merge
#ifdef PHASH_DEBUG
    count_phash();
#endif
    endtransaction_phash();
  }
}

void KYOTO_PersistentHash::quit_phash()
{
  starttransaction_phash(PHASH_MODE_WRITE);
  optimize_phash();
#ifdef PHASH_DEBUG
  count_phash();
#endif
  endtransaction_phash();
}

void KYOTO_PersistentHash::clear_phash()
{
  starttransaction_phash(PHASH_MODE_WRITE);
  doclear_phash();
  endtransaction_phash();
}

void KYOTO_PersistentHash::wantsclear_phash()
{
  MainThread *t = Threads.main_thread();
  if (t->thinking) {
    PersHashWantsClear = true;
  } else {
    clear_phash();
  }
}

void KYOTO_PersistentHash::prune_phash()
{
  starttransaction_phash(PHASH_MODE_WRITE);
  doprune_phash();
  endtransaction_phash();
}

void KYOTO_PersistentHash::wantsprune_phash()
{
  MainThread *t = Threads.main_thread();
  if (t->thinking) {
    PersHashWantsPrune = true;
  } else {
    prune_phash();
  }
}

void KYOTO_PersistentHash::merge_phash()
{
  starttransaction_phash(PHASH_MODE_WRITE);
  domerge_phash();
  endtransaction_phash();
}

void KYOTO_PersistentHash::wantsmerge_phash()
{
  MainThread *t = Threads.main_thread();
  if (t->thinking) {
    PersHashWantsMerge = true;
  } else {
    merge_phash();
  }
}

HashDB *KYOTO_PersistentHash::open_phash(std::string &filename, PHASH_MODE mode)
{
  bool usePersHash = Options["Use Persistent Hash"];
  int hashsize = Options["Persistent Hash Size"];
  HashDB *hash = NULL;
  
  if (usePersHash) {
    hash = new HashDB;
    if (hash) {
      if (mode == PHASH_MODE_WRITE) {
        hash->tune_options(HashDB::TSMALL);
        if (hashsize > 64) hash->tune_map((int64_t)hashsize * 1024LL * 1024LL);
      }
      if (!hash->open(filename, (mode == PHASH_MODE_WRITE) ? (HashDB::OWRITER | HashDB::OCREATE) : HashDB::OREADER)) {
        // error
        printf("open(): %s\n", hash->error().message());
        delete hash;
        hash = NULL;
      }
    }
  }
  return hash;
}

void KYOTO_PersistentHash::close_phash(HashDB *hash)
{
  hash->close();
}

bool KYOTO_PersistentHash::dostore_phash(const Key key, t_phash_data &data)
{
  int rv;
  rv = PersHashFile->set((const char *)&key, sizeof(Key), (const char *)&data, sizeof(t_phash_data));
#ifdef PHASH_DEBUG
  if (rv) {
    //printf("dpput: put %llx\n", key);
  } else {
    printf("set(): %s\n", PersHashFile->error().message());
  }
#endif
  return rv ? true : false;
}

bool KYOTO_PersistentHash::store_phash(const Key key, t_phash_data &data)
{
  Depth oldDepth = DEPTH_ZERO;
  
  if (PersHashFile) {
    probe_phash(key, &oldDepth);
    if (data.d >= oldDepth) {
      return dostore_phash(key, data);
    }
  }
  return false;
}

bool KYOTO_PersistentHash::store_phash(const Key key, Value v, Bound t, Depth d, Move m, Value statV, Value kingD)
{
  Depth oldDepth = DEPTH_ZERO;

  if (PersHashFile) {
    probe_phash(key, &oldDepth);
    if (d >= oldDepth) {
      t_phash_data data;

      data.v = v;
      data.t = t;
      data.d = d;
      data.m = m;
      data.statV = statV;
      data.kingD = kingD;

      return dostore_phash(key, data);
    }
  }
  return false;
}

void KYOTO_PersistentHash::starttransaction_phash(PHASH_MODE mode)
{
  if (PersHashFile) return;
  
  if (PersHashWantsClear) {
    PersHashWantsClear = false;
    clear_phash();
  } else {
    if (PersHashWantsMerge) {
      PersHashWantsMerge = false;
      merge_phash();
    }
    if (PersHashWantsPrune) {
      PersHashWantsPrune = false;
      prune_phash();
    }
  }
  std::string filename = Options["Persistent Hash File"];
  PersHashFile = open_phash(filename, mode);
  if (PersHashFile) {
    PersHashFile->begin_transaction();
  }
}

void KYOTO_PersistentHash::endtransaction_phash()
{
  if (PersHashFile) {
    PersHashFile->end_transaction();
    close_phash(PersHashFile);
    PersHashFile = NULL;
  }
}

int KYOTO_PersistentHash::prune_below_phash(int depth)
{
  int count = 0;

  if (PersHashFile) {
    DB::Cursor *c = PersHashFile->cursor();
    if (c) {
      char *key;
      size_t ksize;
      t_phash_data data;
      size_t dsize;
      c->jump();
      while((key = c->get(&ksize, (const char **)&data, &dsize))) {
        if (dsize == sizeof(t_phash_data)) {
          if (data.d <= depth) {
            c->remove(); // steps
            count++;
          } else {
            c->step();
          }
        }
        delete[] key;
      }
      delete c;
      sync_cout << PersHashFile->error().message() << sync_endl;
    }
  }
  return count;
}


// the basic algorithm is: check the file size, if it's higher than the target size
// delete all entries at the minimum depth and optimize
// if still too big, repeat with the next highest depth and so on until we're below the target size
void KYOTO_PersistentHash::doprune_phash()
{
  if (PersHashFile) {
    int desiredFileSize = Options["Persistent Hash Size"] * (1024 * 1024);
    int hashDepth = Options["Persistent Hash Depth"];
    int pruneDepth = hashDepth;
    int totalPruned = 0;
    std::ostringstream ss;
    size_t currentFileSize;
    size_t origFileSize = PersHashFile->size();
    
    optimize_phash();
    currentFileSize = PersHashFile->size();
    if (currentFileSize < desiredFileSize) {
      sync_cout << "info string Persistent Hash optimized [no pruning necessary]. Previous size: " << origFileSize << " bytes; new size: " << currentFileSize << " bytes." << sync_endl;
      return;
    }
    while (pruneDepth < 100) {
      int pruned = prune_below_phash(pruneDepth);
      if (pruned) {
        optimize_phash();
        currentFileSize = PersHashFile->size();
        totalPruned += pruned;
      } else {
        //sync_cout << "info string Persistent Hash pruned at depth " << pruneDepth << " [0 records]." << sync_endl;
      }
      if (currentFileSize < desiredFileSize) {
        if (hashDepth == pruneDepth) {
          ss << "info string Persistent Hash pruned at depth " << hashDepth;
        } else {
          ss << "info string Persistent Hash pruned between depths " << hashDepth << " and " << pruneDepth;
        }
        ss << " [" << totalPruned << " record(s)]. Previous size: " << origFileSize << " bytes; new size: " << currentFileSize << " bytes.";
        sync_cout << ss.str() << sync_endl;
        return;
      }
      pruneDepth++;
    }
  }
}

void KYOTO_PersistentHash::doclear_phash()
{
  if (PersHashFile) {
    PersHashFile->clear();
    optimize_phash();
  }
}

void KYOTO_PersistentHash::domerge_phash()
{
  if (PersHashFile) {
    std::string mergename = Options["Persistent Hash Merge File"];
    int mindepth = Options["Persistent Hash Depth"];
    HashDB *mergefile;

    mergefile = open_phash(mergename, PHASH_MODE_READ);
    if (mergefile) {
      // define the visitor
      class VisitorImpl : public DB::Visitor {
        // call back function for an existing record
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t *sp) {
          t_phash_data *data = (t_phash_data *)vbuf;
          if (data->d >= mindepth) {
            Depth depth;
            parent->probe_phash(*((const Key *)kbuf), &depth);
            if (data->d > depth) {
              target->set(kbuf, sizeof(Key), vbuf, sizeof(t_phash_data));
              merged++;
            }
          }
          total++;
          return NOP;
        }
        // call back function for an empty record space
        const char* visit_empty(const char* kbuf, size_t ksiz, size_t *sp) {
          //cerr << string(kbuf, ksiz) << " is missing" << endl;
          return NOP;
        }
      public:
        KYOTO_PersistentHash *parent;
        HashDB *target;
        int mindepth;
        unsigned merged;
        unsigned total;
      } visitor;

      visitor.parent = this;
      visitor.target = PersHashFile;
      visitor.mindepth = mindepth;
      visitor.merged = 0;
      visitor.total = 0;
      mergefile->iterate(&visitor, false);

      close_phash(mergefile);
      sync_cout << "info string Persistent Hash merged " << visitor.merged << " records (from " << visitor.total << " total) from file " << mergename << "." << sync_endl;
    }
  }
}

size_t KYOTO_PersistentHash::getsize_phash()
{
  if (PersHashFile) {
    return PersHashFile->size();
  }
  return 0;
}

int KYOTO_PersistentHash::probe_phash(const Key key, Depth *d)
{
  int rv = 0;
  
  *d = DEPTH_ZERO;
  if (PersHashFile) {
    t_phash_data data;
    int32_t datasize = PersHashFile->get((const char *)&key, sizeof(Key), (char *)&data, sizeof(t_phash_data));
    if (datasize == sizeof(t_phash_data)) {
      *d = (Depth)data.d;
      rv = 1;
    }
  }
  return rv;
}

void KYOTO_PersistentHash::to_tt_phash()
{
  if (PersHashFile) {
    // define the visitor
    class VisitorImpl : public DB::Visitor {
      // call back function for an existing record
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t *sp) {
        t_phash_data *data = (t_phash_data *)vbuf;
        TT.store(*((Key *)kbuf), (Value)data->v, (Bound)data->t, (Depth)data->d, (Move)data->m, (Value)data->statV, (Value)data->kingD, false);
        count++;
        return NOP;
      }
      // call back function for an empty record space
      const char* visit_empty(const char* kbuf, size_t ksiz, size_t *sp) {
        //cerr << string(kbuf, ksiz) << " is missing" << endl;
        return NOP;
      }
    public:
      unsigned count;
    } visitor;

    visitor.count = 0;
    PersHashFile->iterate(&visitor, false);
#ifdef PHASH_DEBUG
    printf("restored %d records\n", visitor.count);
#endif
  }
}

void KYOTO_PersistentHash::optimize_phash()
{
  if (PersHashFile) {
    PersHashFile->defrag();
  }
}

int KYOTO_PersistentHash::count_phash()
{
  int64_t count = 0;

  if (PersHashFile) {
    count = PersHashFile->count();
#ifdef PHASH_DEBUG
    printf("counted %d records\n", (int)count);
#endif
  }
  return (int)count;
}

#endif // USE_KYOTO
