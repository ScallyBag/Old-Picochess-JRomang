Picochess
=========

Stand alone chess computer with Stockfish and DGT Board

This page supports a pure python implementation of Picochess. More updated work is at https://github.com/jromang/picochess

This page also supports the piface.

To test the Python DGT driver:

1. Go to the py/ folder
2. Execute "python pydgt.py <device name>" such as /dev/ttyUSB0
3. Ensure that moving a piece on the board will return a new FEN and board graphic


To run on the DGT XL Clock display, piface, and desktop:

Pyfish (stockfish engine for python):

1. To build pyfish (just needed one time):
2. Switch to the pyfish branch -> "git checkout pyfish"
3. Install the python headers, on linux, its "sudo apt-get install python2.7-dev"
3. Go to the src folder
4. Execute "sudo python setup.py install"

After pyfish:

To run on desktop (after one-time flash is complete and the nanpy library is installed):

1. Go to the py/ folder
1. "python pycochess.py <DGT device_name>". E.g. my device name is /dev/cu.usbserial-0000\*\*\*\*  
1. Enjoy! Currently buttons on the Oduino One are not supported but they will be soon..


To run on the raspberry Pi/ or other device with the DGT XL clock connected:

1. Go to the py/ folder
1. Do this one time: https://github.com/piface/pifacecad
1. Also do this one time: "sudo pip install -r requirements.txt" 
1. Execute "python pycochess.py"
1. Fixed time modes should work now (with occasional issues).
1. To stop pycochess, "execute Ctrl-Z" followed by a process kill (e.g. "pkill -9 -f pycochess.py")

Misc:

To run with the Oduino One (One time flash of Oduino One):

1. Git clone https://github.com/sshivaji/nanpy
1. Go to the firmware/Nanpy folder
1. "export BOARD=uno"
1. Modify cfg.h if needed (there is likely NO need to do this).
1. "make"
1. After a successful build, execute "make upload /dev/tty.usbmodem411" (or whatever your device name is for the Oduino One).
1. Install the nanpy library, go to the root folder of https://github.com/sshivaji/nanpy.
1. "sudo python setup.py install"
 
