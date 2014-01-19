from Queue import Queue
import traceback
import stockfish as sf
from threading import Thread
from threading import Semaphore
from threading import Timer
from time import sleep
import datetime
import itertools as it
import os
from pydgt import DGTBoard
from pydgt import FEN

FIXED_TIME = "fixed_time"
BLITZ = "blitz"
BLITZ_FISCHER = "blitz_fischer"

NEW_GAME = "New Game"
BOOK_MODE = "Book Mode"
ANALYSIS_MODE = "Analysis Mode"
GAME_MODE = "Game Mode"
KIBITZ_MODE = "Kibitz Mode"
OBSERVE_MODE = "Observe Mode"

BOOK_EXTENSION = ".bin"
try:
    import pyfiglet
    figlet = pyfiglet.Figlet()
    print figlet.renderText("Pycochess 0.1")
except ImportError:
    figlet = None
    print "No pyfiglet"

piface = None
try:
    import pifacecad
    piface = True
except ImportError:
    piface = False

WHITE = "w"
BLACK = "b"

BOOK_PATH="/opt/picochess/books/"
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

#dgt_sem = Semaphore(value=0)
dgt_queue = Queue()

class Pycochess(object):

    ANALYSIS = "Analysis"
    PLAY = "Play"
    OBSERVE = "Observe"

    def __init__(self, device, **kwargs):
        self.dgt = None
        self.dgt_fen = None
        self.dgt_connected = False
        self.device = device
        self.turn = WHITE
        self.board_updated = False
        self.move_list = []
        self.executed_command = False

        self.engine_comp_color = BLACK

        self.time_white = 0
        self.time_inc_white = 0
        self.time_black = 0
        self.time_inc_black = 0
        self.time_last = None

        # Engine specific stuff
        sf.add_observer(self.parse_score)
        self.score_count = 0
        self.score = None
        self.engine_mode = Pycochess.PLAY
        self.engine_searching = False

        # Game specific stuff
        self.clock_mode = FIXED_TIME
        self.play_mode = GAME_MODE

        # 5 seconds
        self.comp_time = 5000
        self.comp_inc = 0
        self.player_time = 0
        self.player_inc = 0
        self.exec_comp_move = False
        self.engine_computer_move = False

        # Help user execute comp moves
        self.computer_move_FEN_reached = False
        self.computer_move_FEN = ""

    def write_to_piface(self, message, clear = False):
        if piface:
            if clear:
                cad.lcd.clear()
            cad.lcd.write(message)

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
        if self.dgt:
            del self.dgt
        self.dgt_connected = False

    def fen_to_move(self, fen, color):
        return fen.replace(''+WHITE+'', color)

    def on_observe_dgt_move(self, attr):
        if attr.type == FEN:
            fen = attr.message
            print "Fen: {0}".format(fen)
            m = pyco.probe_move(fen)
            if m:
                dgt_queue.put(m)
        #        dgt_sem.release()

    def poll_dgt(self):
        thread = Thread(target=self.dgt.poll)
        thread.start()

    def connect(self):
        self.dgt = DGTBoard(self.device)
        self.dgt.subscribe(self.on_observe_dgt_move)
        self.poll_dgt()

        # board.poll()

        # self.dgt = dgtnix("dgt/libdgtnix.so")
        # self.dgt.SetOption(dgtnix.DGTNIX_DEBUG, dgtnix.DGTNIX_DEBUG_ON)
        # Initialize the driver with port argv[1]
        # result = self.dgt.Init(self.device)
        if not self.dgt:
            print "Unable to connect to the device on {0}".format(self.device)
        else:
            print "The board was found"
            self.dgt_connected = True


    def switch_turn(self):
#        print "prev_turn : {0}".format(self.turn)
        if self.turn == WHITE:
            self.turn = BLACK
        elif self.turn == BLACK:
            self.turn = WHITE
#        print "turn : {0}".format(self.turn)


    def start_new_game(self):
        self.engine_computer_move = False
        # Help user execute comp moves
        self.computer_move_FEN_reached = False
        self.computer_move_FEN = ""
        self.move_list = []
        self.turn = WHITE

        if piface:
            cad.lcd.blink_off()
            cad.lcd.cursor_off()
            cad.lcd.backlight_off()
            cad.lcd.backlight_on()
            self.write_to_piface("New Game", clear=True)


    def check_for_command_fen(self, fen):

        if book_map.has_key(fen):
            filepath = BOOK_PATH + book_map[fen][0] + BOOK_EXTENSION
            print "book filepath : {0}".format(filepath)
            sf.set_option("Book File", filepath)
            self.write_to_piface("Book:\n "+book_map[fen][1], clear=True)
            # Return true so that engine does not think if merely the opening book is changed
            return True
        elif game_map.has_key(fen):
            if game_map[fen] == NEW_GAME:
                if len(self.move_list) > 0:
                    self.start_new_game()
                    return True
            else:
                play_mode = game_map[fen]
                return True
        elif time_control_map.has_key(fen):
            mode = time_control_map[fen][0]
            message = time_control_map[fen][1]

#            print "time_control_mode: {0}".format(mode)
#            print "time_control_message: {0}".format(message)

            if 0 <= mode <= 7:
                self.clock_mode = FIXED_TIME

                if mode == 0:
                    self.comp_time = 1000
                elif mode == 1:
                    self.comp_time = 3000
                elif mode == 2:
                    self.comp_time = 5000
                elif mode == 3:
                    self.comp_time = 10000
                elif mode == 4:
                    self.comp_time = 15000
                elif mode == 5:
                    self.comp_time = 30000
                elif mode == 6:
                    self.comp_time = 60000
                elif mode == 7:
                    self.comp_time = 120000
            elif 8 <= mode <= 15:
                self.clock_mode = BLITZ

                if mode == 8:
                    self.comp_time = 60000
                elif mode == 9:
                    self.comp_time = 180000
                elif mode == 10:
                    self.comp_time = 300000
                elif mode == 11:
                    self.comp_time = 600000
                elif mode == 12:
                    self.comp_time = 900000
                elif mode == 13:
                    self.comp_time = 1800000
                elif mode == 14:
                    self.comp_time = 3600000
                elif mode == 15:
                    self.comp_time = 5400000
            elif 16 <= mode <= 22:
                self.clock_mode = BLITZ_FISCHER

                if mode == 16:
                    self.comp_time = 3 * 60 * 1000
                    self.comp_inc = 2 * 1000
                elif mode == 17:
                    self.comp_time = 4 * 60 * 1000
                    self.comp_inc = 2 * 1000
                elif mode == 18:
                    self.comp_time = 5 * 60 * 1000
                    self.comp_inc = 3 * 1000
                elif mode == 19:
                    self.comp_time = 5 * 60 * 1000
                    self.comp_inc = 5 * 1000
                elif mode == 20:
                    # Handicap time control
                    # Seems to work well for training
                    # Player has 7m + 10s increment
                    # Computer has 1m + 3s increment
                    self.comp_time = 1 * 60 * 1000
                    self.comp_inc = 3 * 1000
                    self.player_time = 7 * 60 * 1000
                    self.player_inc = 10 * 1000
                elif mode == 21:
                    self.comp_time = 15 * 60 * 1000
                    self.comp_inc = 5 * 1000
                elif mode == 22:
                    self.comp_time = 90 * 60 * 1000
                    self.comp_inc = 30 * 1000

            self.write_to_piface(message, clear=True)
            return True

        else:
            try:
                level = level_map.index(fen)
                sf.set_option("Skill Level", level)
                self.write_to_piface("Now on Level "+str(level), clear=True)
                return True
            except ValueError:
                return False

#        elif time_control_map.has_key(fen):

    def reset_clocks(self):
#        self.white_time_now = time.clock()
#        self.black_time_now = time.clock()
        self.time_last = datetime.datetime.now()

        self.time_white = 60
        self.time_inc_white = 3
        self.time_black = 420
        self.time_inc_black = 8
        if self.engine_comp_color == 'b':
            # Swap time allotments if comp is black (comp gets less time)
            self.time_white, self.time_black = self.time_black, self.time_white
            self.time_inc_white, self.time_inc_black = self.time_inc_black, self.time_inc_white

    def update_time(self, color='w'):
        if self.time_last:
            current = datetime.datetime.now()
            seconds_elapsed = (current - self.time_last).total_seconds()
    #        print "seconds_elapsed:{0}".format(seconds_elapsed)
            self.time_last = current
            if color == 'w':
                self.time_white -= seconds_elapsed*1000
            else:
                self.time_black -= seconds_elapsed*1000

    def reset_clock_update(self):
        self.time_last = datetime.datetime.now()

    def time_add_increment(self, color='w'):
        if color == 'w':
            self.time_white+=self.time_inc_white
        else:
            self.time_black+=self.time_inc_black

    def update_player_time(self):
        color = 'w'
        if self.engine_comp_color == 'w':
            color = 'b'
        self.update_time(color=color)

    def format_time_str(self,time_a, separator='.'):
        return "%d%s%02d" % (int(time_a/60), separator, int(time_a%60))

    def format_fixed_time_str(self, time_a):
#        print "fixed_time_str: {0}".format(time_a)
        time_a = int(time_a)
        return "  {0}".format(time_a/1000)

    def update_clocks(self, *args):
        if self.play_mode == GAME_MODE:
            if self.engine_computer_move:
                self.update_time(color=self.engine_comp_color)
#                print "comp_time: {0}".format(self.time_black)

                if self.clock_mode == BLITZ or self.clock_mode == BLITZ_FISCHER:
                    self.write_to_piface(self.format_time_str(self.time_white) + self.format_time_str(self.time_black), clear=True)
                elif self.clock_mode == FIXED_TIME and self.engine_searching:
                    # If FIXED_TIME
                    if self.engine_comp_color == WHITE:
#                        print "comp_time: {0}".format(self.time_white)
                        if self.time_white and self.time_white>=1000:
                            self.write_to_piface(self.format_fixed_time_str(self.time_white), clear = True)
                    else:
#                        print "comp_time: {0}".format(self.time_black)
                        if self.time_black and self.time_black>=1000:
                            self.write_to_piface(self.format_fixed_time_str(self.time_black), clear = True)

                        # self.engine_score.children[0].text = "[color=000000]Thinking..\n[size=24]{0}    [b]{1}[/size][/b][/color]".format(self.format_time_str(self.time_white), self.format_time_str(self.time_black))
            else:
                if not self.exec_comp_move:
                    self.update_player_time()
                    if self.clock_mode == BLITZ or self.clock_mode == BLITZ_FISCHER:
                        self.write_to_piface(self.format_time_str(self.time_white) + self.format_time_str(self.time_black), clear=True)

                # if self.show_hint:
                #     if not self.ponder_move_san and self.ponder_move and self.ponder_move!='(none)':
                #         # print self.ponder_move
                #         try:
                #             self.ponder_move_san = sf.toSAN([self.ponder_move])[0]
                #             # print "ponder_move_san: "+self.ponder_move_san
                #             # if not self.spoke_hint:
                #             #     self.spoke_hint = True
                #             #     self.speak_move(self.ponder_move)
                #         except IndexError:
                #             self.ponder_move_san = "None"
                #     if self.ponder_move_san:
                #         self.engine_score.children[0].text = YOURTURN_MENU.format(self.ponder_move_san, self.eng_eval, self.format_time_str(self.time_white), self.format_time_str(self.time_black))
                #         if not self.spoke_hint:
                #             self.spoke_hint = True
                #             self.speak_move(self.ponder_move, immediate=True)
                #     else:
                #         self.engine_score.children[0].text = YOURTURN_MENU.format("Not available", self.eng_eval, self.format_time_str(self.time_white), self.format_time_str(self.time_black))
                # else:
                #     self.engine_score.children[0].text = YOURTURN_MENU.format("hidden", "hidden", self.format_time_str(self.time_white), self.format_time_str(self.time_black))


    def probe_move(self, fen, *args):
        if self.dgt_connected and self.dgt:
            try:
                new_dgt_fen = fen # color
#                print "mod_fen : {0}".format(fen)

#                print "old_dgt_fen: {0}".format(self.dgt_fen)
#                print "new_dgt_fen: {0}".format(new_dgt_fen)


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

                        computer_move_first_tok = None
                        if self.computer_move_FEN:
                            computer_move_first_tok = self.computer_move_FEN.split()[0]


                        if not self.engine_searching and not self.computer_move_FEN_reached and computer_move_first_tok and computer_move_first_tok == new_dgt_first_token:
                            self.computer_move_FEN_reached = True
#                            self.switch_turn()
#                            print "computer move Fen reached"


#                        print "old_dgt_fen: {0}".format(self.dgt_fen)
#                        print "new_dgt_fen: {0}".format(new_dgt_fen)
                        m = self.get_legal_move(self.dgt_fen, new_dgt_fen)
                        if not m:
                            # If the user made a quick move, try to see if the current position is playable from the computer_move_FEN
                            if self.is_fen(self.computer_move_FEN):
                                m = self.get_legal_move(self.computer_move_FEN, new_dgt_fen)


                        if m:
#                            print "Move: {0}".format(m)
                            self.previous_dgt_fen = self.dgt_fen
                            self.switch_turn()
                            new_dgt_fen = self.fen_to_move(new_dgt_fen, self.turn)

                            self.dgt_fen = new_dgt_fen
                            self.move_list.append(m)

                            if not self.engine_computer_move:
                                self.engine_computer_move = True

                            # Start player clock!
                            return m
                        else:
                            previous_dgt_fen_first_token = self.previous_dgt_fen.split()[0]
#                            print "previous_dgt_fen_first_token : {0}".format(previous_dgt_fen_first_token)
                            if new_dgt_first_token == previous_dgt_fen_first_token:
#                                print "undo"

                                self.dgt_fen = self.previous_dgt_fen
                                if len(self.move_list)>0:
                                    self.move_list.pop()
                                    return "undo"
#                                    return self.move_list[-1]

                elif new_dgt_fen:
                    self.dgt_fen = new_dgt_fen
                    self.previous_dgt_fen = new_dgt_fen
#                if play_mode == GAME_MODE and self.engine_computer_move:
                    # Print engine move on DGT XL clock
#                    self.dgtnix.SendToClock(self.format_str_for_dgt(self.format_time_str(self.time_white,separator='')+self.      format_time_str(self.time_black, separator='')), False, True)
            except Exception:
                self.dgt_connected = False
                self.dgt=None
                print traceback.format_exc()

    def stop_engine(self):
        if self.engine_searching:
            self.engine_searching = False
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

        if self.play_mode == ANALYSIS_MODE:

            line_index = tokens.index('pv')
            if line_index>-1:
                pv = sf.to_san(tokens[line_index+1:])
                if len(pv)>0:
                    self.score_count+=1
                    if self.score_count > 5:
                        self.score_count = 0
                    if piface and self.score_count==1:
                        first_mv = tokens[line_index+1]
                        output = str(score)+' '+first_mv
                        self.write_to_piface(output, clear = True)
                        #cad.lcd.write(str(score)+' ')
                        #                    cad.lcd.write(self.generate_move_list(pv, eval=score, start_move_num=len(self.move_list)+1))
                        print output
                    else:
                        print self.generate_move_list(pv, eval=score, start_move_num=len(self.move_list)+1)
                        print "\n"
        elif self.play_mode == GAME_MODE:
            best_move, ponder_move = self.parse_bestmove(line)
            if best_move:
            #                print "best_move:{0}".format(best_move)
            #                print "best_move_san:{0}".format(sf.to_san([best_move])[0])
                output_move = sf.to_san([best_move])[0]
                if output_move:
                    if figlet:
                        print figlet.renderText(output_move)
                    else:
                        print "SAN best_move: {0}".format(output_move)
                    output_move = " I play "+output_move
                    self.write_to_piface(output_move, clear=True)

                self.exec_comp_move = True
                self.engine_computer_move = False
                self.computer_move_FEN_reached = False
                self.computer_move_FEN = sf.get_fen(self.dgt_fen, [best_move])
#                print "prev dgt_fen: {0}".format(self.dgt_fen)
#                print "computer_move_FEN: {0}".format(self.computer_move_FEN)
                self.engine_searching = False

    def eng_process_move(self):
        self.stop_engine()
        self.score_count = 0
        self.position(self.move_list, pos='startpos')
        # Needed on the Pi!
        if arm:
            sleep(1)
        if self.play_mode == ANALYSIS_MODE:
            sf.go(infinite=True)
        elif self.play_mode == GAME_MODE:
            if self.engine_computer_move:
                if self.clock_mode == FIXED_TIME:
                    if self.engine_comp_color == BLACK:
                    #                        print "comp_time: {0}".format(comp_time)
                        self.time_black = self.comp_time
                    #                        print "dgt_time: {0}".format(dgt.btime)

                    else:
                        self.time_white = self.comp_time
                    sf.go(movetime=self.comp_time)
                    self.reset_clock_update()
                else:
                    if not self.player_time:
                        player_time = self.comp_time
                    if not self.player_inc:
                        player_inc = pyco.comp_inc
                    wtime = self.comp_time
                    winc = self.comp_inc
                    btime = self.player_time
                    binc = self.player_inc

                    if self.engine_comp_color == BLACK:
                        wtime, btime = btime, wtime
                        winc, binc = binc, winc

                    if self.clock_mode == BLITZ:
                        sf.go(wtime=int(wtime), btime=int(btime))
                    elif self.clock_mode == BLITZ_FISCHER:
                        sf.go(wtime=int(wtime), btime=int(btime), winc=int(winc), binc=int(binc))
                    self.reset_clock_update()
        self.engine_searching = True

    def is_fen(self, fen):
        return len(fen.split()) == 6

def update_clocks(pyco):
    Timer(1.0, update_clocks, [pyco]).start()
    pyco.update_clocks()

#    m = pyco.probe_move()
##    print "move: {0}".format(m)
#    if m:
#        dgt_queue.put(m)
##        dgt_sem.release()



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
        pyco = Pycochess("/dev/ttyUSB0")
        arm = True
    else:
        pyco = Pycochess("/dev/cu.usbserial-00001004")

    pyco.connect()
    update_clocks(pyco)

    while True:
        #print "Before acquire"
        m = dgt_queue.get()
#        print "Board Updated!"
        process_move = True
        if m == "undo":
            pyco.write_to_piface(pyco.move_list[-1], clear=True)
            process_move = False

        elif pyco.computer_move_FEN_reached:
            print "Comp_move FEN reached"
#            pyco.write_to_piface("Done", clear=True)
            m = dgt_queue.get()
#            print "Next dgt_get after comp_move fen reached"
            process_move = True

        if process_move:
            pyco.eng_process_move()

