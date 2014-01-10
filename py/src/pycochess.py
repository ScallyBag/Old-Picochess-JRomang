
import traceback
from dgt.dgtnix import *
import stockfish as sf
from threading import Thread
from threading import Semaphore
from threading import Timer
from time import sleep
import itertools as it
import os

piface = None
try:
    import pifacecad
    piface = True
except ImportError:
    piface = False

WHITE = "w"
BLACK = "b"

dgt_sem = Semaphore(value=0)

class DGTBoard(object):

    def __init__(self, device, **kwargs):
        self.dgtnix = None
        self.dgt_fen = None
        self.dgt_connected = False
        self.device = device
        self.turn = WHITE
        self.board_updated = False
        self.move_list = []

    def get_legal_move(self, from_fen, to_fen):
        to_fen_first_tok = to_fen.split()[0]
        for m in sf.legal_moves(from_fen):
            cur_fen = sf.get_fen(from_fen,[m])
            cur_fen_first_tok = str(cur_fen).split()[0]
#            print "cur_token:{0}".format(cur_fen_first_tok)
#            print "to_token:{0}".format(to_fen_first_tok)
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
        except DgtnixError, e:
            print "unable to load the library : %s " % e

    def switch_turn(self):
        if self.turn == WHITE:
            self.turn = BLACK
        elif self.turn == BLACK:
            self.turn = WHITE

    def probe_move(self, *args):
        if self.dgt_connected and self.dgtnix:
            try:

                new_dgt_fen = self.dgtnix.getFen(color=self.turn)
#                new_dgt_fen =
                #            print "length of new dgt fen: {0}".format(len(new_dgt_fen))
                #            print "new_dgt_fen just obtained: {0}".format(new_dgt_fen)
#                print "Old FEN: {0}".format(self.dgt_fen)
                if self.dgt_fen and new_dgt_fen:
                    old_dgt_first_token = self.dgt_fen.split()[0]
                    new_dgt_first_token = new_dgt_fen.split()[0]

                    if old_dgt_first_token == new_dgt_first_token and self.dgt_fen != new_dgt_fen:
                        # Update fen if only color to move has changed
                        self.dgt_fen = new_dgt_fen

                    if old_dgt_first_token != new_dgt_first_token:

                        m = self.get_legal_move(self.dgt_fen, new_dgt_fen)

                        if m:
                            self.previous_dgt_fen = self.dgt_fen
                            self.dgt_fen = new_dgt_fen
                            self.switch_turn()
                            self.move_list.append(m)
                            return m
                        else:
                            previous_dgt_fen_first_token = self.previous_dgt_fen.split()[0]
#                            print "previous_dgt_fen_first_token : {0}".format(previous_dgt_fen_first_token)
                            if new_dgt_first_token == previous_dgt_fen_first_token:
                                print "undo"
                                self.dgt_fen = self.previous_dgt_fen
                                self.move_list.pop()
                                if len(self.move_list)>0:
                                    return self.move_list[-1]

                elif new_dgt_fen:
                    self.dgt_fen = new_dgt_fen
                    self.previous_dgt_fen = new_dgt_fen
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
        sf.add_observer(self.parse_score)
        self.score_count = 0
        self.score = None
        self.engine_mode = EngineManager.PLAY
        self.engine_started = False

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

    def stop_engine(self):
        if self.engine_started:
            sf.stop()

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


    def position(self, move_list, pos='startpos'):
        sf.position(pos, move_list)
        self.move_list = move_list

    def generate_move_list(self, all_moves, eval=None, start_move_num = 1):
        score = ""
        if eval:
            score = str(eval) + " "

        for i, mv in it.izip(it.count(start_move_num), all_moves):
            move = "b"
            if i % 2 == 1:
                score += "%d." % ((i + 1) / 2)
                move = "w"
            if mv:
#                if raw:
                score += "%s " % mv
                if i % 6 == 0:
                    score += "\n"
#                else:
#                    score += " [ref=%d:%s] %s [/ref]"%((i + 1) / 2, move, mv)
        return score

    def parse_score(self, line):
        score = self.get_score(line)

        tokens = line.split()
        line_index = tokens.index('pv')
        if line_index>-1:
            pv = sf.to_san(tokens[line_index+1:])
            if len(pv)>0:
                self.score_count+=1
                if self.score_count > 5:
                    self.score_count = 0
                if piface and self.score_count==1:
                    cad.lcd.clear()
                    first_mv = tokens[line_index+1]
                    output = str(score)+' '+first_mv
                    cad.lcd.write(output)
                    #cad.lcd.write(str(score)+' ')
#                    cad.lcd.write(self.generate_move_list(pv, eval=score, start_move_num=len(self.move_list)+1))
                    print output
                else:
                    print self.generate_move_list(pv, eval=score, start_move_num=len(self.move_list)+1)
                    print "\n"

def start_dgt_thread(dgt):
    t = Thread(target=dgt_probe, args=(dgt,))
    t.daemon = True # thread dies with the program
    t.start()

def dgt_probe(dgt):
    Timer(1.0, dgt_probe, [dgt]).start()
    m = dgt.probe_move()
    if m:
        dgt_sem.release()

if __name__ == '__main__':
    if piface:
        cad = pifacecad.PiFaceCAD()
        cad.lcd.blink_off()
        cad.lcd.cursor_off()
        cad.lcd.backlight_on()
        cad.lcd.write("Pycochess 0.1")

    arm = False
    if os.uname()[4][:3] == 'arm':
        dgt = DGTBoard("/dev/ttyUSB0")
        arm = True
    else:
        dgt = DGTBoard("/dev/cu.usbserial-00001004")

    dgt.connect()
    em = EngineManager()
    dgt_probe(dgt)

    while True:
        #print "Before acquire"
        dgt_sem.acquire()
        print "Board Updated!"
        em.stop_engine()
        em.score_count = 0
        em.position(dgt.move_list, pos='startpos')
        # Needed on the Pi!
        if arm:
            sleep(1)
        sf.go(infinite=True)
        em.engine_started = True

