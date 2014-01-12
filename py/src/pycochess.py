
import traceback
from dgt.dgtnix import *
import stockfish as sf
from threading import Thread
from threading import Semaphore
from threading import Timer
from time import sleep
import itertools as it
import os

FIXED_TIME = "fixed_time"

NEW_GAME = "New Game"
BOOK_MODE = "Book Mode"
ANALYSIS_MODE = "Analysis Mode"
GAME_MODE = "Game Mode"
KIBITZ_MODE = "Kibitz Mode"
OBSERVE_MODE = "Observe Mode"

BOOK_EXTENSION = ".bin"

piface = None
try:
    import pifacecad
    piface = True
except ImportError:
    piface = False

WHITE = "w"
BLACK = "b"

BOOK_PATH="/opt/picochess/books/"

clock_mode = FIXED_TIME
# 5 seconds
comp_time = 5000
play_mode = GAME_MODE

DEFAULT_BOOK_FEN = "rnbqkbnr/pppppppp/8/8/8/5q2/PPPPPPPP/RNBQKBNR"

book_map = {
    "rnbqkbnr/pppppppp/8/8/8/q7/PPPPPPPP/RNBQKBNR": ["nobook", "No Book"],
    "rnbqkbnr/pppppppp/8/8/8/1q6/PPPPPPPP/RNBQKBNR": ["fun", "Fun"],
    "rnbqkbnr/pppppppp/8/8/8/2q5/PPPPPPPP/RNBQKBNR": ["anand", "Anand"],
    "rnbqkbnr/pppppppp/8/8/8/3q4/PPPPPPPP/RNBQKBNR": ["korchnoi", "Korchnoi"],
    "rnbqkbnr/pppppppp/8/8/8/4q3/PPPPPPPP/RNBQKBNR": ["larsen", "Larsen"],
    # Default
    "rnbqkbnr/pppppppp/8/8/8/5q2/PPPPPPPP/RNBQKBNR": ["pro", "Pro"],
    "rnbqkbnr/pppppppp/8/8/8/6q1/PPPPPPPP/RNBQKBNR": ["gm2001", "GM >2001"],
    "rnbqkbnr/pppppppp/8/8/8/7q/PPPPPPPP/RNBQKBNR": ["varied", "Varied"],
    "rnbqkbnr/pppppppp/8/8/7q/8/PPPPPPPP/RNBQKBNR": ["gm1950", "GM >1950"],
    "rnbqkbnr/pppppppp/8/8/6q1/8/PPPPPPPP/RNBQKBNR": ["performance", "Performance"],
    "rnbqkbnr/pppppppp/8/8/5q2/8/PPPPPPPP/RNBQKBNR": ["stfish", "Stockfish"]
}

game_map = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR" : NEW_GAME,
    # White Queen on a5
    "rnbqkbnr/pppppppp/8/Q7/8/8/PPPPPPPP/RNBQKBNR" : BOOK_MODE,
    # White Queen on b5
    "rnbqkbnr/pppppppp/8/1Q6/8/8/PPPPPPPP/RNBQKBNR" : ANALYSIS_MODE,
    # White Queen on c5
    "rnbqkbnr/pppppppp/8/2Q5/8/8/PPPPPPPP/RNBQKBNR" : GAME_MODE,
    # White Queen on d5
    "rnbqkbnr/pppppppp/8/3Q4/8/8/PPPPPPPP/RNBQKBNR" : KIBITZ_MODE,
    # White Queen on e5
    "rnbqkbnr/pppppppp/8/4Q3/8/8/PPPPPPPP/RNBQKBNR" : OBSERVE_MODE

}

level_map = ["rnbqkbnr/pppppppp/q7/8/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/1q6/8/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/2q5/8/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/3q4/8/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/4q3/8/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/5q2/8/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/6q1/8/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/7q/8/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/q7/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/1q6/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/2q5/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/3q4/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/4q3/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/5q2/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/6q1/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/7q/8/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/8/q7/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/8/1q6/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/8/2q5/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/8/3q4/8/PPPPPPPP/RNBQKBNR",
             "rnbqkbnr/pppppppp/8/8/4q3/8/PPPPPPPP/RNBQKBNR"
            ]

time_control_map = {
    "rnbqkbnr/pppppppp/Q7/8/8/8/PPPPPPPP/RNBQKBNR": [0, "1 second per move"],
    "rnbqkbnr/pppppppp/1Q6/8/8/8/PPPPPPPP/RNBQKBNR": [1, "3 seconds per move"],
    "rnbqkbnr/pppppppp/2Q5/8/8/8/PPPPPPPP/RNBQKBNR" : [2, "5 seconds per move"],
    "rnbqkbnr/pppppppp/3Q4/8/8/8/PPPPPPPP/RNBQKBNR" : [3, "10 seconds per move"],
    "rnbqkbnr/pppppppp/4Q3/8/8/8/PPPPPPPP/RNBQKBNR" : [4, "15 seconds per move"],
    "rnbqkbnr/pppppppp/5Q2/8/8/8/PPPPPPPP/RNBQKBNR" : [5, "30 seconds per move"],
    "rnbqkbnr/pppppppp/6Q1/8/8/8/PPPPPPPP/RNBQKBNR" : [6, "60 seconds per move"],
    "rnbqkbnr/pppppppp/7Q/8/8/8/PPPPPPPP/RNBQKBNR" : [7, "120 seconds per move"],
    "rnbqkbnr/pppppppp/8/8/Q7/8/PPPPPPPP/RNBQKBNR" : [8, "Game in 1 minute"],
    "rnbqkbnr/pppppppp/8/8/1Q6/8/PPPPPPPP/RNBQKBNR" : [9, "Game in 3 minutes"],
    "rnbqkbnr/pppppppp/8/8/2Q5/8/PPPPPPPP/RNBQKBNR" : [10, "Game in 5 minutes"],
    "rnbqkbnr/pppppppp/8/8/3Q4/8/PPPPPPPP/RNBQKBNR" : [11, "Game in 10 minutes"],
    "rnbqkbnr/pppppppp/8/8/4Q3/8/PPPPPPPP/RNBQKBNR" : [12, "Game in 15 minutes"],
    "rnbqkbnr/pppppppp/8/8/5Q2/8/PPPPPPPP/RNBQKBNR" : [13, "Game in 30 minutes"],
    "rnbqkbnr/pppppppp/8/8/6Q1/8/PPPPPPPP/RNBQKBNR" : [14, "Game in 60 minutes"],
    "rnbqkbnr/pppppppp/8/8/7Q/8/PPPPPPPP/RNBQKBNR" : [15, "Game in 90 minutes"],
    "rnbqkbnr/pppppppp/8/8/8/Q7/PPPPPPPP/RNBQKBNR" : [16, "Game in 3 + 2s"],
    "rnbqkbnr/pppppppp/8/8/8/1Q6/PPPPPPPP/RNBQKBNR" : [17, "Game in 4 + 2s"],
    "rnbqkbnr/pppppppp/8/8/8/2Q5/PPPPPPPP/RNBQKBNR" : [18, "Game in 5 + 3s"],
    "rnbqkbnr/pppppppp/8/8/8/3Q4/PPPPPPPP/RNBQKBNR" : [19, "Game in 5 + 5s"],
    "rnbqkbnr/pppppppp/8/8/8/5Q2/PPPPPPPP/RNBQKBNR" : [20, "H Game in 7 + 1s"],
    "rnbqkbnr/pppppppp/8/8/8/4Q3/PPPPPPPP/RNBQKBNR" : [21, "Game in 15 + 5s"],
    "rnbqkbnr/pppppppp/8/8/8/6Q1/PPPPPPPP/RNBQKBNR" : [22, "Game in 90 + 30s"]

}

dgt_sem = Semaphore(value=0)

def write_to_piface(message, clear = False):
    if piface:
        if clear:
            cad.lcd.clear()
        cad.lcd.write(message)

class DGTBoard(object):

    def __init__(self, device, **kwargs):
        self.dgtnix = None
        self.dgt_fen = None
        self.dgt_connected = False
        self.device = device
        self.turn = WHITE
        self.board_updated = False
        self.move_list = []
        self.executed_command = False

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

    def start_new_game(self):
        if piface:
            cad.lcd.blink_off()
            cad.lcd.cursor_off()
            cad.lcd.backlight_off()
            cad.lcd.backlight_on()
            cad.lcd.write("New Game")


    def check_for_command_fen(self, fen):
        if book_map.has_key(fen):
            filepath = BOOK_PATH + book_map[fen][0] + BOOK_EXTENSION
            print "book filepath : {0}".format(filepath)
            sf.set_option("Book File", filepath)
            write_to_piface("Book:\n "+book_map[fen][1], clear=True)
            # Return true so that engine does not think if merely the opening book is changed
            return True
        elif game_map.has_key(fen):
            if game_map[fen] == NEW_GAME:
#                self.start_new_game()
                return False
            else:
                play_mode = game_map[fen]
                return True

        else:
            try:
                level = level_map.index(fen)
                sf.set_option("Skill Level", level)
                write_to_piface("Now on Level "+str(level), clear=True)
                return True
            except ValueError:
                return False

#        elif time_control_map.has_key(fen):



    def probe_move(self, *args):
        if self.dgt_connected and self.dgtnix:
            try:
                new_dgt_fen = self.dgtnix.getFen(color=self.turn)

                if self.dgt_fen and new_dgt_fen:
                    old_dgt_first_token = self.dgt_fen.split()[0]
                    new_dgt_first_token = new_dgt_fen.split()[0]


                    if old_dgt_first_token == new_dgt_first_token and self.dgt_fen != new_dgt_fen:
                        # Update fen if only color to move has changed
                        self.dgt_fen = new_dgt_fen

                    if old_dgt_first_token != new_dgt_first_token:
#                        print "old_dgt_first_token: {0}".format(old_dgt_first_token)
#                        print "new_dgt_first_token: {0}".format(new_dgt_first_token)

                        if self.check_for_command_fen(new_dgt_first_token):
                            self.previous_dgt_fen = self.dgt_fen
                            self.dgt_fen = new_dgt_fen

                            # Return no legal move if its a command FEN, we simply change levels, books, time, options,
                        # and there is no need to process a move
                            return False

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
                                if len(self.move_list)>0:
                                    self.move_list.pop()
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

    def parse_bestmove(self, line):
    #        print "line:{0}".format(line)
        best_move = None
        ponder_move = None
        if not line.startswith('bestmove'):
            return best_move, ponder_move
        tokens = line.split()

        try:
            bm_index = tokens.index('bestmove')
            ponder_index = tokens.index('ponder')
        except ValueError:
            bm_index = -1
            ponder_index = -1

        if bm_index!=-1:
            best_move = tokens[bm_index+1]

        if ponder_index!=-1:
            ponder_move = tokens[ponder_index+1]

        return best_move, ponder_move

    def parse_score(self, line):
        print line
        tokens = line.split()
        score = self.get_score(line)

        if play_mode == ANALYSIS_MODE:


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
        elif play_mode == GAME_MODE:
            best_move, ponder_move = self.parse_bestmove(line)
            if best_move:
#                print "best_move:{0}".format(best_move)
#                print "best_move_san:{0}".format(sf.to_san([best_move])[0])
                write_to_piface(sf.to_san([best_move])[0], clear=True)




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

        # Lets assume this is the raspberry Pi for now..

        sf.set_option("OwnBook", "true")

        # In case someone has the pi rev A
        sf.set_option("Hash", 128)
        sf.set_option("Emergency Base Time", 1300)
        sf.set_option("Book File", BOOK_PATH+book_map[DEFAULT_BOOK_FEN][0]+ BOOK_EXTENSION)


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
            if play_mode == ANALYSIS_MODE:
                sf.go(infinite=True)
            elif play_mode == GAME_MODE:
                sf.go(movetime=comp_time)
            em.engine_started = True

