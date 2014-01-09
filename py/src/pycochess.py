import traceback
from dgt.dgtnix import *
import stockfish as sf
from threading import Thread
from time import sleep

class DGTBoard(object):

    def __init__(self, device, **kwargs):
        self.dgtnix = None
        self.dgt_fen = None
        self.dgt_connected = False
        self.device = device

    def get_legal_move(self, from_fen, to_fen):
        to_fen_first_tok = to_fen.split()[0]
        for m in sf.legalMoves(fen=from_fen):
            cur_fen = sf.get_fen(from_fen,[m])
            cur_fen_first_tok = str(cur_fen).split()[0]

            if cur_fen_first_tok == to_fen_first_tok:
                return m

    def disconnect(self):
        if self.dgtnix:
            self.dgtnix.Close()
        self.dgt_connected = False

    def connect(self):
        try:
            self.dgtnix = dgtnix("dgt/libdgtnix.so")
            self.dgtnix.SetOption(dgtnix.DGTNIX_DEBUG, dgtnix.DGTNIX_DEBUG_ON)
            # Initialize the driver with port argv[1]
            result = self.dgtnix.Init(self.device)
            if result < 0:
                print "Unable to connect to the device on {0}".format(self.device)
            else:
                print "The board was found"
                self.dgtnix.update()
                self.dgt_connected = True
        except dgtnix.DgtnixError, e:
            print "unable to load the library : %s " % e

    def probe(self, *args):
        if self.dgt_connected and self.dgtnix:
            try:
                new_dgt_fen = self.dgtnix.GetFen()
                #            print "length of new dgt fen: {0}".format(len(new_dgt_fen))
                #            print "new_dgt_fen just obtained: {0}".format(new_dgt_fen)
                if self.dgt_fen and new_dgt_fen:
                    if new_dgt_fen != self.dgt_fen:
                        m = self.get_legal_move(self.dgt_fen, new_dgt_fen)
                        if m:
                            return m
                        # if not self.try_dgt_legal_moves(self.chessboard.position.fen, new_dgt_fen):
                        #     if self.chessboard.previous_node:
                        #     #                            print new_dgt_fen
                        #     #                            print self.chessboard.previous_node.position.fen
                        #         dgt_fen_start = new_dgt_fen.split()[0]
                        #         prev_fen_start = self.chessboard.previous_node.position.fen.split()[0]
                        #         if dgt_fen_start == prev_fen_start:
                        #             self.back('dgt')
                        # if self.engine_mode != ENGINE_PLAY and self.engine_mode != ENGINE_ANALYSIS:
                        #     if len(self.chessboard.variations)>0:
                        #         self.dgtnix.SendToClock(self.format_move_for_dgt(str(self.chessboard.variations[0].move)), self.  dgt_clock_sound, False)

                elif new_dgt_fen:
                    self.dgt_fen = new_dgt_fen
                # if self.engine_mode == ENGINE_PLAY and self.engine_computer_move:
                #     # Print engine move on DGT XL clock
                #     self.dgtnix.SendToClock(self.format_str_for_dgt(self.format_time_str(self.time_white,separator='')+self.      format_time_str(self.time_black, separator='')), False, True)

            except Exception:
                self.dgt_connected = False
                self.dgtnix=None
                print traceback.format_exc()


class EngineManager(object):

    ANALYSIS = "Analysis"
    PLAY = "Play"
    OBSERVE = "Observe"

    ## MODES: Play, Analyze, Observe
    def __init__(self, **kwargs):
        sf.addObserver(self.parse_score)

        self.score = None
        self.engine_mode = EngineManager.PLAY

#    def show_score_on_dgt(self):
#        if self.engine_mode == EngineManager.ANALYSIS:
#            out_score = self.parse_score(line)
#            #out_score = None
#            if out_score:
#                first_mv, raw_line, cleaned_line = out_score
#
#                if self.dgt_connected and self.dgtnix:
#                    # Display score on the DGT clock
#                    score = str(self.get_score(line))
#                    if score.startswith("mate"):
#                        score = score[4:]
#                        score = "m "+score
#                    score = score.replace("-", "n")
#                    self.dgtnix.SendToClock(self.format_str_for_dgt(score), False, True)
#                    if first_mv:
#                        sleep(1)
#                        self.dgtnix.SendToClock(self.format_move_for_dgt(first_mv), False, False)

    def get_score(self, line):
        tokens = line.split()
        try:
            score_index = tokens.index('score')
        except ValueError, e:
            score_index = -1
        score = None
        score_type = ""
        # print line
        if score_index != -1:
            score_type = tokens[score_index + 1]
            if tokens[score_index + 1] == "cp":
                score = float(tokens[score_index + 2]) / 100 * 1.0
                try:
                    score = float(score)
                except ValueError, e:
                    print "Cannot convert score to a float"
                    print e
            elif tokens[score_index + 1] == "mate":
                score = int(tokens[score_index + 2])
                try:
                    score = int(score)
                except ValueError, e:
                    print "Cannot convert Mate number of moves to a int"
                    print e

            # print self.chessboard.position.turn
#            if self.chessboard.position.turn == 'b':
#                if score:
#                    score *= -1
            if score_type == "mate":
                score = score_type + " " + str(score)
        return score

    def parse_score(self, line):
        score = self.get_score(line)
        move_list = []
        tokens = line.split()
        first_mv = None
        try:
         line_index = tokens.index('pv')
         first_mv = tokens[line_index+1]
         move_list=sf.toSAN(tokens[line_index+1:])
         print score
         print move_list

        except ValueError, e:
         line_index = -1


def start_dgt_thread():
    t = Thread(target=dgt_probe, args=())
    t.daemon = True # thread dies with the program
    t.start()

def dgt_probe():
    dgt = DGTBoard("/dev/cu.usbserial-00001004")
    dgt.connect()
    dgt.probe()
    while True:
        sleep(1)
        print dgt.dgtnix.GetFen()

#        print dgt.dgt_fen

if __name__ == '__main__':
    start_dgt_thread()
    while True:
        pass
