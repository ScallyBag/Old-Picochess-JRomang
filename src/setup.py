from distutils.core import setup, Extension
import platform

args=[]
if '64bit' in platform.architecture():
  args.append('-DIS_64BIT')

module1 = Extension('stockfish',
                    sources = ['benchmark.cpp', 'evaluate.cpp', 'movepick.cpp', 'search.cpp', 'ucioption.cpp',
                               'bitbase.cpp', 'main.cpp', 'notation.cpp', 'thread.cpp', 'bitboard.cpp', 'material.cpp',
                               'pawns.cpp', 'timeman.cpp', 'book.cpp', 'misc.cpp', 'position.cpp', 'tt.cpp', 'endgame.cpp',
                               'movegen.cpp', 'pyfish.cpp', 'uci.cpp'],extra_compile_args=args)

setup (name = 'stockfish',
       version = '1.0',
       description = 'This is a demo package',
       ext_modules = [module1])
