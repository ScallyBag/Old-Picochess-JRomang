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

//#define PHASH_DEBUG

#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif

typedef enum { PHASH_READ, PHASH_WRITE } PHASH_MODE;

void init_phash();
void quit_phash();
void store_phash(const Key key, Value v, Bound t, Depth d, Move m, Value statV, Value kingD);
int probe_phash(const Key key, Depth *d);
void starttransaction_phash(PHASH_MODE mode);
void endtransaction_phash();
void to_tt_phash();
void wantsclear_phash();
void wantsprune_phash();
void wantsmerge_phash();

#endif /* defined(ISAM_H_INCLUDED) */
