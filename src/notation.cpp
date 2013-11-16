/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2013 Marco Costalba, Joona Kiiski, Tord Romstad

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

#include <cassert>
#include <iomanip>
#include <sstream>
#include <stack>

#include "movegen.h"
#include "notation.h"
#include "position.h"

using namespace std;

static const char* PieceToChar[COLOR_NB] = { " PNBRQK", " pnbrqk" };


/// score_to_uci() converts a value to a string suitable for use with the UCI
/// protocol specifications:
///
/// cp <x>     The score from the engine's point of view in centipawns.
/// mate <y>   Mate in y moves, not plies. If the engine is getting mated
///            use negative values for y.

string score_to_uci(Value v, Value alpha, Value beta) {

  stringstream s;

  if (abs(v) < VALUE_MATE_IN_MAX_PLY)
      s << "cp " << v * 100 / int(PawnValueMg);
  else
      s << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;

  s << (v >= beta ? " lowerbound" : v <= alpha ? " upperbound" : "");

  return s.str();
}


/// move_to_uci() converts a move to a string in coordinate notation
/// (g1f3, a7a8q, etc.). The only special case is castling moves, where we print
/// in the e1g1 notation in normal chess mode, and in e1h1 notation in chess960
/// mode. Internally castle moves are always coded as "king captures rook".

const string move_to_uci(Move m, bool chess960) {

  Square from = from_sq(m);
  Square to = to_sq(m);

  if (m == MOVE_NONE)
      return "(none)";

  if (m == MOVE_NULL)
      return "0000";

  if (type_of(m) == CASTLE && !chess960)
      to = (to > from ? FILE_G : FILE_C) | rank_of(from);

  string move = square_to_string(from) + square_to_string(to);

  if (type_of(m) == PROMOTION)
      move += PieceToChar[BLACK][promotion_type(m)]; // Lower case

  return move;
}


/// move_from_uci() takes a position and a string representing a move in
/// simple coordinate notation and returns an equivalent legal Move if any.

Move move_from_uci(const Position& pos, string& str) {

  if (str.length() == 5) // Junior could send promotion piece in uppercase
      str[4] = char(tolower(str[4]));

  for (MoveList<LEGAL> it(pos); *it; ++it)
      if (str == move_to_uci(*it, pos.is_chess960()))
          return *it;

  return MOVE_NONE;
}


/// move_to_san() takes a position and a legal Move as input and returns its
/// short algebraic notation representation.

const string move_to_san(Position& pos, Move m) {

  if (m == MOVE_NONE)
      return "(none)";

  if (m == MOVE_NULL)
      return "(null)";

  assert(MoveList<LEGAL>(pos).contains(m));

  Bitboard others, b;
  string san;
  Color us = pos.side_to_move();
  Square from = from_sq(m);
  Square to = to_sq(m);
  Piece pc = pos.piece_on(from);
  PieceType pt = type_of(pc);

  if (type_of(m) == CASTLE)
      san = to > from ? "O-O" : "O-O-O";
  else
  {
      if (pt != PAWN)
      {
          san = PieceToChar[WHITE][pt]; // Upper case

          // Disambiguation if we have more then one piece of type 'pt' that can
          // reach 'to' with a legal move.
          others = b = (pos.attacks_from(pc, to) & pos.pieces(us, pt)) ^ from;

          while (b)
          {
              Move move = make_move(pop_lsb(&b), to);
              if (!pos.legal(move, pos.pinned_pieces(pos.side_to_move())))
                  others ^= from_sq(move);
          }

          if (others)
          {
              if (!(others & file_bb(from)))
                  san += file_to_char(file_of(from));

              else if (!(others & rank_bb(from)))
                  san += rank_to_char(rank_of(from));

              else
                  san += square_to_string(from);
          }
      }
      else if (pos.capture(m))
          san = file_to_char(file_of(from));

      if (pos.capture(m))
          san += 'x';

      san += square_to_string(to);

      if (type_of(m) == PROMOTION)
          san += string("=") + PieceToChar[WHITE][promotion_type(m)];
  }

  if (pos.gives_check(m, CheckInfo(pos)))
  {
      StateInfo st;
      pos.do_move(m, st);
      san += MoveList<LEGAL>(pos).size() ? "+" : "#";
      pos.undo_move(m);
  }

  return san;
}


/// pretty_pv() formats human-readable search information, typically to be
/// appended to the search log file. It uses the two helpers below to pretty
/// format time and score respectively.

static string time_to_string(int64_t msecs) {

  const int MSecMinute = 1000 * 60;
  const int MSecHour   = 1000 * 60 * 60;

  int64_t hours   =   msecs / MSecHour;
  int64_t minutes =  (msecs % MSecHour) / MSecMinute;
  int64_t seconds = ((msecs % MSecHour) % MSecMinute) / 1000;

  stringstream s;

  if (hours)
      s << hours << ':';

  s << setfill('0') << setw(2) << minutes << ':' << setw(2) << seconds;

  return s.str();
}

static string score_to_string(Value v) {

  stringstream s;

  if (v >= VALUE_MATE_IN_MAX_PLY)
      s << "#" << (VALUE_MATE - v + 1) / 2;

  else if (v <= VALUE_MATED_IN_MAX_PLY)
      s << "-#" << (VALUE_MATE + v) / 2;

  else
      s << setprecision(2) << fixed << showpos << double(v) / PawnValueMg;

  return s.str();
}

string pretty_pv(Position& pos, int depth, Value value, int64_t msecs, Move pv[]) {

  const int64_t K = 1000;
  const int64_t M = 1000000;

  std::stack<StateInfo> st;
  Move* m = pv;
  string san, padding;
  size_t length;
  stringstream s;

  s << setw(2) << depth
    << setw(8) << score_to_string(value)
    << setw(8) << time_to_string(msecs);

  if (pos.nodes_searched() < M)
      s << setw(8) << pos.nodes_searched() / 1 << "  ";

  else if (pos.nodes_searched() < K * M)
      s << setw(7) << pos.nodes_searched() / K << "K  ";

  else
      s << setw(7) << pos.nodes_searched() / M << "M  ";

  padding = string(s.str().length(), ' ');
  length = padding.length();

  while (*m != MOVE_NONE)
  {
      san = move_to_san(pos, *m);

      if (length + san.length() > 80)
      {
          s << "\n" + padding;
          length = padding.length();
      }

      s << san << ' ';
      length += san.length() + 1;

      st.push(StateInfo());
      pos.do_move(*m++, st.top());
  }

  while (m != pv)
      pos.undo_move(*--m);

  return s.str();
}

#ifdef PA_GTB
Value uci_to_score(std::string &str)
{
  Value uci = (Value)atoi(str.c_str());
  Value v = VALUE_NONE;

  if (uci > 32000) {
    v = VALUE_MATE - (32767 - uci);
  } else if (uci < -32000) {
    v = -VALUE_MATE + (32767 + uci);
  } else {
    v = uci * int(PawnValueMg) / 100;
  }
  return v;
}

#include "misc.h"

//#define SAN_DEBUG

enum { SAN_MOVE_NORMAL, SAN_PAWN_CAPTURE };

template <int MoveType> inline Move test_move(Position &pos, Square fromsquare, Square tosquare, PieceType promotion)
{
  Move move;

  if (MoveType == SAN_MOVE_NORMAL) {
    if (promotion != NO_PIECE_TYPE) {
      move = make<PROMOTION>(fromsquare, tosquare, promotion);
    } else {
      move = make<NORMAL>(fromsquare, tosquare);
    }
  } else if (MoveType == SAN_PAWN_CAPTURE) {
    if (pos.ep_square() == tosquare) {
      move = make<ENPASSANT>(fromsquare, tosquare);
    } else {
      if (promotion != NO_PIECE_TYPE) {
        move = make<PROMOTION>(fromsquare, tosquare, promotion);
      } else {
        move = make<NORMAL>(fromsquare, tosquare);
      }
    }
  }
  if (pos.pseudo_legal(move) && pos.legal(move, pos.pinned_pieces(pos.side_to_move()))) {
#ifdef SAN_DEBUG
    sync_cout << "found a move: " << move_to_uci(move, false) << sync_endl;
#endif
    return move;
  } else {
#ifdef SAN_DEBUG
    sync_cout << "invalid move: " << move_to_uci(move, false) << sync_endl;
#endif
    return MOVE_NONE; // invalid;
  }
  return MOVE_NONE;
}

Move san_to_move(Position& pos, std::string& str)
{
  std::string uci = str;
  PieceType promotion = NO_PIECE_TYPE;
  bool castles = false;
  bool capture = false;
  Move move = MOVE_NONE;
  
  size_t idx = uci.find_first_of("+#");
  if (idx != std::string::npos) {
    uci.erase(idx); // erase to end of the string
  }
  idx = uci.find_first_of("=");
  if (idx != std::string::npos) {
    char promo = uci.at(idx);
    switch(promo) {
      case 'Q': promotion = QUEEN; break;
      case 'R': promotion = ROOK; break;
      case 'B': promotion = BISHOP; break;
      case 'N': promotion = KNIGHT; break;
      default: return MOVE_NONE; // invalid
    }
    uci.erase(idx);
  } else { // check the last char, is it QRBN?
    char promo2 = uci.at(uci.size() - 1);
    switch(promo2) {
      case 'Q': promotion = QUEEN; break;
      case 'R': promotion = ROOK; break;
      case 'B': promotion = BISHOP; break;
      case 'N': promotion = KNIGHT; break;
      default: ; // nixda
    }
    if (promotion != NO_PIECE_TYPE)
      uci.erase(uci.size() - 1);
  }
  idx = uci.find_first_of("x");
  if (idx != std::string::npos) {
    capture = true;
    uci.erase(idx, 1);
  }
  
  char piece = str.at(0);
  PieceType piecetype;
  std::string thepiece;
  
  switch(piece) {
    case 'N': piecetype = KNIGHT; break;
    case 'B': piecetype = BISHOP; break;
    case 'R': piecetype = ROOK; break;
    case 'Q': piecetype = QUEEN; break;
    case 'K': piecetype = KING; break;
    case '0':
    case 'O':
      castles = true; piecetype = NO_PIECE_TYPE; break;
    default: piecetype = PAWN;
  }
  
#ifdef SAN_DEBUG
  switch(int(piecetype)) {
    case KNIGHT: thepiece = "knight"; break;
    case BISHOP: thepiece = "bishop"; break;
    case ROOK: thepiece = "rook"; break;
    case QUEEN: thepiece = "queen"; break;
    case KING: thepiece = "king"; break;
    case PAWN: thepiece = "pawn"; break;
    case NO_PIECE_TYPE: thepiece = "castles"; break;
  }

  sync_cout << "restring: " << uci << "; piece type: " << thepiece << sync_endl;
#endif

  if (castles) { // chess 960?
    if (uci == "0-0" || uci == "O-O") {
      if (pos.side_to_move() == WHITE) {
        move = make<CASTLE>(SQ_E1, SQ_H1);
      } else {
        move = make<CASTLE>(SQ_E8, SQ_H8);
      }
    } else if (uci == "0-0-0" || uci == "O-O-O") {
      if (pos.side_to_move() == WHITE) {
        move = make<CASTLE>(SQ_E1, SQ_A1);
      } else {
        move = make<CASTLE>(SQ_E8, SQ_A8);
      }
    }
    if (pos.pseudo_legal(move) && pos.legal(move, pos.pinned_pieces(pos.side_to_move()))) {
      return move;
    }
    return MOVE_NONE; // invalid
  }
  
  // normal move or promotion
  int torank = uci.at(uci.size() - 1) - '1';
  int tofile = uci.at(uci.size() - 2) - 'a';
  int disambig_r = -1;
  int disambig_f = -1;
  if (piecetype != PAWN && piecetype != KING && uci.size() > 3) {
    char ambig = uci.at(uci.size() - 3);
    if (ambig >= 'a' && ambig <= 'h') {
      disambig_f = ambig - 'a';
    } else if (ambig >= '1' && ambig <= '8') {
      disambig_r = ambig - '1';
    } else {
      return MOVE_NONE; // invalid;
    }
  }

  Square tosquare = Square((torank * 8) + tofile);
  const Square *pl;
  int piececount;

  switch (piecetype) {
    case PAWN:
      pl = pos.list<PAWN>(pos.side_to_move());
      piececount = pos.count<PAWN>(pos.side_to_move());
      break;
    case KNIGHT:
      pl = pos.list<KNIGHT>(pos.side_to_move());
      piececount = pos.count<KNIGHT>(pos.side_to_move());
      break;
    case BISHOP:
      pl = pos.list<BISHOP>(pos.side_to_move());
      piececount = pos.count<BISHOP>(pos.side_to_move());
      break;
    case ROOK:
      pl = pos.list<ROOK>(pos.side_to_move());
      piececount = pos.count<ROOK>(pos.side_to_move());
      break;
    case QUEEN:
      pl = pos.list<QUEEN>(pos.side_to_move());
      piececount = pos.count<QUEEN>(pos.side_to_move());
      break;
    case KING:
      pl = pos.list<KING>(pos.side_to_move());
      piececount = pos.count<KING>(pos.side_to_move());
      break;
    default:
      return MOVE_NONE; // invalid
  }

  if (piececount == 1) {
    if (piecetype != PAWN || !capture) {
      move = test_move<SAN_MOVE_NORMAL>(pos, *pl, tosquare, promotion);
    } else {
      move = test_move<SAN_PAWN_CAPTURE>(pos, *pl, tosquare, promotion);
    }
    if (move != MOVE_NONE) {
      return move;
    } else {
      return MOVE_NONE;
    }
  } else if (piececount > 1) {
    Square s;
    while ((s = *pl++) != SQ_NONE) {
      Square ss = SQ_NONE;
#ifdef SAN_DEBUG
      sync_cout << "  looking at " << char((s % 8) + 'a') << char ((s / 8) + '1') << sync_endl;
#endif
      if (disambig_r >=0 || disambig_f >= 0) {
        if (disambig_r >= 0 && rank_of(s) == Rank(disambig_r)) {
          ss = s;
        } else if (disambig_f >= 0 && file_of(s) == File(disambig_f)) {
          ss = s;
        }
      } else {
        ss = s;
      }
      if (ss != SQ_NONE) {
        if (piecetype != PAWN || !capture) {
          move = test_move<SAN_MOVE_NORMAL>(pos, ss, tosquare, promotion);
        } else {
          move = test_move<SAN_PAWN_CAPTURE>(pos, ss, tosquare, promotion);
        }
        if (move != MOVE_NONE) {
          return move;
        } else {
          ; // don't return, we just need to keep trying
        }
      }
    }
  }
  return MOVE_NONE;
}
#endif
