//
//  phash.h
//  stockfish
//
//  Created by Jeremy Bernstein on 22.05.13.
//  Copyright (c) 2013 stockfishchess. All rights reserved.
//

#if !defined(PHASH_H_INCLUDED)
#define PHASH_H_INCLUDED

#include "position.h"
#include "ucioption.h"
#include <sstream>

//#define PHASH_DEBUG
//#define EPD_DEBUG

#define USE_KYOTO

#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif

typedef struct _phash_data
{
  int16_t   v;
  uint8_t   t;
  uint16_t  d;
  uint16_t  m;
  int16_t   statV;
  int16_t   kingD;
} t_phash_data;

typedef enum { PHASH_MODE_READ, PHASH_MODE_WRITE } PHASH_MODE;
typedef enum { PHASH_BACKEND_QDBM, PHASH_BACKEND_KYOTO } PHASH_BACKEND;

class PersistentHash {

public:
#if defined(USE_KYOTO)
  static PersistentHash &getInstance(PHASH_BACKEND backend = PHASH_BACKEND_KYOTO);
#else
  static PersistentHash &getInstance(PHASH_BACKEND backend = PHASH_BACKEND_QDBM);
#endif

  static void import_epd(std::istringstream& is);
  static void exercise(std::istringstream& is);

  virtual void init_phash() = 0;
  virtual void quit_phash() = 0;
  virtual bool store_phash(const Key key, Value v, Bound t, Depth d, Move m, Value statV, Value kingD) = 0;
  virtual bool store_phash(const Key key, t_phash_data &data) = 0;
  virtual Move probe_phash(const Key key, Depth &d, bool &isRoot) = 0;
  virtual void starttransaction_phash(PHASH_MODE mode) = 0;
  virtual void endtransaction_phash() = 0;
  virtual void to_tt_phash() = 0;
  virtual void wantsclear_phash() = 0;
  virtual void wantsprune_phash() = 0;
  virtual void wantsmerge_phash() = 0;
};

#define PHInst PersistentHash::getInstance()

template<class T>
std::string t_to_string(T i)
{
  std::stringstream ss;
  std::string s;
  ss << i;
  s = ss.str();
  return s;
}

#endif /* defined(PHASH_H_INCLUDED) */
