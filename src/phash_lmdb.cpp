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
#include "phash_lmdb.h"
#include <sys/stat.h>

LMDB_PersistentHash LMDB;

////
//// Local definitions
////

////
//// Functions
////

LMDB_PersistentHash::LMDB_PersistentHash() :
PersHashEnv(NULL),
PersHashTxn(NULL),
PersHashDbi(0),
PersHashWantsClear(false),
PersHashWantsPrune(false),
PersHashWantsMerge(false)
{
  ;
}

void LMDB_PersistentHash::init_phash()
{
  bool usePersHash = Options["Use Persistent Hash"];
  
  if (usePersHash) {
    std::string persHashFilePath = Options["Persistent Hash File"];
    std::string persHashMergePath = Options["Persistent Hash Merge File"];

    //convert from qdbm?
  }
  starttransaction_phash(PHASH_MODE_READ); // in case we asked for a clear, purge or merge
#ifdef PHASH_DEBUG
  count_phash();
#endif
  endtransaction_phash();
}

void LMDB_PersistentHash::quit_phash()
{
  bool usePersHash = Options["Use Persistent Hash"];

  if (usePersHash) {
    optimize_phash();
    starttransaction_phash(PHASH_MODE_READ);
#ifdef PHASH_DEBUG
    count_phash();
#endif
    endtransaction_phash();
  }
}

void LMDB_PersistentHash::clear_phash()
{
  starttransaction_phash(PHASH_MODE_WRITE);
  doclear_phash();
  endtransaction_phash();
}

void LMDB_PersistentHash::wantsclear_phash()
{
  MainThread *t = Threads.main_thread();
  if (t->thinking) {
    PersHashWantsClear = true;
  } else {
    clear_phash();
  }
}

void LMDB_PersistentHash::prune_phash()
{
  starttransaction_phash(PHASH_MODE_WRITE);
  doprune_phash();
  endtransaction_phash();
}

void LMDB_PersistentHash::wantsprune_phash()
{
  MainThread *t = Threads.main_thread();
  if (t->thinking) {
    PersHashWantsPrune = true;
  } else {
    prune_phash();
  }
}

void LMDB_PersistentHash::merge_phash()
{
  starttransaction_phash(PHASH_MODE_WRITE);
  domerge_phash();
  endtransaction_phash();
}

void LMDB_PersistentHash::wantsmerge_phash()
{
  MainThread *t = Threads.main_thread();
  if (t->thinking) {
    PersHashWantsMerge = true;
  } else {
    merge_phash();
  }
}

MDB_env *LMDB_PersistentHash::open_phash(PHASH_MODE UNUSED(mode))
{

  bool usePersHash = Options["Use Persistent Hash"];
  std::string filename = Options["Persistent Hash File"];
  int hashsize = Options["Persistent Hash Size"];
  MDB_env *env = NULL;
  
  if (usePersHash) {
    if (!mdb_env_create(&env)) {
      int rv;
      int flags = MDB_NOSUBDIR;
      struct stat filestatus;
      int filesize = 0;
      if (!stat(filename.c_str(), &filestatus)) {
        filesize = (int)filestatus.st_size;
      }
      if (hashsize * 1024 * 1024 != filesize) {
        mdb_env_set_mapsize(env, hashsize * 1024 * 1024);
        mdb_env_close(env);
        mdb_env_create(&env);
      }
      // we don't need this flag for the environment, rather for the transaction
      // if (mode == PHASH_MODE_READ) flags |= MDB_RDONLY;
      rv = mdb_env_open(env, filename.c_str(), flags, 0644);
      if (rv) { // fail
        mdb_env_close(env);
        env = NULL;
#ifdef PHASH_DEBUG
        // note that this may fail if we are read-only and there is no file
        printf("mdb_env_open: %s\n", mdb_strerror(rv));
#endif
      }
    }
  }
  return env;
}

void LMDB_PersistentHash::close_phash(MDB_env *env)
{
  if (env) {
    mdb_env_close(env);
  }
}

void LMDB_PersistentHash::store_phash(const Key key, Value v, Bound t, Depth d, Move m, Value statV, Value kingD)
{
  Depth oldDepth = DEPTH_ZERO;
  
  if (PersHashEnv && PersHashTxn && PersHashDbi) {
    probe_phash(key, &oldDepth);
    if (d >= oldDepth) {
      MDB_val vKey;
      MDB_val vData;
      t_phash_data data;
      int rv = 0;

      rv = rv; // compiler warning
      data.v = v;
      data.t = t;
      data.d = d;
      data.m = m;
      data.statV = statV;
      data.kingD = kingD;

      vKey.mv_size = sizeof(Key);
      vKey.mv_data = (void *)(intptr_t)&key;
      vData.mv_size = sizeof(t_phash_data);
      vData.mv_data = (void *)&data;

      rv = mdb_put(PersHashTxn, PersHashDbi, &vKey, &vData, 0);
      if (rv == MDB_MAP_FULL) {
        // what should we do?
      }
#ifdef PHASH_DEBUG
      if (!rv) {
        printf("mdb_put: put %llx\n", key);
      } else {
        printf("mdb_put: %s\n", mdb_strerror(rv));
      }
#endif
    }
  }
}

void LMDB_PersistentHash::starttransaction_phash(PHASH_MODE mode)
{
  if (PersHashEnv && PersHashTxn && PersHashDbi) return;
  
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
  PersHashEnv = open_phash(mode);
  if (PersHashEnv) {
    int rv;
    
    rv = mdb_txn_begin(PersHashEnv, NULL, (mode == PHASH_MODE_READ) ? MDB_RDONLY : 0, &PersHashTxn);
    if (!rv) {
      rv = mdb_dbi_open(PersHashTxn, NULL, MDB_CREATE, &PersHashDbi);
      if (!rv) {
        return; // success
      } else {
#ifdef PHASH_DEBUG
        printf("mdb_dbi_open: %s\n", mdb_strerror(rv));
#endif
        goto fail;
      }
    } else {
#ifdef PHASH_DEBUG
      printf("mdb_txn_begin: %s\n", mdb_strerror(rv));
#endif
      goto fail;
    }
  }
  
fail:
  PersHashDbi = 0;
  if (PersHashTxn) {
    mdb_txn_abort(PersHashTxn);
  }
  if (PersHashEnv) {
    close_phash(PersHashEnv);
    PersHashEnv = NULL;
  }  
}

void LMDB_PersistentHash::endtransaction_phash()
{
  if (PersHashEnv) {
    if (PersHashTxn) {
      mdb_txn_commit(PersHashTxn);
      PersHashTxn = NULL;
    }
    if (PersHashDbi) {
      mdb_dbi_close(PersHashEnv, PersHashDbi);
      PersHashDbi = 0;
    }
    close_phash(PersHashEnv);
    PersHashEnv = NULL;
  }
}

int LMDB_PersistentHash::prune_below_phash(int depth)
{
  int count = 0;
  
  if (PersHashEnv && PersHashTxn && PersHashDbi) {
    int rv;
    MDB_cursor *cursor;

    rv = mdb_cursor_open(PersHashTxn, PersHashDbi, &cursor);
    if (!rv) {
      MDB_val vKey;
      MDB_val vData;
      int total = 0;
      
      while(!mdb_cursor_get(cursor, &vKey, &vData, MDB_NEXT)) {
        total++;
        if (vData.mv_size == sizeof(t_phash_data)) {
          if (((t_phash_data *)vData.mv_data)->d <= depth) {
            rv = mdb_cursor_del(cursor, 0);
            if (!rv) {
              count++;
            }
#ifdef PHASH_DEBUG
            else {
              printf("mdb_del: %s\n", mdb_strerror(rv));
            }
#endif
          }
        }
      }
      mdb_cursor_close(cursor);
#ifdef PHASH_DEBUG
      printf("prunebelow: pruned %d from %d\n", count, total);
#endif
    } else {
      
    }
  }
  return count;
}


// the basic algorithm is: check the file size, if it's higher than the target size
// delete all entries at the minimum depth and optimize
// if still too big, repeat with the next highest depth and so on until we're below the target size
void LMDB_PersistentHash::doprune_phash()
{
  if (PersHashEnv && PersHashTxn && PersHashDbi) {
    std::string filename = Options["Persistent Hash File"];
    int desiredFileSize = Options["Persistent Hash Size"] * (1024 * 1024);
    int hashDepth = Options["Persistent Hash Depth"];
    int pruneDepth = hashDepth;
    int currentFileSize = 0;
    int totalPruned = 0;
    std::ostringstream ss;
    struct stat filestatus;
    int origFileSize;

    endtransaction_phash();

    if (!stat(filename.c_str(), &filestatus)) {
      origFileSize = (int)filestatus.st_size;
    } else {
      sync_cout << "info string Persistent Hash could not get file size. Aborting." << sync_endl;
      return;
    }
    optimize_phash();
    stat(filename.c_str(), &filestatus);
    currentFileSize = (int)filestatus.st_size;

    if (currentFileSize < desiredFileSize) {
      sync_cout << "info string Persistent Hash optimized [no pruning necessary]. Previous size: " << origFileSize << " bytes; new size: " << currentFileSize << " bytes." << sync_endl;
      return;
    }
    while (pruneDepth < 100) {
      int pruned;
      
      starttransaction_phash(PHASH_MODE_WRITE);
      pruned = prune_below_phash(pruneDepth);
      endtransaction_phash();
      
      if (pruned) {
        optimize_phash();
        stat(filename.c_str(), &filestatus);
        currentFileSize = (int)filestatus.st_size;
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

void LMDB_PersistentHash::doclear_phash()
{
  if (PersHashEnv && PersHashTxn && PersHashDbi) {
    int rv;
    
    rv = mdb_drop(PersHashTxn, PersHashDbi, 0);
    if (!rv) {
      endtransaction_phash();
      optimize_phash();
      starttransaction_phash(PHASH_MODE_WRITE);
    }
  }
}

void LMDB_PersistentHash::domerge_phash()
{
  if (PersHashEnv && PersHashTxn && PersHashDbi) {
    std::string mergename = Options["Persistent Hash Merge File"];
    int mindepth = Options["Persistent Hash Depth"];
    MDB_env *menv;
    
    if (!mdb_env_create(&menv)) {
      if (!mdb_env_open(menv, mergename.c_str(), MDB_NOSUBDIR, 0644)) {
        MDB_txn *mtxn;
        if (!mdb_txn_begin(menv, NULL, MDB_RDONLY, &mtxn)) {
          MDB_dbi mdbi;
          if (!mdb_dbi_open(mtxn, NULL, 0, &mdbi)) {
            MDB_cursor *cursor;
            MDB_val vKey;
            MDB_val vData;
            int merged = 0;
            int count = 0;
            
            if (!mdb_cursor_open(mtxn, mdbi, &cursor)) {
              while (!mdb_cursor_get(cursor, &vKey, &vData, MDB_NEXT)) {
                if (vData.mv_size == sizeof(t_phash_data)) {
                  Depth md = (Depth)((t_phash_data *)vData.mv_data)->d;
                  if (md >= mindepth) {
                    Depth depth;
                    probe_phash(*((const Key *)vKey.mv_data), &depth);
                    if (md > depth) {
                      if (mdb_put(PersHashTxn, PersHashDbi, &vKey, &vData, 0) == MDB_MAP_FULL) {
                        // what should we do?
                      }
                      merged++;
                    }
                  }
                }
                count++;
              }
              mdb_cursor_close(cursor);
            }
            sync_cout << "info string Persistent Hash merged " << merged << " records (from " << count << " total) from file " << mergename << "." << sync_endl;

            mdb_dbi_close(menv, mdbi);
          }
          mdb_txn_abort(mtxn);
        }
      }
      mdb_env_close(menv);
    }
  }
}

int LMDB_PersistentHash::probe_phash(const Key key, Depth *d)
{
  int rv = 0;
  
  *d = DEPTH_ZERO;
  if (PersHashEnv && PersHashTxn && PersHashDbi) {
    MDB_val vKey;
    MDB_val vData;
    
    vKey.mv_size = sizeof(Key);
    vKey.mv_data = (void *)(intptr_t)&key;

    rv = mdb_get(PersHashTxn, PersHashDbi, &vKey, &vData);
    if (!rv) {
      if (vData.mv_size == sizeof(t_phash_data)) {
        *d = (Depth)((t_phash_data *)vData.mv_data)->d;
        rv = 1;
      }
    }
  }
  return rv;
}

void LMDB_PersistentHash::to_tt_phash()
{
  if (PersHashEnv && PersHashTxn && PersHashDbi) {
    int rv;
    MDB_cursor *cursor;
#ifdef PHASH_DEBUG
    int count = 0;
#endif
    
    rv = mdb_cursor_open(PersHashTxn, PersHashDbi, &cursor);
    if (!rv) {
      MDB_val vKey;
      MDB_val vData;
      t_phash_data *data;
      
      while (!mdb_cursor_get(cursor, &vKey, &vData, MDB_NEXT)) {
        if (vData.mv_size == sizeof(t_phash_data)) {
          data = (t_phash_data *)vData.mv_data;
          TT.store(*((Key *)vKey.mv_data), (Value)data->v, (Bound)data->t, (Depth)data->d, (Move)data->m, (Value)data->statV, (Value)data->kingD, false);
#ifdef PHASH_DEBUG
          printf("mdb_cursor_get: pull %llx\n", *((Key *)vKey.mv_data));
          count++;
#endif
        }
      }
      mdb_cursor_close(cursor);
    }
#ifdef PHASH_DEBUG
    printf("restored %d records\n", count);
#endif
  }
}

void LMDB_PersistentHash::copyfile(std::string &srcfile, std::string &dstfile)
{
  std::ifstream  src(srcfile.c_str(), std::ifstream::binary);
  std::ofstream  dst(dstfile.c_str(), std::ofstream::binary);

  dst << src.rdbuf();
}

void LMDB_PersistentHash::optimize_phash()
{
  std::string filename = Options["Persistent Hash File"];
  std::string lockname = filename + "-lock";
  std::string bakname = filename + ".bak";
  std::string baklockname = bakname + "-lock";
  int success = false;
  MDB_env *oenv;

  remove(bakname.c_str());
  remove(baklockname.c_str());
  copyfile(filename, bakname);

  if (!mdb_env_create(&oenv)) {
    if (!mdb_env_open(oenv, bakname.c_str(), MDB_NOSUBDIR, 0644)) {
      MDB_txn *otxn;
      if (!mdb_txn_begin(oenv, NULL, MDB_RDONLY, &otxn)) {
        MDB_dbi odbi;
        if (!mdb_dbi_open(otxn, NULL, 0, &odbi)) {
          remove(filename.c_str());
          remove(lockname.c_str());
          starttransaction_phash(PHASH_MODE_WRITE);
          if (PersHashEnv && PersHashTxn && PersHashDbi) {
            MDB_cursor *cursor;

            if (!mdb_cursor_open(otxn, odbi, &cursor)) {
              MDB_val vKey;
              MDB_val vData;
              while (!mdb_cursor_get(cursor, &vKey, &vData, MDB_NEXT)) {
                if (mdb_put(PersHashTxn, PersHashDbi, &vKey, &vData, 0) == MDB_MAP_FULL) {
                  // what should we do?
                }
              }
              mdb_cursor_close(cursor);
              success = true;
            }
          }
          endtransaction_phash();
          mdb_dbi_close(oenv, odbi);
        }
        mdb_txn_abort(otxn);
      }
    }
    mdb_env_close(oenv);
  }
  if (!success) {
    copyfile(bakname, filename);
  }
  remove(lockname.c_str());
  remove(bakname.c_str());
  remove(baklockname.c_str());
}

int LMDB_PersistentHash::count_phash()
{
  if (PersHashEnv && PersHashTxn && PersHashDbi) {
    MDB_stat stat;
    
    mdb_stat(PersHashTxn, PersHashDbi, &stat);
#ifdef PHASH_DEBUG
    printf("phash file has %d entries\n", (int)stat.ms_entries);
#endif
    return (int)stat.ms_entries;
  }
  return 0;
}
