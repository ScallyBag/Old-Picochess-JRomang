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
#ifdef USE_EGTB
#include <cassert>
#include <string>

#include "egtb.h"
#include "egtb/gtb-probe.h"
#include "rkiss.h"
#include "misc.h"
#include "movegen.h"
#include "position.h"
#include "ucioption.h"

static RKISS rk;
int MaxEgtbPieces = 0;
////
//// Local definitions
////

namespace
{
  /// Variables
  std::string TbPaths;
  int TbSize = 0;
  
  const char **paths;
  
  int CompressionScheme;
  
  bool Chess960;
  
  /// Local functions
  std::string trim(const std::string& str);
  int get_compression_scheme_from_string(const std::string& str);
}

////
//// Functions
////

// init_egtb() initializes or reinitializes gaviota tablebases if necessary.

void init_egtb()
{
  bool useTbs = Options["UseGaviotaTb"];
  std::string newTbPaths = Options["GaviotaTbPath"];
  int newTbSize = Options["GaviotaTbCache"];
  int newCompressionScheme = get_compression_scheme_from_string(Options["GaviotaTbCompression"]);
  Chess960 = Options["UCI_Chess960"];
  unsigned int tb_available_bits = 0;
  
  // If we don't use the tablebases, close them out (in case previously open).
  if (!useTbs) {
    close_egtb();
    return;
  }
  
  // Check if we need to initialize or reinitialize.
  if (newTbSize != TbSize || newTbPaths != TbPaths || newCompressionScheme != CompressionScheme)
  {
    // Close egtbs before reinitializing.
    close_egtb();
    
    TbSize = newTbSize;
    TbPaths = newTbPaths;
    CompressionScheme = newCompressionScheme;
    
    // Parse TbPaths string which can contain many paths separated by ';'
    paths = tbpaths_init();
    
    std::string substr;
    size_t prev_pos = 0, pos = 0;
    
    while( (pos = TbPaths.find(';', prev_pos)) != std::string::npos )
    {
      if ((substr = trim(TbPaths.substr(prev_pos, pos-prev_pos))) != "")
        paths = tbpaths_add(paths, substr.c_str());
      
      prev_pos = pos + 1;
    }
    
    if (prev_pos < TbPaths.size() && (substr = trim(TbPaths.substr(prev_pos))) != "")
      paths = tbpaths_add(paths, substr.c_str());
    
    //Finally initialize tablebases
    tb_init(0, CompressionScheme, paths);
    if (tb_is_initialized()) {
      std::ostringstream status;
      
      tb_available_bits = tb_availability();
      if (tb_available_bits) {
        status << "info string GTB successfully initialized: ";
        // 3-man
        if (tb_available_bits & 0x3) {
          status << "3-man";
          if (!(tb_available_bits & 0x2)) {
            status << " (partial)";
          }
          status << ", ";
          MaxEgtbPieces = 3;
        }
        // 4-man
        if (tb_available_bits & 0xC) {
          status << "4-man";
          if (!(tb_available_bits & 0x8)) {
            status << " (partial)";
          }
          status << ", ";
          MaxEgtbPieces = 4;
        }
        // 5-man
        if (tb_available_bits & 0x30) {
          status << "5-man";
          if (!(tb_available_bits & 0x20)) {
            status << " (partial)";
          }
          MaxEgtbPieces = 5;
        }
#if 0
        // 6-man, not yet available
        if (tb_available_bits & 0xC0) {
          status << ", 6-man";
          if (!(tb_available_bits & 0x80)) {
            status << " (partial)";
          }
          MaxEgtbPieces = 6;
        }
#endif
        status << "; cache: " << TbSize << "MB";
        tbcache_init(TbSize * 1024 * 1024, 124);
      }
      else {
        status << "info string GTB could not be initialized";
        close_egtb();
      }
      sync_cout << status.str() << sync_endl;
    }
    tbstats_reset();
  }
}

// close_egtb() closes/frees tablebases if necessary
void close_egtb()
{
  if (tbcache_is_on()) {
    tbcache_done();
    TbSize = 0;
  }
  if (tb_is_initialized()) {
    tb_done();
    paths = tbpaths_done(paths);
  }
}

// probe_egtb() does the actual probing. On failure it returns VALUE_NONE.
Value probe_egtb(Position &pos, const bool hard, const bool exact)
{
  // Conversion variables
  Bitboard occ;
  int count;
  
  // stockfish -> egtb
  int stm, epsquare, castling;
  unsigned int  ws[17], bs[17];
  unsigned char wp[17], bp[17];
  
  // egtb -> stockfish
  int tb_available;
  unsigned info = tb_UNKNOWN;
  unsigned pliestomate;
  
  // Prepare info for white (stockfish -> egtb)
  occ = pos.pieces(WHITE);
  count = 0;
  while (occ)
  {
    Square s = pop_lsb(&occ);
    ws[count] = s;
    wp[count] = (unsigned char) type_of(pos.piece_on(s));
    count++;
  }
  ws[count] = tb_NOSQUARE;
  wp[count] = tb_NOPIECE;
  
  // Prepare info for black (stockfish -> egtb)
  occ = pos.pieces(BLACK);
  count = 0;
  while (occ)
  {
    Square s = pop_lsb(&occ);
    bs[count] = s;
    bp[count] = (unsigned char) type_of(pos.piece_on(s));
    count++;
  }
  bs[count] = tb_NOSQUARE;
  bp[count] = tb_NOPIECE;
  
  // Prepare general info
  stm      = pos.side_to_move();
  epsquare = pos.ep_square();
  castling = tb_NOCASTLE;
  
  if (pos.can_castle(WHITE) || pos.can_castle(BLACK))
  {
    if (Chess960)
      return VALUE_NONE;
    
    if (pos.can_castle(WHITE_OO))
      castling |= tb_WOO;
    if (pos.can_castle(WHITE_OOO))
      castling |= tb_WOOO;
    if (pos.can_castle(BLACK_OO))
      castling |= tb_BOO;
    if (pos.can_castle(BLACK_OOO))
      castling |= tb_BOOO;
  }
  
  // Do the actual probing
  if (hard)
  {
    if (exact)
      tb_available = tb_probe_hard (stm, epsquare, castling, ws, bs, wp, bp, &info, &pliestomate);
    else
      tb_available = tb_probe_WDL_hard (stm, epsquare, castling, ws, bs, wp, bp, &info);
  }
  else
  {
    if (exact)
      tb_available = tb_probe_soft (stm, epsquare, castling, ws, bs, wp, bp, &info, &pliestomate);
    else
      tb_available = tb_probe_WDL_soft (stm, epsquare, castling, ws, bs, wp, bp, &info);
  }
  
  // Return probing info (if available)
  if (tb_available)
  {
    pos.set_tb_hits(pos.tb_hits() + 1);
    if (info == tb_DRAW)
      return VALUE_DRAW;
    else if (info == tb_WMATE && stm == tb_WHITE_TO_MOVE)
      return (exact ? mate_in(pliestomate) : VALUE_KNOWN_WIN);
    else if (info == tb_BMATE && stm == tb_BLACK_TO_MOVE)
      return (exact ? mate_in(pliestomate) : VALUE_KNOWN_WIN);
    else if (info == tb_WMATE && stm == tb_BLACK_TO_MOVE)
      return (exact ? mated_in(pliestomate) : -VALUE_KNOWN_WIN);
    else if (info == tb_BMATE && stm == tb_WHITE_TO_MOVE)
      return (exact ? mated_in(pliestomate) : -VALUE_KNOWN_WIN);
  }
  return VALUE_NONE;
}

// note that these are cribbed from the strategy employed by Ronald de Man
// in his syzygy tablebase probing code from https://github.com/syzygy1/tb
Value egtb_probe_root(Position &pos, Move *return_move, int *success)
{
  Value value;
  
  *success = 1;
  value = probe_egtb(pos, PROBE_HARD, PROBE_EXACT); // always probe hard; the root probe is exact
  if (value == VALUE_NONE) {
    *success = 0;
    return VALUE_NONE;
  }
  
  
  MoveStack stack[192];
  MoveStack *moves, *end;
  StateInfo st;
  
  // Generate at least all legal moves.
  if (!pos.checkers())
    end = generate<NON_EVASIONS>(pos, stack);
  else
    end = generate<EVASIONS>(pos, stack);
  
  CheckInfo ci(pos);
  int num_best = 0;
  int best = -VALUE_INFINITE;
  int best2 = 0;
  int v, w;
  for (moves = stack; moves < end; moves++) {
    Move move = moves->move;
    if (!pos.pl_move_is_legal(move, ci.pinned))
      continue;
    pos.do_move(move, st, ci, pos.move_gives_check(move, ci));
    v = w = 0;
    if (pos.checkers()) {
      MoveStack s[192];
      if (generate<LEGAL>(pos, s) == s)
        v = VALUE_MATE;
    }
    if (!v) {
      v = -probe_egtb(pos, PROBE_HARD, PROBE_EXACT);
    }
    pos.undo_move(move);

    if (v == VALUE_NONE) return VALUE_NONE;
    
    if (v > best || (v == best && w > best2)) {
      stack[0].move = move;
      best = v;
      best2 = w;
      num_best = 1;
    } else if (v == best && w == best2) {
      stack[num_best++].move = move;
    }
  }
  if (!num_best) {
    *return_move = stack[0].move;
  } else {
    *return_move = stack[rk.rand<unsigned>() % num_best].move;
  }
  return value;
}

Value egtb_probe(Position &pos, const bool hard, const bool exact, int *success)
{
  Value value;

  *success = 0;
  value = probe_egtb(pos, hard ? PROBE_HARD : PROBE_SOFT, exact ? PROBE_EXACT : PROBE_WDL);
  if (value != VALUE_NONE) {
    *success = 1;
    return value;
  }
  return VALUE_NONE;
}


namespace
{
  // trim() removes leading and trailing spaces (and like) from string
  
  std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if(start == std::string::npos) return "";
    return str.substr(start, str.find_last_not_of(" \t\n\r") - start + 1);
  }
  
  int get_compression_scheme_from_string(const std::string& str) {
    if (str == "Uncompressed")
      return tb_UNCOMPRESSED;
    else if (str == "Huffman (cp1)")
      return tb_CP1;
    else if (str == "LZF (cp2)")
      return tb_CP2;
    else if (str == "Zlib-9 (cp3)")
      return tb_CP3;
    else
      return tb_CP4;
  }
}

#endif
