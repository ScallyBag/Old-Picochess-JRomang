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
#include "phash_kyoto.h"
#include "notation.h"

PersistentHash &PersistentHash::getInstance(PHASH_BACKEND backend)
{
#if defined(USE_KYOTO)
  if (backend != PHASH_BACKEND_QDBM)
    return KYOTO;
#endif
  return QDBM;
}

#define wantsToken(token) (token == "bm" || token == "ce" || token == "acd")

// import_epd() imports the positions and best moves from an epd file to
// the persistent hash.

void PersistentHash::import_epd(std::istringstream& is) {
  
  std::string token, filename, noce;
  bool useHash = Options["Use Persistent Hash"]; // cache existing vlaue
  unsigned count = 0;
  unsigned total = 0;

  Options["Use Persistent Hash"] = t_to_string("true"); // has to be true for the PH transaction to work

  PHInst.starttransaction_phash(PHASH_MODE_WRITE);

  // importepd <filename> [noce]
  is >> filename;
  is >> noce;
  std::ifstream infile(filename.c_str());
  std::string line;
  while (std::getline(infile, line)) {
    Position pos;
    std::istringstream iss(line);
    std::string fen;
    std::string field;
    Value v = VALUE_NONE;
    Move m = MOVE_NONE;
    Depth d = (Depth)(int)Options["Persistent Hash Depth"];
    int fensize = 0;
    bool validfen = true;
    size_t startpos = 0;

    while (iss >> token && !wantsToken(token)) {
      fen += token + " "; // we might have some extra here
      if (!fensize) {
        // verify that the initial FEN block only contains valid chars
        if (fen.find_first_not_of("PNBRQKpnbrqk12345678/ ") != std::string::npos) {
          validfen = false; // invalid
          break;
        }
        // verify that there are 7 '/' seperators in the initial FEN block
        if (std::count(fen.begin(), fen.end(), '/') != 7) {
          validfen = false; // invalid
          break;
        }
        // verify that each rank of the initial FEN block specifies 8 squares
        while (fen.find_first_of("/ ", startpos) != std::string::npos) {
          int blockcount = 0;
          while (1) {
            char c = fen.at(startpos++);

            if (c >= '1' && c <= '8') blockcount += c - '0';
            else if (c == '/' || c == ' ') {
              if (blockcount != 8) {
                validfen = false;
              }
              break;
            }
            else blockcount++;
          }
          if (!validfen) break;
        }
        if (!validfen) break;
      }
      fensize++;
    }
    if (!validfen) continue;
    if (fensize < 6) {
      switch (fensize) {
        case 5:
          fen += "1";
          break;
        case 4:
          fen += "0 1";
          break;
        case 3:
          fen += "- 0 1";
          break;
        default:
          continue; // invalid
      }
    }
    pos.set(fen, Options["UCI_Chess960"], Threads.main_thread());
#ifdef EPD_DEBUG
    sync_cout << pos.fen() << ": " << pos.key() << sync_endl;
#endif
    do {
      field = "";
      do {
        if (token.at(token.size()-1) == ';') {
          token.erase(token.size()-1);
          field += token;
          break;
        } else {
          field += token + " ";
        }
      } while (iss >> token);
      if (!field.empty()) {
        std::istringstream fss(field);
        fss >> token;
        if (wantsToken(token)) {
          std::string name = token;
          fss >> token;
          if (name == "bm") {             // best move
            m = san_to_move(pos, token);
          } else if (name == "ce") {      // centipawn evaluation
            v = uci_to_score(token);
          } else if (name == "acd") {     // analysis count depth
            d = (Depth)::atoi(token.c_str());
          }
#ifdef EPD_DEBUG
          sync_cout << name << ": " << token << sync_endl;
#endif
        }
      }
    } while (iss >> token);
    if (v == VALUE_NONE && noce == "noce") {
      v = pos.side_to_move() == WHITE ? Value(VALUE_KNOWN_WIN) : Value(-VALUE_KNOWN_WIN);
    }
    if (v != VALUE_NONE && m != MOVE_NONE) {
      if (PHInst.store_phash(pos.key(), v, Bound((int)BOUND_EXACT | BOUND_ROOT), d, m, v, VALUE_ZERO)) {
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
  int rndepth = 1;
  int hashdepth = Options["Persistent Hash Depth"];

  is >> iterations;
  is >> rndepth;
  memset(&data, 0, sizeof(t_phash_data));
  PHInst.starttransaction_phash(PHASH_MODE_WRITE);
  for (int i = 0; i < iterations; i++) {
    Key r30 = RAND_MAX*rand()+rand();
    Key s30 = RAND_MAX*rand()+rand();
    Key t4  = rand() & 0xf;
    Key res = (r30 << 34) + (s30 << 4) + t4;
    data.d = rndepth ? rand() % 30 + hashdepth : 25;
    PHInst.store_phash((const Key)res, data);
  }
  PHInst.endtransaction_phash();
  sync_cout << "exercise done (" << iterations << " records)." << sync_endl;
}
