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
#include "phash_lmdb.h"
#include "notation.h"

PersistentHash &PersistentHash::getInstance(PHASH_BACKEND backend)
{
  if (backend == PHASH_BACKEND_LMDB) {
    return LMDB;
  } else {
    return QDBM;
  }
}

#define wantsToken(token) (token == "bm" || token == "ce" || token == "acd")

// import_epd() imports the positions and best moves from an epd file to
// the persistent hash.

void PersistentHash::import_epd(std::istringstream& is) {
  
  std::string token, filename;
  bool useHash = Options["Use Persistent Hash"]; // cache existing vlaue
  unsigned count = 0;
  unsigned total = 0;

  Options["Use Persistent Hash"] = t_to_string("true"); // has to be true for the PH transaction to work

  PHInst.starttransaction_phash(PHASH_MODE_WRITE);

  is >> filename; // importepd <filename>
  std::ifstream infile(filename.c_str());
  std::string line;
  while (std::getline(infile, line)) {
    Position pos;
    std::istringstream iss(line);
    std::string fen;
    Value v = VALUE_NONE;
    Move m = MOVE_NONE;
    Depth d = (Depth)(int)Options["Persistent Hash Depth"];

    while (iss >> token && !wantsToken(token))
      fen += token + " "; // we might have some extra here
    pos.set(fen, Options["UCI_Chess960"], Threads.main_thread());
#ifdef EPD_DEBUG
    sync_cout << pos.fen() << ": " << pos.key() << sync_endl;
#endif
    do {
      if (wantsToken(token)) {
        std::string name = token;
        iss >> token;
        if (token.at(token.size()-1) == ';')
          token.erase(token.size()-1);
        if (name == "bm") {             // best move
          m = san_to_move(pos, token);
        } else if (name == "ce") {      // centipawn evaluation
          v = uci_to_score(token);
        } else if (name == "acd") {     // analysis count depth
          d = (Depth)atoi(token.c_str());
        }
#ifdef EPD_DEBUG
        sync_cout << name << ": " << token << sync_endl;
#endif
      }
    } while (iss >> token && !wantsToken(token));
    if (v == VALUE_NONE) {
      v = pos.side_to_move() == WHITE ? Value(VALUE_KNOWN_WIN) : Value(-VALUE_KNOWN_WIN); // is this reasonable?
    }
    if (v != VALUE_NONE && m != MOVE_NONE) {
      if (PHInst.store_phash(pos.key(), v, BOUND_EXACT, d, m, v, VALUE_ZERO)) {
        count++;
      }
    }
    total++;
  }
  PHInst.endtransaction_phash();
  sync_cout << "info string Persistent Hash imported " << count << " records from " << filename << " (from " << total << " total)." << sync_endl;
  // restore previous value
  Options["Use Persistent Hash"] = t_to_string(useHash ? "true" : "false");
}

void PersistentHash::exercise(std::istringstream& is)
{
  t_phash_data data;
  int iterations;

  is >> iterations;
  memset(&data, 0, sizeof(t_phash_data));
  data.d = 25;
  PHInst.starttransaction_phash(PHASH_MODE_WRITE);
  for (int i = 0; i < iterations; i++) {
    Key r30 = RAND_MAX*rand()+rand();
    Key s30 = RAND_MAX*rand()+rand();
    Key t4  = rand() & 0xf;
    Key res = (r30 << 34) + (s30 << 4) + t4;
    PHInst.store_phash((const Key)res, data);
  }
  PHInst.endtransaction_phash();
}
