### PyFish

Pyfish is a python wrapper for the Stockfish engine. It wraps Stockfish as a C module.

To build and install pyfish: 

1. Install git if you dont already have it via ``sudo apt-get install git`` if you have Ubuntu.
1. ``git clone https://github.com/jromang/Stockfish``
1. Switch to the pyfish branch - ``git checkout pyfish``
1. Go to the src directory - ``cd src``
1. Install the python-dev package if you dont already have it - For Ubuntu, its ``sudo apt-get install python-dev``
1. Install the pyfish package via ``sudo python setup.py install``
1. If this step succeeds, you are done.

Pyfish python API:

1. To call pyfish in your code - ``import stockfish as sf``
1. You can then call methods from the stockfish/sf module. Note that the go, legal\_moves, to\_can, to\_san methods required a fen and a move list as the first two parameters. The move list can be an empty ([]) if the processing can be done on the fen itself. 
1. Supported methods: 
   1. ``add_observer`` : Add an observer method that processes every line returned by stockfish. Example - ``sf.add_observer(<your method>)``
   1. ``remove_observer`` : Remove an observer. This is probably not needed unless you want to register an observer.
   1. ``go`` : Start thinking. The parameters supported are listed below. Fen and moves are required, the rest of the params are optional. They call all be passed as keyword params. Example - ``sf.go(fen='startpos', moves=[], wtime=int(5000*1000), btime=int(5000*1000), winc=int(5*1000), binc=int(5*1000))`` for a G/5 with a 5 second increment every move. Example of infinite search - ``sf.go(fen='startpos', moves=[], infinite=True)`` launches an infinite search on the start position. Note: You MUST terminate an infinite search with sf.stop() at some point!
   
       1. fen is the first parameter and is a string. For the start position, ``'startpos'`` is acceptable.
       1. moves is a list of moves that have been played from the fen position. This can be an empty list.
       1. ``searchmoves`` is the set of moves to restrict the search to.
       1. ``wtime`` is the amount of time in milliseconds to give to white
       1. ``btime`` is the same for black
       1. ``winc`` is the increment in milliseconds given every move
       1. ``binc`` is the same figure for black
       1. ``depth`` is the search depth, is an integer
       1. if ``infinite=True``, the search depth is infinite
       1. if ``movetime`` is provided, that becomes the amount of fixed time for a move
   1. ``info`` : Get version info from stockfish.
   1. ``key`` : Get the polyglot opening book key.
   1. ``legal_moves`` : Get legal moves for a position, need fen as the first argument and the moves played as the next argument. An empty list is fine for moves played if you want legal moves for the fen position. Example - ``sf.legal_moves('startpos')`` will return a list of legal moves from the start position.
   1. ``get_fen`` : Get resulting fen after executing a bunch of moves, first parameter is fen, and second parameter is the move list to execute. Example - ``sf.get_fen('startpos',['e2e4'])`` will return the FEN after 1.e4 from the start position.
   1. ``to_can`` : Given a fen, and a list of moves in SAN notation, return a list of CAN moves. e.g. ``sf.to_can('startpos', ['e4','c5']) will return ['e2e4', 'c7c5']
   1. ``to_san`` : Given a fen, and a list of moves in CAN notation, return a list of SAN moves. e.g. ``sf.to_can('startpos', ['e2e4','c7c5']) will return ['e4', 'c5']
   1. ``ponderhit`` : If stockfish is set to stop on ponder via a UCI option, calling this method will make Stockfish stop searching when a ponder is hit.
   1. ``set_option`` : Set a particular UCI option with a value. Example - ``sf.set_option('skill level', 10)`` will set stockfish to level 10
   1. ``get_options`` : Get the list of supported UCI options along with current and supported values as a dictionary.
   1. ``stop`` : Stop the current search, if there is an active search. Example - ``sf.stop()``
1. More examples are in pyfish_test.py
   


### Stockfish Overview

Stockfish is a free UCI chess engine derived from Glaurung 2.1. It is
not a complete chess program and requires some UCI-compatible GUI
(e.g. XBoard with PolyGlot, eboard, Arena, Sigma Chess, Shredder, Chess
Partner or Fritz) in order to be used comfortably. Read the
documentation for your GUI of choice for information about how to use
Stockfish with it.

This version of Stockfish supports up to 64 CPUs. The engine defaults
to one search thread, so it is therefore recommended to inspect the value of
the *Threads* UCI parameter, and to make sure it equals the number of CPU
cores on your computer.

This version of Stockfish has support for Syzygybases.


### Files

This distribution of Stockfish consists of the following files:

  * Readme.md, the file you are currently reading.

  * Copying.txt, a text file containing the GNU General Public License.

  * src, a subdirectory containing the full source code, including a Makefile
    that can be used to compile Stockfish on Unix-like systems. For further
    information about how to compile Stockfish yourself read section below.

  * polyglot.ini, for using Stockfish with Fabien Letouzey's PolyGlot
    adapter.


### Syzygybases

**Configuration**

Syzygybases are configured using the UCI options "SyzygyProbeLimit" and
"SyzygyPath".

The option "SyzygyPath" should be set to the directory or directories
where the .rtbw and .rtbz files can be found. Multiple directories should
be separated by ";" on Windows and by ":" on Unix-based operating systems.

Example: `C:\tablebases\wdl345;C:\tablebases\wdl6;D:\tablebases\dtz345;D:\tablebases\dtz6`

It is recommended to store .rtbw files on an SSD. There is no loss in
storing the .rtbz files on a regular HD.

**Note:** At the moment, the "SyzygyPath" option can only be set once. If you want to change it, you need to restart the engine.

If you have the 6-piece tables, set the value of "SyzygyProbeLimit" to 6 (the default).
If you only have the 5-piece table, set it to 5. Set the value of this option
to 0 if you want to temporarily disable tablebase probing.

**What to expect**  
If the engine is searching a position that is not in the tablebases (e.g.
a position with 7 pieces), it will access the tablebases during the search.
If the engine reports a large mate score, this means that it has found a
winning line into a tablebase position. Example: mate in 60 means 10 moves
into a winning tablebase position.

If the engine is given a position to search that is in the tablebases, it
will use the tablebases at the beginning of the search to preselect all
good moves, i.e. all moves that preserve the win or preserve the draw while
taking into account the 50-move rule.
It wil then perform a search only on those moves. **The engine will not move
immediately**, unless there is only a single good move. **The engine might 
not report a mate score even when the position is won.** Instead, it reports
the score that is returned by the search.

It is therefore clear that behaviour is not identical to what one might
be used to with Nalimov tablebases. There are technical reasons for this
difference, the main technical reason being that Nalimov tablebases use the
DTM metric (distance-to-mate), while Syzygybases use a variation of the
DTZ metric (distance-to-zero, zero meaning any move that resets the 50-move
counter). This special metric is one of the reasons that Syzygybases are
more compact than Nalimov tablebases, while still storing all information
needed for optimal play and in addition being able to take into account
the 50-move rule.

In the near future an option will be added to switch between the current
behaviour and a mode in which Stockfish will immediately play one of the
good moves. This new mode will have the problem that it leads to unnatural
play once the engine has reached a tablebase position. For example, the
engine will then prefer any winning pawn move (even those that lose material
and complicate the win) over moves that lead to a quick mate but have a
higher "distance-to-zero" value.


### Opening books

This version of Stockfish has support for PolyGlot opening books. For
information about how to create such books, consult the PolyGlot
documentation. The book file can be selected by setting the *Book File*
UCI parameter.


### Compiling it yourself

On Unix-like systems, it should be possible to compile Stockfish
directly from the source code with the included Makefile.

Stockfish has support for 32 or 64-bit CPUs, the hardware POPCNT
instruction, big-endian machines such as Power PC, and other platforms.

In general it is recommended to run `make help` to see a list of make
targets with corresponding descriptions. When not using the Makefile to
compile (for instance with Microsoft MSVC) you need to manually
set/unset some switches in the compiler command line; see file *types.h*
for a quick reference.


### Terms of use

Stockfish is free, and distributed under the **GNU General Public License**
(GPL). Essentially, this means that you are free to do almost exactly
what you want with the program, including distributing it among your
friends, making it available for download from your web site, selling
it (either by itself or as part of some bigger software package), or
using it as the starting point for a software project of your own.

The only real limitation is that whenever you distribute Stockfish in
some way, you must always include the full source code, or a pointer
to where the source code can be found. If you make any changes to the
source code, these changes must also be made available under the GPL.

For full details, read the copy of the GPL found in the file named
*Copying.txt*
