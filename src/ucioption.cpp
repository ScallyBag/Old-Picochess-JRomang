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

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "evaluate.h"
#include "misc.h"
#include "thread.h"
#include "tt.h"
#include "ucioption.h"
#if PA_GTB
#include "phash.h"
#endif

using std::string;

UCI::OptionsMap Options; // Global object

namespace UCI {

/// 'On change' actions, triggered by an option's value change
void on_logger(const Option& o) { start_logger(o); }
void on_eval(const Option&) { Eval::init(); }
void on_threads(const Option&) { Threads.read_uci_options(); }
void on_hash_size(const Option& o) { TT.set_size(o); }
void on_clear_hash(const Option&) { TT.clear(); }
#if PA_GTB
  void on_clear_phash(const Option&) { wantsclear_phash(); }
  void on_prune_phash(const Option&) { wantsprune_phash(); }
  void on_merge_phash(const Option&) { wantsmerge_phash(); }
#endif

/// Our case insensitive less() function as required by UCI protocol
bool ci_less(char c1, char c2) { return tolower(c1) < tolower(c2); }

bool CaseInsensitiveLess::operator() (const string& s1, const string& s2) const {
  return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), ci_less);
}


/// init() initializes the UCI options to their hard coded default values
/// and initializes the default value of "Threads" and "Min Split Depth"
/// parameters according to the number of CPU cores detected.

void init(OptionsMap& o) {

  int cpus = std::min(cpu_count(), MAX_THREADS);
  int msd = cpus < 8 ? 4 : 7;

  o["Use Debug Log"]               = Option(false, on_logger);
  o["Use Search Log"]              = Option(false);
  o["Search Log Filename"]         = Option("SearchLog.txt");
  o["Book File"]                   = Option("book.bin");
  o["Best Book Move"]              = Option(false);
  o["Contempt Factor"]             = Option(0, -50,  50);
  o["Mobility (Midgame)"]          = Option(100, 0, 200, on_eval);
  o["Mobility (Endgame)"]          = Option(100, 0, 200, on_eval);
  o["Pawn Structure (Midgame)"]    = Option(100, 0, 200, on_eval);
  o["Pawn Structure (Endgame)"]    = Option(100, 0, 200, on_eval);
  o["Passed Pawns (Midgame)"]      = Option(100, 0, 200, on_eval);
  o["Passed Pawns (Endgame)"]      = Option(100, 0, 200, on_eval);
  o["Space"]                       = Option(100, 0, 200, on_eval);
  o["Aggressiveness"]              = Option(100, 0, 200, on_eval);
  o["Cowardice"]                   = Option(100, 0, 200, on_eval);
  o["Min Split Depth"]             = Option(msd, 4, 12, on_threads);
  o["Max Threads per Split Point"] = Option(5, 4, 8, on_threads);
  o["Threads"]                     = Option(cpus, 1, MAX_THREADS, on_threads);
  o["Use Sleeping Threads"]        = Option(true);
  o["Hash"]                        = Option(32, 1, 8192, on_hash_size);
  o["Clear Hash"]                  = Option(on_clear_hash);
  o["Ponder"]                      = Option(true);
  o["OwnBook"]                     = Option(false);
  o["MultiPV"]                     = Option(1, 1, 500);
  o["Skill Level"]                 = Option(20, 0, 20);
  o["Emergency Move Horizon"]      = Option(40, 0, 50);
  o["Emergency Base Time"]         = Option(200, 0, 30000);
  o["Emergency Move Time"]         = Option(70, 0, 5000);
  o["Minimum Thinking Time"]       = Option(20, 0, 5000);
  o["Slow Mover"]                  = Option(100, 10, 1000);
  o["UCI_Chess960"]                = Option(false);
  o["UCI_AnalyseMode"]             = Option(false, on_eval);
#if PA_GTB
  o["Use Persistent Hash"]         = Option(false);
  o["Persistent Hash File"]        = Option("stockfish.hsh");
  o["Clear Persistent Hash"]       = Option(on_clear_phash);
  o["Persistent Hash Depth"]       = Option(24, 10, 99);
  o["Persistent Hash Size"]        = Option(32, 4, 1024);
  o["Prune Persistent Hash"]       = Option(on_prune_phash);
  o["Persistent Hash Merge File"]  = Option("stockfish_merge.hsh");
  o["Merge Persistent Hash"]       = Option(on_merge_phash);
#ifdef USE_EGTB
  o["UseGaviotaTb"]                = Option(true);
  o["ProbeOnlyAtRoot"]             = Option(false);
  o["GaviotaTbPath"]               = Option("c:/gtb");
  o["GaviotaTbCache"]              = Option(32, 4, 1024);
  o["Soft Probe Depth"]            = Option(10, 2, 99);
  o["Hard Probe Depth"]            = Option(16, 2, 99);
  StrVector schemes(5);
  schemes[0] = "Uncompressed";
  schemes[1] = "Huffman (cp1)";
  schemes[2] = "LZF (cp2)";
  schemes[3] = "Zlib-9 (cp3)";
  schemes[4] = "LZMA-5-4k (cp4)";
  o["GaviotaTbCompression"]        = Option(schemes[4], schemes);
#endif
#endif
}


/// operator<<() is used to print all the options default values in chronological
/// insertion order (the idx field) and in the format defined by the UCI protocol.

std::ostream& operator<<(std::ostream& os, const OptionsMap& om) {

  for (size_t idx = 0; idx <= om.size(); idx++)
      for (OptionsMap::const_iterator it = om.begin(); it != om.end(); ++it)
          if (it->second.idx == idx)
          {
              const Option& o = it->second;
              os << "\noption name " << it->first << " type " << o.type;

              if (o.type != "button")
                  os << " default " << o.defaultValue;

              if (o.type == "spin")
                  os << " min " << o.min << " max " << o.max;

#if PA_GTB && defined(USE_EGTB)
              if (o.type == "combo")
              {
                for (StrVector::const_iterator itc = it->second.comboValues.begin(); itc != it->second.comboValues.end(); ++itc)
                  os << " var " << *itc;
              }
#endif
              break;
          }
  return os;
}


/// Option c'tors and conversion operators

Option::Option(const char* v, Fn* f) : type("string"), min(0), max(0), idx(Options.size()), on_change(f)
{ defaultValue = currentValue = v; }

Option::Option(bool v, Fn* f) : type("check"), min(0), max(0), idx(Options.size()), on_change(f)
{ defaultValue = currentValue = (v ? "true" : "false"); }

Option::Option(Fn* f) : type("button"), min(0), max(0), idx(Options.size()), on_change(f)
{}

Option::Option(int v, int minv, int maxv, Fn* f) : type("spin"), min(minv), max(maxv), idx(Options.size()), on_change(f)
{ std::ostringstream ss; ss << v; defaultValue = currentValue = ss.str(); }

#if PA_GTB && defined (USE_EGTB)
Option::Option(string def, StrVector values) : type("combo"), idx(Options.size()), defaultValue(def), currentValue(def), min(0), max(0), comboValues(values)
{}
#endif

Option::operator int() const {
  assert(   type == "check"
         || type == "spin"
#if PA_GTB && defined (USE_EGTB)
         || type == "combo"
#endif
         );
  return ((   type == "spin"
#if PA_GTB && defined (USE_EGTB)
           || type == "combo"
#endif
           ) ? atoi(currentValue.c_str()) : currentValue == "true");
}

Option::operator std::string() const {
  assert(type == "string" || type == "combo");
  return currentValue;
}


/// operator=() updates currentValue and triggers on_change() action. It's up to
/// the GUI to check for option's limits, but we could receive the new value from
/// the user by console window, so let's check the bounds anyway.

Option& Option::operator=(const string& v) {

  assert(!type.empty());

  if (   (type != "button" && v.empty())
      || (type == "check" && v != "true" && v != "false")
      || (type == "spin" && (atoi(v.c_str()) < min || atoi(v.c_str()) > max)))
      return *this;

  if (type != "button")
      currentValue = v;

  if (on_change)
      (*on_change)(*this);

  return *this;
}

} // namespace UCI
