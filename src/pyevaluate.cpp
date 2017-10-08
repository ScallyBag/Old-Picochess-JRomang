/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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
#include <cstring> // For std::memset
#include <iomanip>
#include <sstream>
#include <iostream>
#include <Python.h>

#include "bitboard.h"
#include "position.h"
#include "pyevaluate.h"

namespace PyEval {

static int gil_init = 0;

PyObject *pName, *pModule, *pDict, *pFunc;
PyObject *pChessModule, *pBoardFunc;

void init()
{
    Py_Initialize();

    PyObject* sys = PyImport_ImportModule("sys");
    PyObject* path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyUnicode_DecodeFSDefault("."));

    pName = PyUnicode_DecodeFSDefault("evaluate");
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    pName = PyUnicode_DecodeFSDefault("chess");
    pChessModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pChessModule != NULL) {
        pBoardFunc = PyObject_GetAttrString(pChessModule, "Board");
        if (!(pBoardFunc && PyCallable_Check(pBoardFunc))) {

            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function Board\n");
        }
    }
    else {
        PyErr_Print();
        fprintf(stderr, "Failed to load chess\n");
    }

    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, "evaluate");
        if (!(pFunc && PyCallable_Check(pFunc))) {

            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function evaluate\n");
        }
    }
    else {
        PyErr_Print();
        fprintf(stderr, "Failed to load evaluate.py\n");
    }
}

Value evaluate(const Position& pos)
{
    if (pModule == NULL)
        return (Value)0;
    PyObject *pArgs, *pBoard, *pStringFEN, *pValue;

    if (!gil_init) {
        gil_init = 1;
        PyEval_InitThreads();
        PyEval_SaveThread();
    }
    PyGILState_STATE state;
    state = PyGILState_Ensure();

    pStringFEN = PyUnicode_DecodeFSDefault(pos.fen().c_str());
    pArgs = PyTuple_Pack(1, pStringFEN);
    pBoard = PyObject_CallObject(pBoardFunc, pArgs);

    Py_DECREF(pStringFEN);
    Py_DECREF(pArgs);

    pArgs = PyTuple_Pack(1, pBoard);
    pValue = PyObject_CallObject(pFunc, pArgs);
    if (PyErr_Occurred()) {
        fprintf(stderr, "Error in evaluation function\n");
        PyErr_Print();
        return (Value)0;
    }

    Py_DECREF(pArgs);
    Py_DECREF(pBoard);

    int value = _PyLong_AsInt(pValue);
    Py_DECREF(pValue);

    PyGILState_Release(state);

    return (Value)(pos.side_to_move() == WHITE ? value : -value); // + Eval::Tempo; // Side to move point of view
}
}