import stockfish as sf
import time

def get_legal_move(from_fen, to_fen):
     to_fen_first_tok = to_fen.split()[0]
     for m in sf.legal_moves(from_fen):
         cur_fen = sf.get_fen(from_fen,[m])
         cur_fen_first_tok = str(cur_fen).split()[0]
#            print "cur_token:{0}".format(cur_fen_first_tok)
#            print "to_token:{0}".format(to_fen_first_tok)
         if cur_fen_first_tok == to_fen_first_tok:
             return m

#Show info
print(sf.info())

#Show UCI option
print "The list of supported UCI options are : "
print(sf.get_options())

#Set an option
sf.set_option('hash', 32)

#Create an observer function
def my_observer(s):
   print('I observed the line from Stockfish : '+s)

#Register the observer
sf.add_observer(my_observer)

#Set a position (UCI style)
#sf.position('startpos', ['e2e4', 'e7e6'])

#Get legal moves
print(sf.legal_moves('startpos'))

#Launch a search
sf.go('startpos',[], depth=5)

#Wait to avoid exiting the script during search
time.sleep(3)

#CAN to SAN notation
#sf.position('startpos', [])
print "Example of converting from CAN to SAN"
print "e2e4 b8c6 in SAN notation is : "
print sf.to_san('startpos', ['e2e4','b8c6'])

from_fen = "7k/1pr3pp/p4p2/4pP1P/8/1qnP2Q1/3R1PP1/3BK3 b - - 0 1"
to_fen = "7k/1pr3pp/p4p2/4pP1P/8/q1nP2Q1/3R1PP1/3BK3 w - - 0 1"

assert get_legal_move(from_fen, to_fen) == 'b3a3'
print "Legal move fen parsing test passes"
