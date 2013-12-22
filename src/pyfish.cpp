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

extern "C" PyObject* stockfish_position(PyObject* self, PyObject *args)
{
  //http://code.activestate.com/lists/python-list/31841/
    PyObject *o;
    char *s;
    if (!PyArg_ParseTuple(args, "sO", &s,  &o)) {
      return NULL;
    }
    cout<<"initial:"<< s<<endl;
    Py_RETURN_NONE;
}       

static char stockfish_docs[] =
    "helloworld( ): Any message you want to put here!!\n";

static PyMethodDef stockfish_funcs[] = {
    {"info", (PyCFunction)stockfish_info, METH_NOARGS, stockfish_docs},
    {"position", (PyCFunction)stockfish_position, METH_VARARGS, stockfish_docs},
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
