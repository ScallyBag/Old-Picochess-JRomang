from distutils.core import setup, Extension
import platform
import sys
import os

args=[]
if '64bit' in platform.architecture():
  args.append('-DIS_64BIT')

if (sys.platform == 'darwin' and [int(x) for x in os.uname()[2].split('.')] >= [11, 0, 0]):
            # special things for clang
    args.append('-Wno-error=unused-command-line-argument-hard-error-in-future')
module1 = Extension('stockfish',
                    sources = ['benchmark.cpp', 'evaluate.cpp', 'movepick.cpp', 'search.cpp', 'ucioption.cpp',
                               'bitbase.cpp', 'main.cpp', 'notation.cpp', 'thread.cpp', 'bitboard.cpp', 'material.cpp',
                               'pawns.cpp', 'timeman.cpp', 'book.cpp', 'misc.cpp', 'position.cpp', 'tt.cpp', 'endgame.cpp',
                               'movegen.cpp', 'pyfish.cpp', 'uci.cpp', 'tbprobe.cpp'],extra_compile_args=args)

setup (name = 'stockfish',
       version = '1.0',
       description = 'This is a demo package',
       ext_modules = [module1])
