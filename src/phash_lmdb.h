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

class LMDB_PersistentHash : public PersistentHash
{
  
public:
  LMDB_PersistentHash();
  
  virtual void init_phash();
  virtual void quit_phash();
  virtual void store_phash(const Key key, Value v, Bound t, Depth d, Move m, Value statV, Value kingD);
  virtual int probe_phash(const Key key, Depth *d);
  virtual void starttransaction_phash(PHASH_MODE mode);
  virtual void endtransaction_phash();
  virtual void to_tt_phash();
  virtual void wantsclear_phash();
  virtual void wantsprune_phash();
  virtual void wantsmerge_phash();
  
private:
  int count_phash();
  MDB_env *open_phash(PHASH_MODE mode);
  void close_phash(MDB_env *depot);
  void clear_phash();
  void doclear_phash();
  void prune_phash();
  void doprune_phash();
  void merge_phash();
  void domerge_phash();
  void optimize_phash();
  int getsize_phash();
  int prune_below_phash(int depth);

  void copyfile(std::string &srcfile, std::string &dstfile);

  bool needsconvert_phash(MDB_env *depot);
  void doconvert_phash(MDB_env *dst, MDB_env *src);
  void convert_phash(std::string &srcname);
  
  MDB_env *PersHashEnv;
  MDB_txn *PersHashTxn;
  MDB_dbi PersHashDbi;

  bool PersHashWantsClear;
  bool PersHashWantsPrune;
  bool PersHashWantsMerge;
};

extern LMDB_PersistentHash LMDB;

#endif
