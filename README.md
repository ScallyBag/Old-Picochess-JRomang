Stockfish
=========

Stand alone chess computer with Stockfish and DGT Board

This branch supports a pure python implementation with the pyfish engine (taken from the pyfish branch).

It also supports the piface.

To run on the piface and desktop:

To build pyfish:

1. This needs pyfish. To build pyfish (just needed one time):
2. Switch to the pyfish branch -> "git checkout pyfish"
3. Install the python headers, on linux, its "sudo apt-get install python2.7-dev"
3. Go to the src folder
4. Execute "sudo python setup.py install"

After pyfish:

1. Go to the py/ folder
1. Do this one time: https://github.com/piface/pifacecad
1. Also do this one time: "sudo pip install -r requirements.txt" 
1. Execute "python pycochess.py"
1. Fixed time modes should work now (with occasional issues).
1. To stop pycochess, "execute Ctrl-Z" followed by a process kill (e.g. "pkill -9 -f pycochess.py")


To test the DGT driver:

1. Go to the py/ folder
2. Execute "python pydgt.py <device name>" such as /dev/ttyUSB0
3. Ensure that moving a piece on the board will return a new FEN and board graphic
