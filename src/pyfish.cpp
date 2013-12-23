#include <Python.h>

#include <sstream>
#include <iostream>

#include "bitboard.h"
#include "evaluate.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "ucioption.h"
#include "notation.h"


using namespace std;

// FEN string of the initial position, normal chess
const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Keep track of position keys along the setup moves (from start position to the
// position just before to start searching). Needed by repetition draw detection.
Search::StateStackPtr SetupStates;


Position *pos;

extern "C" PyObject* stockfish_info(PyObject* self)
{
    return Py_BuildValue("s", engine_info().c_str());
}

extern "C" PyObject* stockfish_flip(PyObject* self)
{
    pos->flip();
    Py_RETURN_NONE;
}

extern "C" PyObject* stockfish_setOption(PyObject* self, PyObject *args)
{
    const char *name;
    PyObject *valueObj;
    if (!PyArg_ParseTuple(args, "sO", &name, &valueObj)) {
        return NULL;
    }

    if (Options.count(name))
        Options[name] = string(PyString_AsString(PyObject_Str(valueObj)));
    else
        sync_cout << "No such option: " << name << sync_endl; //TODO raise exception

    Py_RETURN_NONE;
}

extern "C" PyObject* stockfish_position(PyObject* self, PyObject *args)
{
    PyObject *listObj;
    const char *fen;
    if (!PyArg_ParseTuple(args, "sO!", &fen,  &PyList_Type, &listObj)) {
        return NULL;
    }

    if(strcmp(fen,"startpos")==0) fen=StartFEN;
    pos->set(fen, Options["UCI_Chess960"], Threads.main());
    SetupStates = Search::StateStackPtr(new std::stack<StateInfo>());

    // parse the move list
    int numMoves = PyList_Size(listObj);
    for (int i=0; i<numMoves ; i++) {
        string moveStr( PyString_AsString( PyList_GetItem(listObj, i)) );
        Move m;
        if((m = move_from_uci(*pos, moveStr)) != MOVE_NONE)
        {
            SetupStates->push(StateInfo());
            pos->do_move(m, SetupStates->top());
        }
        else {
            cout<<"Invalid move:"<<moveStr<<endl;
            break; //TODO raise error
        }

    }

    Py_RETURN_NONE;
}

// go() is called when engine receives the "go" UCI command. The function sets
// the thinking time and other parameters from the input string, and starts
// the search.

extern "C" PyObject* stockfish_go(PyObject *self, PyObject *args, PyObject *kwargs) {
    Search::LimitsType limits;
    vector<Move> searchMoves;
    PyObject *listSearchMoves;

    const char *kwlist[] = {"searchmoves", "wtime", "btime", "winc", "binc", "movestogo", "depth", "nodes", "movetime", "mate", "infinite", "ponder", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O!iiiiiiiiiii", const_cast<char **>(kwlist), &PyList_Type, &listSearchMoves,
                                     &(limits.time[WHITE]), &(limits.time[BLACK]), &(limits.inc[WHITE]), &(limits.inc[BLACK]),
                                     &(limits.movestogo), &(limits.depth), &(limits.nodes), &(limits.movetime), &(limits.mate), &(limits.infinite), &(limits.ponder)))
        return NULL;

    Threads.start_thinking(*pos, limits, searchMoves, SetupStates);
    Py_RETURN_NONE;
}

static char stockfish_docs[] =
    "helloworld( ): Any message you want to put here!!\n";

static PyMethodDef stockfish_funcs[] = {
    {"flip", (PyCFunction)stockfish_flip, METH_NOARGS, stockfish_docs},
    {"go", (PyCFunction)stockfish_go, METH_KEYWORDS, stockfish_docs},
    {"info", (PyCFunction)stockfish_info, METH_NOARGS, stockfish_docs},
    {"position", (PyCFunction)stockfish_position, METH_VARARGS, stockfish_docs},
    {"setOption", (PyCFunction)stockfish_setOption, METH_VARARGS, stockfish_docs},
    {NULL}
};

PyMODINIT_FUNC initstockfish(void)
{
    Py_InitModule3("stockfish", stockfish_funcs,
                   "Extension module example!");

    UCI::init(Options);
    Bitboards::init();
    Position::init();
    Bitbases::init_kpk();
    Search::init();
    Pawns::init();
    Eval::init();
    Threads.init();
    TT.set_size(Options["Hash"]);

    pos=new Position(StartFEN, false, Threads.main());
}
