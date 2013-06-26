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
#include "phash_qdbm.h"
#include <ctime>
#include <sys/stat.h>

QDBM_PersistentHash QDBM;

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

QDBM_PersistentHash::QDBM_PersistentHash() :
  PersHashFile(NULL),
  PersHashWantsClear(false),
  PersHashWantsPrune(false),
  PersHashWantsMerge(false)
{
  ;
}

void QDBM_PersistentHash::init_phash()
{
  bool usePersHash = Options["Use Persistent Hash"];
  
  if (usePersHash) {
    std::string persHashFilePath = Options["Persistent Hash File"];
    std::string persHashMergePath = Options["Persistent Hash Merge File"];

    convert_phash(persHashFilePath); // in case of old-format phash files
    convert_phash(persHashMergePath);
  }
#if 0
  starttransaction_phash(PHASH_MODE_READ); // in case we asked for a clear, purge or merge
#ifdef PHASH_DEBUG
  count_phash();
#endif
  endtransaction_phash();
#endif
}

void QDBM_PersistentHash::quit_phash()
{
#if 0
  starttransaction_phash(PHASH_MODE_WRITE);
  optimize_phash();
  endtransaction_phash();
#endif
}

void QDBM_PersistentHash::clear_phash()
{
  starttransaction_phash(PHASH_MODE_WRITE);
  doclear_phash();
  endtransaction_phash();
}

void QDBM_PersistentHash::wantsclear_phash()
{
  MainThread *t = Threads.main_thread();
  if (t->thinking) {
    PersHashWantsClear = true;
  } else {
    clear_phash();
  }
}

void QDBM_PersistentHash::prune_phash()
{
  starttransaction_phash(PHASH_MODE_WRITE);
  doprune_phash();
  endtransaction_phash();
}

void QDBM_PersistentHash::wantsprune_phash()
{
  MainThread *t = Threads.main_thread();
  if (t->thinking) {
    PersHashWantsPrune = true;
  } else {
    prune_phash();
  }
}

void QDBM_PersistentHash::merge_phash()
{
  starttransaction_phash(PHASH_MODE_WRITE);
  domerge_phash();
  endtransaction_phash();
}

void QDBM_PersistentHash::wantsmerge_phash()
{
  MainThread *t = Threads.main_thread();
  if (t->thinking) {
    PersHashWantsMerge = true;
  } else {
    merge_phash();
  }
}

bool QDBM_PersistentHash::needsconvert_phash(DEPOT *depot)
{
  bool rv = false;
  
  // check the first record. if it's the old size, we want to convert.
  if (depot) {
    if (dpiterinit(depot)) {
      char *key;
      while ((key = dpiternext(depot, NULL))) {
        t_phash_data_old data;
        int datasize = 0;
        
        datasize = dpgetwb(depot, (const char *)key, (int)sizeof(Key), 0, (int)sizeof(t_phash_data_old), (char *)&data);
        if (datasize == sizeof(t_phash_data_old)) {
          rv = true;
        } else {
          rv = false;
        }
        free(key);
        break;
      }
    }
  }
  return rv;
}

void QDBM_PersistentHash::doconvert_phash(std::string &dstname, DEPOT *src)
{
  if (src && dpiterinit(src)) {
    char *key;
    int count = 0;
    
    while ((key = dpiternext(src, NULL))) {
      t_phash_data data;
      t_phash_data_old odata;
      int datasize = 0;
      
      datasize = dpgetwb(src, (const char *)key, (int)sizeof(Key), 0, (int)sizeof(t_phash_data_old), (char *)&odata);
      if (datasize == sizeof(t_phash_data_old)) {
        data.v = odata.v;
        data.t = odata.t;
        data.d = odata.d;
        data.m = odata.m;
        data.statV = odata.statV;
        data.kingD = odata.kingD;
      } else if (datasize == sizeof(t_phash_data)) {
        memcpy(&data, &odata, sizeof(t_phash_data));
      } else {
        sync_cout << "info Persistent Hash error converting " << dstname << " (records are incorrectly sized, database is probably invalid)." << sync_endl;
        return;
      }
      PHInst.store_phash(*((const Key *)key), data);
      count++;
      free(key);
    }
    sync_cout << "info Persistent Hash updated " << count << " records in " << dstname << " to new format." << sync_endl;
  }
}

void QDBM_PersistentHash::convert_phash(std::string &srcname)
{
  bool needsconvert = false;
  DEPOT *srcfile;
  
  srcfile = dpopen(srcname.c_str(), DP_OREADER, 0);
  if (srcfile) {
#if defined(USE_KYOTO)
    // we're converting to some other format -- it's a QDBM file, so yes, we need to convert
    needsconvert = 1;
#else
    needsconvert = needsconvert_phash(srcfile);
#endif
    dpclose(srcfile);
  }
  if (needsconvert) {
    std::string backupname = srcname + ".old";
    DEPOT *backupfile;
    struct stat st;

    if (!stat(backupname.c_str(), &st)) {
      // file exists
      std::time_t t = std::time(0);
      std::ostringstream oss;
      oss << std::hex << t;
      backupname = srcname + "_" + oss.str() + ".old";
    }
    
    rename(srcname.c_str(), backupname.c_str());
    backupfile = dpopen(backupname.c_str(),DP_OREADER, 0);
    if (backupfile) {
      PHInst.starttransaction_phash(PHASH_MODE_WRITE);
      doconvert_phash(srcname, backupfile);
      PHInst.endtransaction_phash();
      dpclose(backupfile);
    }
  }
}

DEPOT *QDBM_PersistentHash::open_phash(PHASH_MODE mode)
{
  bool usePersHash = Options["Use Persistent Hash"];
  std::string filename = Options["Persistent Hash File"];
  DEPOT *hash = NULL;
  
  if (usePersHash) {
    hash = dpopen(filename.c_str(), (mode == PHASH_MODE_WRITE) ? DP_OWRITER | DP_OCREAT : DP_OREADER, 0);
    if (mode == PHASH_MODE_WRITE) {
      dpsetalign(hash, sizeof(t_phash_data)); // optimizes overwrite operations
    }
  }
  return hash;
}

void QDBM_PersistentHash::close_phash(DEPOT *depot)
{
  dpclose(depot);
}

bool QDBM_PersistentHash::store_phash(const Key key, t_phash_data &data)
{
  Depth oldDepth = DEPTH_ZERO;
  bool isRoot;

  if (PersHashFile) {
    probe_phash(key, oldDepth, isRoot);
    if (DEPTH_IS_VALID(data.d, oldDepth)) {
      int rv;
      rv = dpput(PersHashFile, (const char *)&key, (int)sizeof(Key), (const char *)&data, (int)sizeof(t_phash_data), DP_DOVER);
#ifdef PHASH_DEBUG
      if (rv) {
        //printf("dpput: put %llx\n", key);
      } else {
        printf("dpput: %s\n", dperrmsg(dpecode));
      }
#endif
      return rv ? true : false;
    }
  }
  return false;
}

bool QDBM_PersistentHash::store_phash(const Key key, Value v, Bound t, Depth d, Move m, Value statV, Value kingD)
{
  Depth oldDepth = DEPTH_ZERO;
  bool isRoot;

  if (PersHashFile) {
    probe_phash(key, oldDepth, isRoot);
    if (DEPTH_IS_VALID(d, oldDepth) && !(isRoot && !(t & BOUND_ROOT))) {
      t_phash_data data;
      int rv;

      data.v = v;
      data.t = t;
      data.d = d;
      data.m = m;
      data.statV = statV;
      data.kingD = kingD;

      rv = dpput(PersHashFile, (const char *)&key, (int)sizeof(Key), (const char *)&data, (int)sizeof(t_phash_data), DP_DOVER);
#ifdef PHASH_DEBUG
      if (rv) {
        printf("dpput: put %llx\n", key);
      } else {
        printf("dpput: %s\n", dperrmsg(dpecode));
      }
#endif
      return rv ? true : false;
    }
  }
  return false;
}

void QDBM_PersistentHash::starttransaction_phash(PHASH_MODE mode)
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
  PersHashFile = open_phash(mode);
}

void QDBM_PersistentHash::endtransaction_phash()
{
  if (PersHashFile) {
    close_phash(PersHashFile);
    PersHashFile = NULL;
  }
}

int QDBM_PersistentHash::prune_below_phash(int depth)
{
  int count = 0;

  if (PersHashFile) {
    if (dpiterinit(PersHashFile)) {
      char *key;
      while ((key = dpiternext(PersHashFile, NULL))) {
        t_phash_data data;
        int datasize = 0;

        datasize = dpgetwb(PersHashFile, (const char *)key, (int)sizeof(Key), 0, (int)sizeof(t_phash_data), (char *)&data);
        if (datasize == sizeof(t_phash_data)) {
          if ((uint16_t)data.d <= depth) { // cast to unsigned to save DEPTH_NONE entries
            dpout(PersHashFile, (const char *)key, sizeof(Key));
            count++;
          }
        }
        free(key);
      }
    }
  }
  return count;
}


// the basic algorithm is: check the file size, if it's higher than the target size
// delete all entries at the minimum depth and optimize
// if still too big, repeat with the next highest depth and so on until we're below the target size
void QDBM_PersistentHash::doprune_phash()
{
  if (PersHashFile) {
    int desiredFileSize = Options["Persistent Hash Size"] * (1024 * 1024);
    int hashDepth = Options["Persistent Hash Depth"] * ONE_PLY;
    int pruneDepth = hashDepth;
    int currentFileSize;
    int totalPruned = 0;
    std::ostringstream ss;
    int origFileSize = dpfsiz(PersHashFile);
    
    optimize_phash();
    currentFileSize = dpfsiz(PersHashFile);
    if (currentFileSize < desiredFileSize) {
      sync_cout << "info string Persistent Hash optimized [no pruning necessary]. Previous size: " << origFileSize << " bytes; new size: " << currentFileSize << " bytes." << sync_endl;
      return;
    }
    while (pruneDepth < (100 * ONE_PLY)) {
      int pruned = prune_below_phash(pruneDepth);
      if (pruned) {
        optimize_phash();
        currentFileSize = dpfsiz(PersHashFile);
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
      pruneDepth += ONE_PLY;
    }
  }
}

void QDBM_PersistentHash::doclear_phash()
{
  if (PersHashFile) {
#ifdef PHASH_DEBUG
    int count = 0;
#endif
    int rv;
    
    if (dpiterinit(PersHashFile)) {
      char *key;
      
      while ((key = dpiternext(PersHashFile, NULL))) {
        rv = dpout(PersHashFile, (const char *)key, sizeof(Key));
#ifdef PHASH_DEBUG
        if (rv) {
          count++;
          printf("dpout: deleted %0llx\n", *((Key *)key));
        }
#else
        rv = rv; // compiler warning
#endif
        free(key);
      }
      optimize_phash();
    }
#ifdef PHASH_DEBUG
    printf("purged %d records\n", count);
#endif
  }
}

void QDBM_PersistentHash::domerge_phash()
{
  if (PersHashFile) {
    std::string mergename = Options["Persistent Hash Merge File"];
    int mindepth = Options["Persistent Hash Depth"] * ONE_PLY;
    DEPOT *mergefile;
    
    mergefile = dpopen(mergename.c_str(), DP_OREADER, 0);
    if (mergefile) {
      dpiterinit(mergefile);
      char *key;
      int merged = 0;
      int count = 0;
      
      while ((key = dpiternext(mergefile, NULL))) {
        t_phash_data data;
        int datasize = 0;
        
        datasize = dpgetwb(mergefile, (const char *)key, (int)sizeof(Key), 0, (int)sizeof(t_phash_data), (char *)&data);
        if (datasize == sizeof(t_phash_data)) {
          if (DEPTH_IS_VALID(data.d, mindepth)) {
            Depth depth;
            bool isRoot;
            probe_phash(*((const Key *)key), depth, isRoot);
            if ((uint16_t)data.d > depth && !(isRoot && !(data.t & BOUND_ROOT))) { // cast to unsigned to save DEPTH_NONE entries
              dpput(PersHashFile, (const char *)key, (int)sizeof(Key), (const char *)&data, (int)sizeof(t_phash_data), DP_DOVER);
              merged++;
            }
          }
        }
        count++;
        free(key);
      }
      dpclose(mergefile);
      
      sync_cout << "info string Persistent Hash merged " << merged << " records (from " << count << " total) from file " << mergename << "." << sync_endl;
    }
  }
}

Move QDBM_PersistentHash::probe_phash(const Key key, Depth &d, bool &isRoot)
{
  Move rv = MOVE_NONE;
  
  d = DEPTH_ZERO;
  isRoot = false;
  if (PersHashFile) {
    t_phash_data data;
    int datasize = 0;
    
    datasize = dpgetwb(PersHashFile, (const char *)&key, (int)sizeof(Key), 0, (int)sizeof(t_phash_data), (char *)&data);
    if (datasize == sizeof(t_phash_data)) {
      d = (Depth)data.d;
      isRoot = (data.t & BOUND_ROOT) ? true : false;
      rv = (Move)data.m;
    }
  }
  return rv;
}

void QDBM_PersistentHash::to_tt_phash()
{
  if (PersHashFile) {
#ifdef PHASH_DEBUG
    int count = 0;
#endif
    
    if (dpiterinit(PersHashFile)) {
      Key *key;
      
      while ((key = (Key *)dpiternext(PersHashFile, NULL))) {
        t_phash_data data;
        int datasize = 0;
        
        datasize = dpgetwb(PersHashFile, (const char *)key, (int)sizeof(Key), 0, (int)sizeof(t_phash_data), (char *)&data);
        if (datasize == sizeof(t_phash_data)) {
          TT.store(*((Key *)key), (Value)data.v, (Bound)data.t, (Depth)data.d, (Move)data.m, (Value)data.statV, (Value)data.kingD, false);
#ifdef PHASH_DEBUG
          printf("dpgetwb: pull %llx\n", *((Key *)key));
          count++;
#endif
        }
        free(key);
      }
    }
#ifdef PHASH_DEBUG
    printf("restored %d records\n", count);
#endif
  }
}

void QDBM_PersistentHash::optimize_phash()
{
  if (PersHashFile) {
    dpoptimize(PersHashFile, 0);
  }
}

int QDBM_PersistentHash::count_phash()
{
  int count = 0;

  if (PersHashFile) {
    if (dpiterinit(PersHashFile)) {
      char *key;
      
      while ((key = dpiternext(PersHashFile, NULL))) count++;
    }
#ifdef PHASH_DEBUG
    printf("counted %d records\n", count);
#endif
  }
  return count;
}
