#include <Python.h>

#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <streambuf>
#include <stack>

#include "types.h"
#include "bitboard.h"
#include "evaluate.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "ucioption.h"
#include "notation.h"
#include "book.h"


using namespace std;

namespace
{
// FEN string of the initial position, normal chess
const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Keep track of position keys along the setup moves (from start position to the
// position just before to start searching). Needed by repetition draw detection.
Search::StateStackPtr SetupStates;
Position pos, searchPos;
vector<PyObject*> observers;
Lock bestmoveLock;
WaitCondition bestmoveCondition;
}

extern "C" PyObject* stockfish_getOptions(PyObject* self)
{
    PyObject* dict = PyDict_New();
    for (UCI::OptionsMap::iterator iter = Options.begin(); iter != Options.end(); ++iter)
    {
        PyObject *dict_key=Py_BuildValue("s", (*iter).first.c_str());
        PyObject *dict_value=((*iter).second.type == "spin" ?
                              Py_BuildValue("(sssii)",(*iter).second.currentValue.c_str(),(*iter).second.type.c_str(),(*iter).second.defaultValue.c_str(),(*iter).second.min,(*iter).second.max):
                              Py_BuildValue("(sss)",(*iter).second.currentValue.c_str(),(*iter).second.type.c_str(),(*iter).second.defaultValue.c_str())
                             );
        PyDict_SetItem(dict,dict_key,dict_value);
    }
    return dict;
}

extern "C" PyObject* stockfish_info(PyObject* self)
{
    return Py_BuildValue("s", engine_info().c_str());
}

extern "C" PyObject* stockfish_key(PyObject* self)
{
    return Py_BuildValue("L", PolyglotBook::polyglot_key(pos));
}

extern "C" PyObject* stockfish_flip(PyObject* self)
{
    pos.flip();
    Py_RETURN_NONE;
}

extern "C" PyObject* stockfish_stop(PyObject* self)
{
    if(Threads.main()->thinking)
    {
        Search::Signals.stop = true;
        Threads.main()->notify_one(); // Could be sleeping

        Py_BEGIN_ALLOW_THREADS
        cond_wait(bestmoveCondition,bestmoveLock);
        Py_END_ALLOW_THREADS
    }
    Py_RETURN_NONE;
}

extern "C" PyObject* stockfish_ponderhit(PyObject* self)
{
    if (Search::Signals.stopOnPonderhit)
        stockfish_stop(self);
    else
        Search::Limits.ponder = false;
    Py_RETURN_NONE;
}

extern "C" PyObject* stockfish_setOption(PyObject* self, PyObject *args)
{
    const char *name;
    PyObject *valueObj;
    if (!PyArg_ParseTuple(args, "sO", &name, &valueObj)) return NULL;

    if (Options.count(name))
        Options[name] = string(PyString_AsString(PyObject_Str(valueObj)));
    else
    {
        PyErr_SetString(PyExc_ValueError, (string("No such option ")+name+"'").c_str());
        return NULL;
    }
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
    pos.set(fen, Options["UCI_Chess960"], Threads.main());
    SetupStates = Search::StateStackPtr(new std::stack<StateInfo>());

    // parse the move list
    int numMoves = PyList_Size(listObj);
    for (int i=0; i<numMoves ; i++) {
        string moveStr( PyString_AsString( PyList_GetItem(listObj, i)) );
        Move m;
        if((m = move_from_uci(pos, moveStr)) != MOVE_NONE)
        {
            SetupStates->push(StateInfo());
            pos.do_move(m, SetupStates->top());
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, (string("Invalid move '")+moveStr+"'").c_str());
            return NULL;
        }

    }
    Py_RETURN_NONE;
}

extern "C" PyObject* stockfish_addObserver(PyObject* self, PyObject *args)
{
    PyObject *observer;
    if (!PyArg_ParseTuple(args, "O", &observer)) return NULL;

    Py_INCREF(observer);
    observers.push_back(observer);
    Py_RETURN_NONE;
}

extern "C" PyObject* stockfish_removeObserver(PyObject* self, PyObject *args)
{
    PyObject *observer;
    if (!PyArg_ParseTuple(args, "O", &observer)) return NULL;

    observers.erase(remove(observers.begin(), observers.end(), observer), observers.end());
    Py_XDECREF(observer);
    Py_RETURN_NONE;
}

extern "C" PyObject* stockfish_legalMoves(PyObject* self)
{
    PyObject* list = PyList_New(0);

    for (MoveList<LEGAL> it(pos); *it; ++it)
    {
        PyObject *move=Py_BuildValue("s", move_to_uci(*it,false).c_str());
        PyList_Append(list, move);
        Py_XDECREF(move);
    }
    return list;
}

void stockfish_notifyObservers(string s)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject *arglist=Py_BuildValue("(s)", s.c_str());
    for (vector<PyObject*>::iterator it = observers.begin() ; it != observers.end(); ++it)
        PyObject_CallObject(*it, arglist);

    if(!s.compare(0,8,"bestmove")) cond_signal(bestmoveCondition);

    Py_DECREF(arglist);
    PyGILState_Release(gstate);
}

//Given a list of moves in CAN formats, it returns a list of moves in SAN format
extern "C" PyObject* stockfish_toSAN(PyObject* self, PyObject *args)
{
    PyObject* sanMoves = PyList_New(0), *moveList;
    stack<Move> moveStack;
    Search::StateStackPtr states = Search::StateStackPtr(new std::stack<StateInfo>());

    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &moveList)) return NULL;

    // parse the move list
    int numMoves = PyList_Size(moveList);
    for (int i=0; i<numMoves ; i++) {
        string moveStr( PyString_AsString( PyList_GetItem(moveList, i)) );
        Move m;
        if((m = move_from_uci(pos, moveStr)) != MOVE_NONE)
        {
            //add to the san move list
            PyObject *move=Py_BuildValue("s", move_to_san(pos,m).c_str());
            PyList_Append(sanMoves, move);
            Py_XDECREF(move);

            //do the move
            states->push(StateInfo());
            moveStack.push(m);
            pos.do_move(m, states->top());
        }
        else
        {
            //undo the moves
            while(!moveStack.empty())
            {
                pos.undo_move(moveStack.top());
                moveStack.pop();
            }
            PyErr_SetString(PyExc_ValueError, (string("Invalid move '")+moveStr+"'").c_str());
            return NULL;
        }
    }

    //undo the moves
    while(!moveStack.empty())
    {
        pos.undo_move(moveStack.top());
        moveStack.pop();
    }

    return sanMoves;
}

//Given a list of moves in SAN formats, it returns a list of moves in CAN format
extern "C" PyObject* stockfish_toCAN(PyObject* self, PyObject *args)
{
    PyObject *canMoves = PyList_New(0), *moveList;
    stack<Move> moveStack;
    Search::StateStackPtr states = Search::StateStackPtr(new std::stack<StateInfo>());

    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &moveList)) return NULL;

    // parse the move list
    int numMoves = PyList_Size(moveList);
    for (int i=0; i<numMoves ; i++)
    {
        string moveStr( PyString_AsString( PyList_GetItem(moveList, i)) );
        bool found=false;
        for (MoveList<LEGAL> it(pos); *it; ++it)
        {
            if(!moveStr.compare(move_to_san(pos,*it)))
            {
                PyObject *move=Py_BuildValue("s", move_to_uci(*it,false).c_str());
                //add to the can move list
                PyList_Append(canMoves, move);
                Py_XDECREF(move);

                //do the move
                states->push(StateInfo());
                moveStack.push(*it);
                pos.do_move(*it, states->top());
                found=true;
                break;
            }
        }
        if(!found)
        {
            PyErr_SetString(PyExc_ValueError, (string("Invalid move '")+moveStr+"'").c_str());
            return NULL;
        }
    }

    //undo the moves
    while(!moveStack.empty())
    {
        pos.undo_move(moveStack.top());
        moveStack.pop();
    }

    return canMoves;
}

// go() is called when engine receives the "go" UCI command. The function sets
// the thinking time and other parameters from the input string, and starts
// the search.
extern "C" PyObject* stockfish_go(PyObject *self, PyObject *args, PyObject *kwargs) {
    Search::LimitsType limits;
    vector<Move> searchMoves;
    PyObject *listSearchMoves;

    stockfish_stop(self);
    const char *kwlist[] = {"searchmoves", "wtime", "btime", "winc", "binc", "movestogo", "depth", "nodes", "movetime", "mate", "infinite", "ponder", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O!iiiiiiiiiii", const_cast<char **>(kwlist), &PyList_Type, &listSearchMoves,
                                     &(limits.time[WHITE]), &(limits.time[BLACK]), &(limits.inc[WHITE]), &(limits.inc[BLACK]),
                                     &(limits.movestogo), &(limits.depth), &(limits.nodes), &(limits.movetime), &(limits.mate), &(limits.infinite), &(limits.ponder)))
        return NULL;

    searchPos=pos;
    Threads.start_thinking(searchPos, limits, searchMoves, SetupStates);
    Py_RETURN_NONE;
}

static char stockfish_docs[] =
    "helloworld( ): Any message you want to put here!!\n";

static PyMethodDef stockfish_funcs[] = {
    {"addObserver", (PyCFunction)stockfish_addObserver, METH_VARARGS, stockfish_docs},
    {"removeObserver", (PyCFunction)stockfish_removeObserver, METH_VARARGS, stockfish_docs},
    {"flip", (PyCFunction)stockfish_flip, METH_NOARGS, stockfish_docs},
    {"go", (PyCFunction)stockfish_go, METH_KEYWORDS, stockfish_docs},
    {"info", (PyCFunction)stockfish_info, METH_NOARGS, stockfish_docs},
    {"key", (PyCFunction)stockfish_key, METH_NOARGS, stockfish_docs},
    {"legalMoves", (PyCFunction)stockfish_legalMoves, METH_NOARGS, stockfish_docs},
    {"toCAN", (PyCFunction)stockfish_toCAN, METH_VARARGS, stockfish_docs},
    {"toSAN", (PyCFunction)stockfish_toSAN, METH_VARARGS, stockfish_docs},
    {"ponderhit", (PyCFunction)stockfish_ponderhit, METH_NOARGS, stockfish_docs},
    {"position", (PyCFunction)stockfish_position, METH_VARARGS, stockfish_docs},
    {"setOption", (PyCFunction)stockfish_setOption, METH_VARARGS, stockfish_docs},
    {"getOptions", (PyCFunction)stockfish_getOptions, METH_NOARGS, stockfish_docs},
    {"stop", (PyCFunction)stockfish_stop, METH_NOARGS, stockfish_docs},
    {NULL}
};

PyMODINIT_FUNC initstockfish(void)
{
    Py_InitModule3("stockfish", stockfish_funcs, "Extension module example!");

    UCI::init(Options);
    Bitboards::init();
    Position::init();
    Bitbases::init_kpk();
    Search::init();
    Pawns::init();
    Eval::init();
    Threads.init();
    TT.set_size(Options["Hash"]);

    lock_init(bestmoveLock);
    cond_init(bestmoveCondition);

    pos.set(StartFEN, false, Threads.main());

    // Make sure the GIL has been created since we need to acquire it in our
    // callback to safely call into the python application.
    if (! PyEval_ThreadsInitialized()) {
        PyEval_InitThreads();
    }
}
