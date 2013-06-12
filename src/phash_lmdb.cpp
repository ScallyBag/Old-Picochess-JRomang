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
#include "phash_qdbm.h"
#include <sys/stat.h>

LMDB_PersistentHash LMDB;

////
//// Local definitions
////

////
//// LMDB_PHFile
////

// shouldn't be called directly
LMDB_PHFile::LMDB_PHFile(std::string &fname, int mode) :
  e(NULL), t(NULL), d(0), fn(""), tc(0)
{
  fn = fname;
  if (!mdb_env_create(&e)) {
    fixmapsize_phash(); // fix the file/map size as necessary
    if (!mdb_env_open(e, fn.c_str(), MDB_NOSUBDIR | MDB_WRITEMAP, 0644)) {
      if (!mdb_txn_begin(e, NULL, (mode == PHASH_MODE_READ) ? MDB_RDONLY : 0, &t)) {
        if (!mdb_dbi_open(t, NULL, (mode == PHASH_MODE_WRITE) ? MDB_CREATE : 0, &d)) {
          MDB_envinfo info;
          MDB_stat st;
          mdb_env_info(e, &info);
          mdb_stat(t, d, &st);
          mp = info.me_mapsize / st.ms_psize;
          return;
        }
      }
    }
  }
  close(false);
}

LMDB_PHFile::~LMDB_PHFile()
{
  close();
}

LMDB_PHFile *LMDB_PHFile::open(std::string &filename, int mode)
{
  LMDB_PHFile *file = NULL;
  bool usePersHash = Options["Use Persistent Hash"];
  
  if (usePersHash) {
    file = new LMDB_PHFile(filename, mode);
    if (!file->env()) {
      delete file;
      file = NULL;
    }
  }
  return file;
}

void LMDB_PHFile::close(bool commit)
{
  if (e) {
    std::string lockfile = fn + "-lock";
    if (t) {
      if (commit)
        mdb_txn_commit(t);
      else
        mdb_txn_abort(t);
      t = NULL;
    }
    if (d) {
      mdb_dbi_close(e, d);
      d = 0;
    }
    e = NULL;
    remove(lockfile.c_str());
  }
  tc = 0;
  fn = "";
}

void LMDB_PHFile::fixmapsize_phash()
{
  if (e) {
    int hashsize = Options["Persistent Hash Size"];
    struct stat filestatus;
    int filesize = 0;
    
    hashsize *= (1024 * 1024);
    if (!::stat(fn.c_str(), &filestatus)) {
      filesize = (int)filestatus.st_size;
    }
    if (hashsize != filesize) {
      mdb_env_set_mapsize(e, hashsize);
      // necessary to fix the size; otherwise the db doesn't allow access
      mdb_env_open(e, fn.c_str(), MDB_NOSUBDIR | MDB_WRITEMAP, 0644);
      mdb_env_close(e);
      mdb_env_create(&e);
      mdb_env_set_mapsize(e, hashsize);
      // end necessary
    }
  }
}

int LMDB_PHFile::put(MDB_val *vKey, MDB_val *vData, int flags)
{
  int rv = -1;
  
  if (e && t && d) {
    if (++tc > MAX_TXN) {
      mdb_txn_commit(t);
      mdb_txn_begin(e, NULL, 0, &t);
      tc = 0;
    }
    rv = mdb_put(t, d, vKey, vData, flags);
  }
  return rv;
}

int LMDB_PHFile::get(MDB_val *vKey, MDB_val *vData)
{
  if (e && t && d) {
    return mdb_get(t, d, vKey, vData);
  }
  return -1;
}

int LMDB_PHFile::clear()
{
  if (e && t && d) {
    return mdb_drop(t, d, 0);
  }
  return -1;
}

int LMDB_PHFile::commit(bool nextreadonly)
{
  if (e && t && d) {
    mdb_txn_commit(t);
    return mdb_txn_begin(e, NULL, nextreadonly ? MDB_RDONLY : 0, &t);
    tc = 0;
  }
  return -1;
}

int LMDB_PHFile::abort(bool nextreadonly)
{
  if (e && t && d) {
    mdb_txn_abort(t);
    return mdb_txn_begin(e, NULL, nextreadonly ? MDB_RDONLY : 0, &t);
    tc = 0;
  }
  return -1;
}

MDB_cursor *LMDB_PHFile::cursor()
{
  MDB_cursor *c = NULL;

  if (e && t && d) {
    mdb_cursor_open(t, d, &c);
  }
  return c;
}

size_t LMDB_PHFile::numentries()
{
  if (e && t && d) {
    MDB_stat st;

    mdb_stat(t, d, &st);
    return st.ms_entries;
  }
  return 0;
}

void LMDB_PHFile::stat(MDB_stat *st)
{
  if (e && t && d) {
    mdb_stat(t, d, st);
  }
}

////
//// LMDB_PersistentHash
////

LMDB_PersistentHash::LMDB_PersistentHash() :
PersHashFile(NULL),
PersHashFileName("stockfish.hsh"),
PersHashPrunefileName("stockfish_pruned.hsh"),
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
    std::string filename = Options["Persistent Hash File"];
    std::string rawname;
    std::string extensi;
    size_t lastindex;
    
    lastindex = filename.find_last_of(".");
    if (lastindex != std::string::npos) {
      rawname = filename.substr(0, lastindex);
      extensi = filename.substr(lastindex, std::string::npos);
    } else {
      rawname = filename;
      extensi = ".hsh";
    }
    PersHashPrunefileName = rawname + "_pruned" + extensi;
    // note: v005 will crash when performing operations on a LMDB file, but who cares?
    PersHashFileName = filename;

    // convert from QDBM if necessary
    QDBM.init_phash();
    QDBM.quit_phash();
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
#ifdef PHASH_DEBUG
    starttransaction_phash(PHASH_MODE_READ);
    count_phash();
    endtransaction_phash();
#endif
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

bool LMDB_PersistentHash::commit_and_rebuild(bool commit, bool UNUSED(optimize))
{
  if (PersHashFile) {
    PersHashFile->close(commit);
    delete PersHashFile;
    PersHashFile = NULL;
  }
#if 0
  if (optimize) {
    optimize_phash(); // don't do this in realtime, only at quit
  }
#endif
  PersHashFile = LMDB_PHFile::open(PersHashFileName, PHASH_MODE_WRITE);
  if (PersHashFile) {
    return true;
  }
  return false;
}

int LMDB_PersistentHash::put_withprune(MDB_val *vKey, MDB_val *vData)
{
  int rv = -1;
  int pruned = 0;

  if (PersHashFile && vKey && vData) {
    int depth = Options["Persistent Hash Depth"];
#if 1 // better method to handle this prophylactically
    MDB_stat stat;
    MDB_stat stat2;
    size_t usedpages = 0;
    PersHashFile->stat(&stat);

    mdb_stat(PersHashFile->txn(), 0, &stat2);
    usedpages = (stat.ms_branch_pages + stat.ms_leaf_pages + stat.ms_overflow_pages);
    if (usedpages > (PersHashFile->maxpages() * 0.8)) {
      MDB_cursor *c = NULL;
      MDB_val vKey, vData;
      size_t *iptr;
      size_t freepages = 0;

      PersHashFile->commit(true);
      if (!mdb_cursor_open(PersHashFile->txn(), 0, &c)) {
        while(!mdb_cursor_get(c, &vKey, &vData, MDB_NEXT)) {
          iptr = (size_t *)vData.mv_data;
          freepages += *iptr;
        }
        mdb_cursor_close(c);
      } else {
        printf("wtf\n");
      }
      PersHashFile->abort();
      if (usedpages - freepages > (PersHashFile->maxpages() * 0.8)) {
        do {
          pruned = prune_below_phash(depth);
          commit_and_rebuild(true);
        } while (depth++ <= 99 && !pruned);
      }
    }
    rv = PersHashFile->put(vKey, vData);
    if (rv) {
      if (rv == MDB_MAP_FULL) {
        sync_cout << "info string Persistent Hash catastrophic failure (out of space in PH file). Increase your Persistent Hash Size." << sync_endl;
        sync_cout << "info string Persistent Hash setting Use Persistent Hash to false." << sync_endl;
        delete PersHashFile;
        PersHashFile = NULL;
        Options["Use Persistent Hash"] = t_to_string("false");
      }
    }
#else
    rv = PersHashFile->put(vKey, vData);
    if (rv) {
      if (rv == MDB_MAP_FULL) {
        int hashsize = Options["Persistent Hash Size"];

        // double the phash size pre-purge so that we have some room to work
        Options["Persistent Hash Size"] = t_to_string(hashsize * 2);
        // we're going to lose the last transaction, I'm afraid
        if (commit_and_rebuild(false)) {
          // restore the phash size before calling prune so that the prune DB isn't 2x
          Options["Persistent Hash Size"] = t_to_string(hashsize);
          do {
            pruned = prune_below_phash(depth);
          } while (depth++ <= 99 && !pruned);
          // commit the prune, close and reopen the DB at the old size (as possible)
          if (commit_and_rebuild(true, true)) {
            // try again and fail mercilessly if it doesn't work
            rv = PersHashFile->put(vKey, vData);
            if (rv) {
              sync_cout << "info string Persistent Hash catastrophic failure (out of space in PH file). Increase your Persistent Hash Size." << sync_endl;
              sync_cout << "info string Persistent Hash setting Use Persistent Hash to false." << sync_endl;
              Options["Use Persistent Hash"] = t_to_string("false");
            }
          }
        }
      }
    }
#endif
  }
  return rv;
}

bool LMDB_PersistentHash::dostore_phash(const Key key, t_phash_data &data)
{
  MDB_val vKey;
  MDB_val vData;
  int rv;
  
  vKey.mv_size = sizeof(Key);
  vKey.mv_data = (void *)(intptr_t)&key;
  vData.mv_size = sizeof(t_phash_data);
  vData.mv_data = (void *)&data;
  
  rv = put_withprune(&vKey, &vData);
#ifdef PHASH_DEBUG
  if (!rv) {
    //printf("mdb_put: put %llx\n", key);
  } else {
    printf("mdb_put: %s\n", mdb_strerror(rv));
  }
#endif
  return !rv;
}

bool LMDB_PersistentHash::store_phash(const Key key, t_phash_data &data)
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

bool LMDB_PersistentHash::store_phash(const Key key, Value v, Bound t, Depth d, Move m, Value statV, Value kingD)
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

void LMDB_PersistentHash::starttransaction_phash(PHASH_MODE mode)
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
  PersHashFile = LMDB_PHFile::open(PersHashFileName, mode);
  return;
}

void LMDB_PersistentHash::endtransaction_phash()
{
  if (PersHashFile) {
    delete PersHashFile;
    PersHashFile = NULL;
  }
}

int LMDB_PersistentHash::prune_below_phash(int depth)
{
  unsigned count = 0;

  if (PersHashFile) {
    int rv;
    LMDB_PHFile *pfile;
    MDB_cursor *cursor;

    pfile = LMDB_PHFile::open(PersHashPrunefileName, PHASH_MODE_WRITE);
    cursor = PersHashFile->cursor();
    if (cursor) {
      MDB_val vKey;
      MDB_val vData;
      size_t entries;
      unsigned txct = 0;
      // start at the end and work backward since we're deleting stuff.
      entries = PersHashFile->numentries();
      
      while (!mdb_cursor_get(cursor, &vKey, &vData, MDB_PREV) && count != entries) {
        if (vData.mv_size == sizeof(t_phash_data)) {
          if (((t_phash_data *)vData.mv_data)->d <= depth) {
            if (pfile) {
              rv = pfile->put(&vKey, &vData);
              if (rv) {
                if (rv == MDB_MAP_FULL) {
                  // tant pis, shit happens; we could make a new file or change the map size if it turns out to be important.
                }
                pfile->close(true);
                delete pfile;
                pfile = NULL;
              }
            }
            rv = mdb_cursor_del(cursor, 0);
            if (!rv) {
              count++;
            }
#ifdef PHASH_DEBUG
            else {
              printf("mdb_del: %s\n", mdb_strerror(rv));
            }
#endif
            if (++txct > LMDB_PHFile::MAX_TXN) { // we have to be careful about overloading cursor_del transactions, too
              mdb_cursor_close(cursor);
              PersHashFile->commit();
              cursor = PersHashFile->cursor();
              mdb_cursor_get(cursor, &vKey, &vData, MDB_SET); // reset the cursor for the new transaction
              txct = 0;
            }
          }
        }
      }
      mdb_cursor_close(cursor);
#ifdef PHASH_DEBUG
      printf("prune_below_phash: pruned %d from %ld at depth %d\n", count, entries, depth);
#endif
    }
    if (pfile) {
      pfile->close(true);
      delete pfile;
    }
  }
  return count;
}


// the basic algorithm is: check the file size, if it's higher than the target size
// delete all entries at the minimum depth and optimize
// if still too big, repeat with the next highest depth and so on until we're below the target size
void LMDB_PersistentHash::doprune_phash()
{
  if (PersHashFile) {
    int depth = Options["Persistent Hash Depth"];
    unsigned pruned;
    std::stringstream ss;

    pruned = prune_below_phash(depth);
    // commit the prune, close and reopen the DB at the old size (as possible)
    commit_and_rebuild(true, true);

    sync_cout << "info string Persistent Hash pruned at depth " << depth << " [" << pruned << " record(s)]." << sync_endl;
  }
}

void LMDB_PersistentHash::doclear_phash()
{
  if (PersHashFile) {
    int rv;
    
    rv = PersHashFile->clear();
    if (!rv) {
#if 0 // don't do this in realtime, only at quit
      endtransaction_phash();
      optimize_phash();
      starttransaction_phash(PHASH_MODE_WRITE);
#endif
    }
  }
}

void LMDB_PersistentHash::domerge_phash()
{
  if (PersHashFile) {
    std::string mergename = Options["Persistent Hash Merge File"];
    int mindepth = Options["Persistent Hash Depth"];
    LMDB_PHFile *mfile;

    mfile = LMDB_PHFile::open(mergename, PHASH_MODE_READ);
    if (mfile) {
      MDB_cursor *cursor;
      MDB_val vKey;
      MDB_val vData;
      int merged = 0;
      int count = 0;

      cursor = mfile->cursor();
      if (cursor) {
        while (!mdb_cursor_get(cursor, &vKey, &vData, MDB_NEXT)) {
          if (vData.mv_size == sizeof(t_phash_data)) {
            Depth md = (Depth)((t_phash_data *)vData.mv_data)->d;
            if (md >= mindepth) {
              Depth depth;
              probe_phash(*((const Key *)vKey.mv_data), &depth);
              if (md > depth) {
                if (!put_withprune(&vKey, &vData)) {
                  merged++;
                }
              }
            }
          }
          count++;
        }
        mdb_cursor_close(cursor);
      }
      sync_cout << "info string Persistent Hash merged " << merged << " records (from " << count << " total) from file " << mergename << "." << sync_endl;
      mfile->close(false);
      delete mfile;
    }
  }
}

int LMDB_PersistentHash::probe_phash(const Key key, Depth *d)
{
  int rv = 0;
  
  *d = DEPTH_ZERO;
  if (PersHashFile) {
    MDB_val vKey;
    MDB_val vData;
    
    vKey.mv_size = sizeof(Key);
    vKey.mv_data = (void *)(intptr_t)&key;

    rv = PersHashFile->get(&vKey, &vData);
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
  if (PersHashFile) {
    MDB_cursor *cursor;
#ifdef PHASH_DEBUG
    int count = 0;
#endif
    
    cursor = PersHashFile->cursor();
    if (cursor) {
      MDB_val vKey;
      MDB_val vData;
      t_phash_data *data;
      
      while (!mdb_cursor_get(cursor, &vKey, &vData, MDB_NEXT)) {
        if (vData.mv_size == sizeof(t_phash_data)) {
          data = (t_phash_data *)vData.mv_data;
          TT.store(*((Key *)vKey.mv_data), (Value)data->v, (Bound)data->t, (Depth)data->d, (Move)data->m, (Value)data->statV, (Value)data->kingD, false);
#ifdef PHASH_DEBUG
          //printf("mdb_cursor_get: pull %llx\n", *((Key *)vKey.mv_data));
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
  std::string filename = PersHashFileName;
  std::string lockname = filename + "-lock";
  std::string bakname = filename + ".bak";
  std::string baklockname = bakname + "-lock";
  int success = false;
  LMDB_PHFile *ofile;

  remove(bakname.c_str());
  remove(baklockname.c_str());
  copyfile(filename, bakname);

  ofile = LMDB_PHFile::open(bakname, PHASH_MODE_READ);
  if (ofile) {
    remove(filename.c_str());
    remove(lockname.c_str());
    starttransaction_phash(PHASH_MODE_WRITE);
    if (PersHashFile) {
      MDB_cursor *cursor;

      cursor = ofile->cursor();
      if (cursor) {
        MDB_val vKey;
        MDB_val vData;
        while (!mdb_cursor_get(cursor, &vKey, &vData, MDB_NEXT)) {
          put_withprune(&vKey, &vData);
        }
        mdb_cursor_close(cursor);
        success = true;
      }
    }
    endtransaction_phash();
    ofile->close(false);
    delete ofile;
  }
  if (!success) {
    copyfile(bakname, filename);
  }
  remove(lockname.c_str());
  remove(bakname.c_str());
}

int LMDB_PersistentHash::count_phash()
{
  if (PersHashFile) {
    size_t entries = PersHashFile->numentries();
#ifdef PHASH_DEBUG
    printf("phash file has %d entries\n", (int)entries);
#endif
    return (int)entries;
  }
  return 0;
}
