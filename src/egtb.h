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

#ifdef USE_EGTB
#if !defined(EGTB_H_INCLUDED)
#define EGTB_H_INCLUDED

////
//// Includes
////

#include "position.h"

////
//// Constants and variables
////
extern int MaxEgtbPieces;

enum {
  PROBE_SOFT = 0,
  PROBE_HARD = 1
};
enum {
  PROBE_WDL = 0,
  PROBE_EXACT = 1
};

////
//// Prototypes
////

void init_egtb();
void close_egtb();

Value egtb_probe_root(Position &pos, Move *return_move, int *success);
Value egtb_probe(Position &pos, const bool hard, const bool exact, int *success);

#endif
#endif