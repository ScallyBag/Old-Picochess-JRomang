//
//  qdbm.h
//  stockfish
//
//  Created by Jeremy Bernstein on 07.06.13.
//  Copyright (c) 2013 stockfishchess. All rights reserved.
//

#ifndef stockfish_qdbm_h
#define stockfish_qdbm_h

#include <cassert>
#include <string>

#include "phash.h"
#include "qdbm/depot.h"
#include "misc.h"
#include "thread.h"
#include "tt.h"

class QDBM_PersistentHash : public PersistentHash
{
  
public:
  QDBM_PersistentHash();

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
  DEPOT *open_phash(PHASH_MODE mode);
  void close_phash(DEPOT *depot);
  void clear_phash();
  void doclear_phash();
  void prune_phash();
  void doprune_phash();
  void merge_phash();
  void domerge_phash();
  void optimize_phash();
  int getsize_phash();
  int prune_below_phash(int depth);
  void convert_phash(std::string &filename);
  bool needsconvert_phash(DEPOT *depot);
  void doconvert_phash(DEPOT *dst, DEPOT *src);
  
  DEPOT *PersHashFile;
  bool PersHashWantsClear;
  bool PersHashWantsPrune;
  bool PersHashWantsMerge;
};

extern QDBM_PersistentHash QDBM;

#endif
