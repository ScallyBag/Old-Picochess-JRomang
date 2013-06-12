//
//  qdbm.h
//  stockfish
//
//  Created by Jeremy Bernstein on 07.06.13.
//  Copyright (c) 2013 stockfishchess. All rights reserved.
//

#ifndef stockfish_kyoto_h
#define stockfish_kyoto_h

#include <cassert>
#include <string>

#include "phash.h"
#include "kyotocabinet/kchashdb.h"
#include "misc.h"
#include "thread.h"
#include "tt.h"

using namespace kyotocabinet;

class KYOTO_PersistentHash : public PersistentHash
{
  
public:
  KYOTO_PersistentHash();

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
  HashDB *open_phash(PHASH_MODE mode);
  bool dostore_phash(const Key key, t_phash_data &data);
  void close_phash(HashDB *depot);
  void clear_phash();
  void doclear_phash();
  void prune_phash();
  void doprune_phash();
  void merge_phash();
  void domerge_phash();
  void optimize_phash();
  size_t getsize_phash();
  int prune_below_phash(int depth);
  
  HashDB *PersHashFile;
  bool PersHashWantsClear;
  bool PersHashWantsPrune;
  bool PersHashWantsMerge;
};

extern KYOTO_PersistentHash KYOTO;

#endif
