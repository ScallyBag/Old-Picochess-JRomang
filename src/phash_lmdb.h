//
//  phash_lmdb.h
//  stockfish
//
//  Created by Jeremy Bernstein on 07.06.13.
//  Copyright (c) 2013 stockfishchess. All rights reserved.
//

#ifndef stockfish_phash_lmdb_h
#define stockfish_phash_lmdb_h

#include <cassert>
#include <string>

#include "phash.h"
#include "lmdb/lmdb.h"
#include "misc.h"
#include "thread.h"
#include "tt.h"

struct LMDB_PHFile
{
  static LMDB_PHFile *open(std::string &fname, int mode = PHASH_MODE_WRITE);

  LMDB_PHFile(std::string &filename, int mode = PHASH_MODE_WRITE);
  ~LMDB_PHFile();

  void close(bool commit = true);
  int put(MDB_val *vKey, MDB_val *vData, int flags = 0);
  int get(MDB_val *vKey, MDB_val *vData);
  int clear();

  MDB_env *env()          { return e; }
  MDB_txn *txn()          { return t; }
  MDB_dbi  dbi()          { return d; }
  unsigned txct()         { return tc; }
  std::string &filename() { return fn; }
  size_t maxpages()       { return mp; }
  MDB_cursor *cursor();
  size_t numentries();
  void stat(MDB_stat *st);

private:
  void fixmapsize_phash();

  MDB_env *e;
  MDB_txn *t;
  MDB_dbi d;
  std::string fn;
  unsigned tc;
  size_t mp;
};

class LMDB_PersistentHash : public PersistentHash
{
  
public:
  LMDB_PersistentHash();
  
  virtual void init_phash();
  virtual void quit_phash();
  virtual bool store_phash(const Key key, t_phash_data &data);
  virtual bool store_phash(const Key key, Value v, Bound t, Depth d, Move m, Value statV, Value kingD);
  virtual int probe_phash(const Key key, Depth *d);
  virtual void starttransaction_phash(PHASH_MODE mode);
  virtual void endtransaction_phash();
  virtual void to_tt_phash();
  virtual void wantsclear_phash();
  virtual void wantsprune_phash();
  virtual void wantsmerge_phash();
  
private:
  int count_phash();
  void clear_phash();
  void doclear_phash();
  void prune_phash();
  void doprune_phash();
  void merge_phash();
  void domerge_phash();
  void optimize_phash();
  int getsize_phash();
  int prune_below_phash(int depth);
  int put_withprune(MDB_val *vKey, MDB_val *vData);
  bool dostore_phash(const Key key, t_phash_data &data);
  bool commit_and_rebuild(bool commit, bool optimize = false);

  void copyfile(std::string &srcfile, std::string &dstfile);

  LMDB_PHFile *PersHashFile;
  
  std::string PersHashFileName;
  std::string PersHashPrunefileName;

  bool PersHashWantsClear;
  bool PersHashWantsPrune;
  bool PersHashWantsMerge;
};

extern LMDB_PersistentHash LMDB;

#endif
