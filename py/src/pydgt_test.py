from pydgt import VirtualDGTBoard
import pydgt
from threading import Thread
from time import sleep

def dgt_observer(attrs):
    if attrs.type == pydgt.FEN:
        print "FEN: {0}".format(attrs.message)
    elif attrs.type == pydgt.BOARD:
        print "Board: "
        print attrs.message

def poll_dgt(board):
    thread = Thread(target=board.poll)
    thread.start()

if __name__ == "__main__":


    board = VirtualDGTBoard('Test')
    board.subscribe(dgt_observer)

    # Start position
    board.set_fen('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1')
    poll_dgt(board)
    sleep(1)
    # After 1.e4
    board.set_fen('rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1')
    sleep(1)
    # After 1..c5
    board.set_fen('rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2')
    sleep(1)
    # After 2.Nf3
    board.set_fen('rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2')


    # board.poll()