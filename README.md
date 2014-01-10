Stockfish
=========

Stand alone chess computer with Stockfish and DGT Board

This branch supports a pure python implementation with the pyfish engine (taken from the pyfish branch).

To run on the piface and desktop:

1. Go to the py/src/dgt folder
2. Execute g++ dgtnix.c  -w -shared -o libdgtnix.so
3. The above command is also in README.txt in the same folder
4. Go to the py/src folder
5. Execute "python pycochess.py"
6. Analysis score should be updated when pieces are moved on the DGT board, sleeps still need to be added.

