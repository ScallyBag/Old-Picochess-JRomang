#!/usr/bin/python

from Queue import Queue
import traceback
import stockfish as sf
from threading import Thread, RLock
from threading import Timer
from time import sleep
import datetime
import itertools as it
import os
import subprocess
import sys
import socket

from ChessBoard import ChessBoard
from pydgt import DGTBoard
from pydgt import FEN
from pydgt import CLOCK_BUTTON_PRESSED
from pydgt import CLOCK_ACK
from polyglot_opening_book import PolyglotOpeningBook

START_GAME_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"

FORCE_MOVE = "Force"

START_ALT_INPUT = ['a', '1', 'a', '1']

FIXED_TIME = "fixed_time"
BLITZ = "blitz"
BLITZ_FISCHER = "blitz_fischer"

NEW_GAME = "New Game"
BOOK_MODE = "Book Mode"
ANALYSIS_MODE = "Analysis Mode"
GAME_MODE = "Game Mode"
KIBITZ_MODE = "Kibitz Mode"
OBSERVE_MODE = "Observe Mode"

VERSION = "0.22"

BOOK_EXTENSION = ".bin"
try:
    import pyfiglet
    figlet = pyfiglet.Figlet()
    print figlet.renderText("Pycochess {0}".format(VERSION))
except ImportError:
    figlet = None
    print "No pyfiglet"

piface = None
arduino = False
dgt_clock = False

try:
    import pifacecad
    piface = True
except ImportError:
    piface = False
    try:
        import nanpy
        arduino = True
    except ImportError:
        arduino = False

WHITE = "w"
BLACK = "b"

if piface:
    PROG_PATH = "/home/miniand/git/Stockfish"
else:
    PROG_PATH = os.path.realpath("..")

BOOK_PATH = "/opt/picochess/books/"
DEFAULT_BOOK_FEN = "rnbqkbnr/pppppppp/8/8/8/5q2/PPPPPPPP/RNBQKBNR"
DEFAULT_BOOK_INDEX = 5
COMP_PLAYS_WHITE = "rnbq1bnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"
# Default
COMP_PLAYS_BLACK = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQ1BNR"

book_list = [("nobook", "No Book"), ("fun", "Fun"), ("anand", "Anand"), ("korchnoi", "Korchnoi"), ("larsen", "Larsen"),
             ("pro", "Pro"), ("gm2001", "GM >2001"), ("varied", "Varied"),
             ("gm1950", "GM >1950"), ("performance", "Performance"), ("stfish", "Stockfish")]

max_book_index = len(book_list) - 1

book_map = {
    "rnbqkbnr/pppppppp/8/8/8/q7/PPPPPPPP/RNBQKBNR": book_list[0],
    "rnbqkbnr/pppppppp/8/8/8/1q6/PPPPPPPP/RNBQKBNR": book_list[1],
    "rnbqkbnr/pppppppp/8/8/8/2q5/PPPPPPPP/RNBQKBNR": book_list[2],
    "rnbqkbnr/pppppppp/8/8/8/3q4/PPPPPPPP/RNBQKBNR": book_list[3],
    "rnbqkbnr/pppppppp/8/8/8/4q3/PPPPPPPP/RNBQKBNR": book_list[4],
    # Default
    "rnbqkbnr/pppppppp/8/8/8/5q2/PPPPPPPP/RNBQKBNR": book_list[5],
    "rnbqkbnr/pppppppp/8/8/8/6q1/PPPPPPPP/RNBQKBNR": book_list[6],
    "rnbqkbnr/pppppppp/8/8/8/7q/PPPPPPPP/RNBQKBNR": book_list[7],
    "rnbqkbnr/pppppppp/8/8/7q/8/PPPPPPPP/RNBQKBNR": book_list[8],
    "rnbqkbnr/pppppppp/8/8/6q1/8/PPPPPPPP/RNBQKBNR": book_list[9],
    "rnbqkbnr/pppppppp/8/8/5q2/8/PPPPPPPP/RNBQKBNR": book_list[10]
}

game_map = {
    START_GAME_FEN: NEW_GAME,
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

time_control_list = ["1 sec per move", "3 sec per move", "5 sec per move", "10 sec per move",
                     "15 sec per move", "30 sec per move", "60 sec per move", "120 sec per move",
                     "Game in 1 min", "Game in 3 mins", "Game in 5 mins", "Game in 10 mins",  "Game in 15 mins",
                     "Game in 30 mins", "Game in 60 mins", "Game in 90 mins", "Game in 3 mins + 2s",
                     "Game in 4 mins + 2s", "Game in 5 mins + 3s", "Game in 5 mins + 5s", "Handicap G/7 mins + 1s",
                     "Game in 15 mins + 5s", "Game in 90 mins + 30s"
                     ]

dgt_time_control_list = ["mov001", "mov003", "mov005", "mov010",
                     "mov015", "mov030", "mov060", "mov120",
                     "bli100", "bli300", "bli500", "bli000",  "bli500",
                     "bli030", "bli100", "bli130", "f 32  ",
                     "f 42  ", "f 53  ", "f 55  ", "h 71  ",
                     "f155  ", "f9030  "
                     ]


max_time_control_index = len(time_control_list) - 1

time_control_map = {
    "rnbqkbnr/pppppppp/Q7/8/8/8/PPPPPPPP/RNBQKBNR": (0, time_control_list[0], dgt_time_control_list[0]),
    "rnbqkbnr/pppppppp/1Q6/8/8/8/PPPPPPPP/RNBQKBNR": (1, time_control_list[1], dgt_time_control_list[1]),
    "rnbqkbnr/pppppppp/2Q5/8/8/8/PPPPPPPP/RNBQKBNR" : (2, time_control_list[2], dgt_time_control_list[2]),
    "rnbqkbnr/pppppppp/3Q4/8/8/8/PPPPPPPP/RNBQKBNR" : (3, time_control_list[3], dgt_time_control_list[3]),
    "rnbqkbnr/pppppppp/4Q3/8/8/8/PPPPPPPP/RNBQKBNR" : (4, time_control_list[4], dgt_time_control_list[4]),
    "rnbqkbnr/pppppppp/5Q2/8/8/8/PPPPPPPP/RNBQKBNR" : (5, time_control_list[5], dgt_time_control_list[5]),
    "rnbqkbnr/pppppppp/6Q1/8/8/8/PPPPPPPP/RNBQKBNR" : (6, time_control_list[6], dgt_time_control_list[6]),
    "rnbqkbnr/pppppppp/7Q/8/8/8/PPPPPPPP/RNBQKBNR" :  (7, time_control_list[7], dgt_time_control_list[7]),
    "rnbqkbnr/pppppppp/8/8/Q7/8/PPPPPPPP/RNBQKBNR" :  (8, time_control_list[8], dgt_time_control_list[8]),
    "rnbqkbnr/pppppppp/8/8/1Q6/8/PPPPPPPP/RNBQKBNR" : (9, time_control_list[9], dgt_time_control_list[9]),
    "rnbqkbnr/pppppppp/8/8/2Q5/8/PPPPPPPP/RNBQKBNR" : (10, time_control_list[10], dgt_time_control_list[10]),
    "rnbqkbnr/pppppppp/8/8/3Q4/8/PPPPPPPP/RNBQKBNR" : (11, time_control_list[11], dgt_time_control_list[11]),
    "rnbqkbnr/pppppppp/8/8/4Q3/8/PPPPPPPP/RNBQKBNR" : (12, time_control_list[12], dgt_time_control_list[12]),
    "rnbqkbnr/pppppppp/8/8/5Q2/8/PPPPPPPP/RNBQKBNR" : (13, time_control_list[13], dgt_time_control_list[13]),
    "rnbqkbnr/pppppppp/8/8/6Q1/8/PPPPPPPP/RNBQKBNR" : (14, time_control_list[14], dgt_time_control_list[14]),
    "rnbqkbnr/pppppppp/8/8/7Q/8/PPPPPPPP/RNBQKBNR" :  (15, time_control_list[15], dgt_time_control_list[15]),
    "rnbqkbnr/pppppppp/8/8/8/Q7/PPPPPPPP/RNBQKBNR" :  (16, time_control_list[16], dgt_time_control_list[16]),
    "rnbqkbnr/pppppppp/8/8/8/1Q6/PPPPPPPP/RNBQKBNR" : (17, time_control_list[17], dgt_time_control_list[17]),
    "rnbqkbnr/pppppppp/8/8/8/2Q5/PPPPPPPP/RNBQKBNR" : (18, time_control_list[18], dgt_time_control_list[18]),
    "rnbqkbnr/pppppppp/8/8/8/3Q4/PPPPPPPP/RNBQKBNR" : (19, time_control_list[19], dgt_time_control_list[19]),
    "rnbqkbnr/pppppppp/8/8/8/5Q2/PPPPPPPP/RNBQKBNR" : (20, time_control_list[20], dgt_time_control_list[20]),
    "rnbqkbnr/pppppppp/8/8/8/4Q3/PPPPPPPP/RNBQKBNR" : (21, time_control_list[21], dgt_time_control_list[21]),
    "rnbqkbnr/pppppppp/8/8/8/6Q1/PPPPPPPP/RNBQKBNR" : (22, time_control_list[22], dgt_time_control_list[22])

}

class ButtonEvent:
    def __init__(self, pin_num):
        self.pin_num = pin_num

# Menus accessible via piface and perhaps other devices
class PlayMode:
    GAME, ANALYSIS, KIBITZ, TRAINING, SILENT = range(5)


class ClockMode:
    FIXEDTIME, INFINITE, TOURNAMENT, BLITZ, BLITZFISCHER, SPECIAL = range(6)


class SystemMenu:
    length = 5
    IP, VER, UPDATE, RESTART, SHUTDOWN = range(length)


class PositionMenu:
    length = 5
    TO_MOVE_TOGGLE, COMP_PLAY_TOGGLE, REVERSE_ORIENTATION, SPACER, SCAN_POSITION = range(length)


class PlayMenu:
    LAST_MOVE, HINT, EVAL, SILENT, SWITCH_MODE = range(5)


class EngineMenu:
    LEVEL, BOOK, TIME, TBASES, ENG_INFO = range(5)

class AltInputMenu:
    length = 5
    FIRST, SECOND, THIRD, FOURTH, VALIDATE = range(length)

class DatabaseMenu:
    length = 5
    FIRST_GAME, PREV_GAME, NEXT_GAME, LOAD_GAME, FILLER = range(length)

class MenuRotation:
    length = 6
    MAIN, POSITION, DATABASE, ALT_INPUT, SYSTEM, ENGINE = range(length)

move_queue = Queue()


class Pycochess(object):

    ANALYSIS = "Analysis"
    PLAY = "Play"
    OBSERVE = "Observe"

    def __init__(self, device, **kwargs):
        self.alt_input_entry = START_ALT_INPUT
        self.use_tb = False
        self.clock_ack_queue = Queue()
        self.dgt_clock_lock = RLock()

        sf.set_option('SyzygyPath', '/home/pi/syzygy/')
        sf.set_option('SyzygyProbeLimit', 0)

        self.level = 20
        self.book_index = DEFAULT_BOOK_INDEX

        self.current_menu = MenuRotation.MAIN
        self.pyfish_fen = 'startpos'
        self.pyfish_castling_fen = None
        self.dgt = None
        self.current_fen = None
        self.dgt_fen = None
        self.dgt_connected = False
        self.device = device
        self.turn = WHITE
        self.board_updated = False
        self.move_list = []
        self.san_move_list = []
        self.executed_command = False
        self.pgn_file = open(PROG_PATH+'/py/game.pgn', 'w', 0)
        self.rewrite_pgn = False
        self.silent = False
        self.first_dgt_fen = None

        self.engine_comp_color = BLACK

        self.time_white = 0
        self.time_inc_white = 0
        self.time_black = 0
        self.time_inc_black = 0
        self.time_last = None

        # Engine specific stuff
        self.score_count = 0
        self.score = None
        self.engine_mode = Pycochess.PLAY
        self.engine_searching = False
        self.ponder_move = None

        # Game specific stuff
        self.clock_mode = FIXED_TIME
        self.play_mode = GAME_MODE

        # 5 seconds
        self.time_control_mode = 2
        self.comp_time = 5000
        self.comp_inc = 0
        self.player_time = 0
        self.player_inc = 0
        self.engine_computer_move = False

        # Help user execute comp moves
        self.computer_move_FEN_reached = False
        self.computer_move_FEN = ""
        self.pre_computer_move_FEN = None
        self.invalid_computer_move = False
        self.last_output_move = None
        self.last_output_move_can = None
        sf.add_observer(self.parse_score)

        # Polyglot book load
        # Load the GM book for now to provide human reference moves
        self.polyglot_book = PolyglotOpeningBook(BOOK_PATH+"gm1950.bin")

        # display lock
        self.display_lock = RLock()

    def write_to_dgt(self, message, move=False, dots=False, beep=True, max_num_tries = 5):
        if self.dgt.dgt_clock:
            with self.dgt_clock_lock:
                self.dgt.send_message_to_clock(message, move=move, dots=dots, beep=beep, max_num_tries=max_num_tries)
                # Wait for dgt clock ack after sending a message
                self.clock_ack_queue.get()
    def write_to_piface(self, message, custom_bitmap = None, clear = False):
        if len(message) > 32:
            message = message[:32]
        if len(message) > 16 and "\n" not in message:
            # Append "\n"
            message = message[:16]+"\n"+message[16:]
        if piface:
            # Acquire piface write lock to guard against multiple threads writing at the same time
            with self.display_lock:
                if clear:
                    cad.lcd.clear()

                if not clear:
                    col, row = cad.lcd.get_cursor()
                    if row == 0 and col + len(message)>16:
                        cad.lcd.set_cursor(0, 1)


                cad.lcd.write(message)
                if custom_bitmap is not None:
                    cad.lcd.set_cursor(15,1)
                    cad.lcd.write_custom_bitmap(custom_bitmap)
                # print "piface wrote: {0}".format(message)
                # Microsleep before returning lock
                # Sleep enables that garbage is not written to the screen
                sleep(0.3)
        elif arduino:
            with self.display_lock:
                # lcd.printString("                ", 0, 0)
                # lcd.printString("                ", 1, 0)
                if clear:
                    lcd.printString("                ", 0, 0)
                    lcd.printString("                ", 0, 1)

                    # lcd.printString("      ",0,1)
                if "\n" in message:
                    first, second = message.split("\n")
                    # print first
                    # print second
                    lcd.printString(first, 0, 0)
                    lcd.printString(second, 0, 1)
                else:
                    lcd.printString(message, 0, 0)
                sleep(0.1)
        else:
            # if self.dgt.dgt_clock:
            #     self.dgt.send_message_to_clock(message, False, False)
            print "Piface: [{0}]".format(message)

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
            # print "pyfish_fen: {0}".format(self.pyfish_fen)
            print "move_list: {0}".format(self.move_list)
            # print "Fen: {0}".format(fen)

            if self.pyfish_castling_fen and self.pyfish_fen!='startpos':
                fen = fen.replace('KQkq', self.pyfish_castling_fen)

            if not self.first_dgt_fen:
                if fen.split(" ")[0] != START_GAME_FEN:
                    fen = self.update_castling_rights(fen)
                self.first_dgt_fen = fen

                # self.update_castling_rights()
                print "first dgt fen : {0}".format(self.first_dgt_fen)
            fen = self.update_castling_rights(fen)
            print "Fen: {0}".format(fen)

            self.current_fen = fen
            # print "Probing for move.."
            m = self.probe_move(fen)
            print "move: {0}".format(m)
            if m:
                move_queue.put(m)
        #        dgt_sem.release()
        if attr.type == CLOCK_BUTTON_PRESSED:
            # print "Clock button {0} pressed".format(attr.message)
            e = ButtonEvent(attr.message)
            self.button_event(e)
        if attr.type == CLOCK_ACK:
            self.clock_ack_queue.put('ack')
            print "Clock ACK Received"


    def poll_dgt(self):
        thread = Thread(target=self.dgt.poll)
        thread.start()

    def connect(self):
        if self.device != "human":
            self.dgt = DGTBoard(self.device)

            self.dgt.subscribe(self.on_observe_dgt_move)
            self.poll_dgt()
            # sleep(1)
            self.dgt.test_for_dgt_clock()
            if self.dgt.dgt_clock:
                print "Found DGT Clock"
            # else:
            #     print "No DGT Clock found"
            self.dgt.get_board()

        # board.poll()

        # self.dgt = dgtnix("dgt/libdgtnix.so")
        # self.dgt.SetOption(dgtnix.DGTNIX_DEBUG, dgtnix.DGTNIX_DEBUG_ON)
        # Initialize the driver with port argv[1]
        # result = self.dgt.Init(self.device)
#        sf.add_observer(self.parse_score)

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

        self.pre_computer_move_FEN = None
        self.invalid_computer_move = False
        self.last_output_move = None
        self.last_output_move_can = None

        self.move_list = []
        self.san_move_list = []
        self.turn = WHITE

        # if piface:
#            cad.lcd.blink_off()
#            cad.lcd.cursor_off()
#            cad.lcd.backlight_off()
#            cad.lcd.backlight_on()
        self.write_to_piface("New Game", clear=True)
        self.write_to_dgt("newgam", max_num_tries=2)
        self.reset_clocks()

        if self.engine_comp_color == WHITE:
            self.engine_computer_move = True
            move_queue.put(FORCE_MOVE)

    def set_level(self, level):
        sf.set_option("Skill Level", level)
        self.write_to_piface("Now on Level " + str(level), clear=True)
        self.write_to_dgt("lvl{: >3}".format(level), max_num_tries=1)

    def set_book(self, book_map_entry):
        filepath = BOOK_PATH + book_map_entry[0] + BOOK_EXTENSION
        print "book filepath : {0}".format(filepath)
        sf.set_option("Book File", filepath)
        self.write_to_piface("Book:\n " + book_map_entry[1], clear=True)
        self.write_to_dgt(book_map_entry[0], move=False, beep=True, max_num_tries=1)

    def set_time_control(self, message, dgt_message, mode):
        self.player_time = 0
        self.player_inc = 0
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
                self.comp_time = 60 * 1000
            elif mode == 9:
                self.comp_time = 180 * 1000
            elif mode == 10:
                self.comp_time = 300 * 1000
            elif mode == 11:
                self.comp_time = 600 * 1000
            elif mode == 12:
                self.comp_time = 900 * 1000
            elif mode == 13:
                self.comp_time = 1800 * 1000
            elif mode == 14:
                self.comp_time = 3600 * 1000
            elif mode == 15:
                self.comp_time = 5400 * 1000

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
        self.reset_clocks()
        self.write_to_piface(message, clear=True)
        self.write_to_dgt(dgt_message, move=False, beep=False, dots=True, max_num_tries=1)


    def set_comp_color(self, fen, start_new_game = True):
        self.engine_comp_color = WHITE if fen == COMP_PLAYS_WHITE else BLACK
        color = "white" if self.engine_comp_color == WHITE else "black"
        print "Computer plays {0}".format(color)
        self.write_to_piface("Computer plays {0}".format(color), clear=True)
        # self.engine_comp_color = WHITE
        self.engine_computer_move = True
        if start_new_game:
            self.start_new_game()
            #move_queue.put(FORCE_MOVE)

    def check_for_command_fen(self, fen):

        if book_map.has_key(fen):
            book_map_entry = book_map[fen]
            self.set_book(book_map_entry)
            # Return true so that engine does not think if merely the opening book is changed
            return True
        elif fen == COMP_PLAYS_WHITE or fen == COMP_PLAYS_BLACK:
            self.set_comp_color(fen)

        elif game_map.has_key(fen):
            if game_map[fen] == NEW_GAME:
                if len(self.move_list) > 0:
                    self.start_new_game()
                    return True
            else:
                play_mode = game_map[fen]
                return True
        elif time_control_map.has_key(fen):
            time_control_entry = time_control_map[fen]
            mode = time_control_entry[0]
            message = time_control_entry[1]
            dgt_message = time_control_entry[2]
#            print "time_control_mode: {0}".format(mode)
#            print "time_control_message: {0}".format(message)

            self.set_time_control(message, dgt_message, mode)
            return True

        else:
            try:
                level = level_map.index(fen)
                self.set_level(level)
                return True
            except ValueError:
                return False

    def update_time(self, color=WHITE):
#        if self.time_last:
#            current = datetime.datetime.now()
#            seconds_elapsed = (current - self.time_last).total_seconds()
    #        print "seconds_elapsed:{0}".format(seconds_elapsed)
#            self.time_last = current
        if color == WHITE:
            self.time_white -= 1*1000
        else:
            self.time_black -= 1*1000

    def reset_clock_update(self):
        self.time_last = datetime.datetime.now()

    def time_add_increment(self, color=WHITE):
        if color == WHITE:
            self.time_white += self.time_inc_white
        else:
            self.time_black += self.time_inc_black

    def update_player_time(self):
        color = WHITE
        if self.engine_comp_color == WHITE:
            color = BLACK
        self.update_time(color=color)

    def format_time_str(self, time_a):

        seconds = time_a*1.0/1000
        # print "seconds: {0}".format(seconds)
        m, s = divmod(seconds, 60)
        # print "m : {0}".format(m)
        # print "s : {0}".format(s)

        if m >=60:
            h, m = divmod(m, 60)
            return "%d:%02d:%02d" % (h, m, s)
        else:
            # print "%02d:%02d" % (m, s)
            return "%02d:%02d" % (m, s)

    def format_time_strs(self, time_a, time_b, disp_length=16):
        fmt_time_a = self.format_time_str(time_a)
        fmt_time_b = self.format_time_str(time_b)

        head_len = len(fmt_time_a)
        tail_len = len(fmt_time_b)

        num_spaces = disp_length - head_len - tail_len

        return fmt_time_a+" "*num_spaces+fmt_time_b

    def update_clocks(self, *args):
        # print "updating clocks"
        if self.play_mode == GAME_MODE:
            custom_bitmap = 0
            b_blink = False
            w_blink = False
            if self.turn == BLACK:
                custom_bitmap = 1
                b_blink = True
            else:
                w_blink = True
            if self.engine_computer_move:
                # print "computer_move"
                if self.engine_searching:
                    self.update_time(color=self.engine_comp_color)
#                    print "comp_time: {0}".format(self.time_black)

                if self.engine_searching and (self.clock_mode == BLITZ or self.clock_mode == BLITZ_FISCHER):
                    self.write_to_piface(self.format_time_strs(self.time_white, self.time_black), custom_bitmap=custom_bitmap, clear=True)
                    # self.write_to_dgt(self.format_time_strs(self.time_white, self.time_black, disp_length=6), beep=False, dots=True)


                    self.dgt.print_time_on_clock(self.time_white, self.time_black, w_blink=w_blink, b_blink=b_blink)

                elif self.clock_mode == FIXED_TIME and self.engine_searching:
                    # If FIXED_TIME
                    if self.engine_comp_color == WHITE:
#                        print "comp_time: {0}".format(self.time_white)
                        if self.time_white and self.time_white >= 1000:
                            self.write_to_piface(self.format_time_str(self.time_white), custom_bitmap=custom_bitmap, clear = True)
                            # print "DGT time: {0}".format(self.dgt.compute_dgt_time_string(self.time_white))
                            # self.write_to_dgt(self.format_time_str(self.time_white), beep=False, dots=True)
                            self.write_to_dgt(self.dgt.compute_dgt_time_string(self.time_white), beep=False, dots=True)
                    else:
#                        print "comp_time: {0}".format(self.time_black)
                        if self.time_black and self.time_black >= 1000:
                            self.write_to_piface(self.format_time_str(self.time_black), custom_bitmap=custom_bitmap, clear = True)
                            # self.write_to_dgt(self.format_time_str(self.time_black), beep=False, dots=True)
                            # print "DGT time: {0}".format(self.dgt.compute_dgt_time_string(self.time_black))
                            self.write_to_dgt(self.dgt.compute_dgt_time_string(self.time_black), beep=False, dots=True)

                        # self.engine_score.children[0].text = "[color=000000]Thinking..\n[size=24]{0}    [b]{1}[/size][/b][/color]".format(self.format_time_str(self.time_white), self.format_time_str(self.time_black))
            # print "not comp move"
#                print "engine_searching : {0}".format(self.engine_searching)
            player_move = not self.engine_searching and self.computer_move_FEN_reached
            # print "player_move: {0}".format(player_move)
            if player_move and len(self.move_list) > 0 and (self.clock_mode == BLITZ or self.clock_mode == BLITZ_FISCHER):
                # print "player_move"
                self.update_player_time()
                self.write_to_piface(self.format_time_strs(self.time_white, self.time_black), custom_bitmap=custom_bitmap, clear=True)
                # self.write_to_dgt(self.format_time_strs(self.time_white, self.time_black, disp_length=6), beep=False, dots=True)
                self.dgt.print_time_on_clock(self.time_white, self.time_black, w_blink=w_blink, b_blink=b_blink)



    def register_move(self, m):
        san = self.get_san([m])[0]
        self.san_move_list.append(san)
        self.move_list.append(m)
        self.write_pgn(san)
        if not self.engine_computer_move:
            self.engine_computer_move = True
        if self.clock_mode == BLITZ_FISCHER and self.play_mode == GAME_MODE:
            # print "white_time : {0}".format(self.time_white)
            # print "black_time : {0}".format(self.time_black)

            self.time_add_increment(color=self.turn)
            # print "Adding increment for {0}".format(self.turn)
            # print "white_time : {0}".format(self.time_white)
            # print "black_time : {0}".format(self.time_black)
        self.switch_turn()



    def perform_undo(self):
        self.move_list.pop()
        self.san_move_list.pop()
        self.rewrite_pgn = True

    def probe_move(self, fen, *args):
        if self.dgt_connected and self.dgt:
            try:
                new_dgt_fen = fen  # color
#                print "mod_fen : {0}".format(fen)
#                print "old_dgt_fen: {0}".format(self.dgt_fen)
#                print "new_dgt_fen: {0}".format(new_dgt_fen)


                if self.dgt_fen and new_dgt_fen:
                    old_dgt_first_token = self.dgt_fen.split()[0]
                    new_dgt_first_token = new_dgt_fen.split()[0]

                    #if old_dgt_first_token == new_dgt_first_token and self.dgt_fen != new_dgt_fen:
                    #    # Update fen if only color to move has changed
                    #    self.dgt_fen = new_dgt_fen

                    if old_dgt_first_token != new_dgt_first_token:
#                        print "old_dgt_first_token: {0}".format(old_dgt_first_token)
#                        print "new_dgt_first_token: {0}".format(new_dgt_first_token)

                        if self.check_for_command_fen(new_dgt_first_token):
                            # print "got Command FEN"
                            self.previous_dgt_fen = self.dgt_fen
                            self.dgt_fen = new_dgt_fen

                            # Return no legal move if its a command FEN, we simply change levels, books, time, options,
                            # and there is no need to process a move
                            return False
                        # print "Not a command FEN"

                        computer_move_first_tok = None
                        if self.computer_move_FEN:
                            computer_move_first_tok = self.computer_move_FEN.split()[0]
#                            print "computer_move_fen_first_tok : {0}".format(computer_move_first_tok)
#                            print "new_dgt_first_token : {0}".format(new_dgt_first_token)
                        if not self.engine_searching and not self.computer_move_FEN_reached and computer_move_first_tok and computer_move_first_tok == new_dgt_first_token:
                            self.computer_move_FEN_reached = True
                            self.invalid_computer_move = False
#                            self.switch_turn()
                            print "computer move Fen reached"
                            self.computer_move_FEN = ""
                        else:
                            self.computer_move_FEN_reached = False
                            if self.computer_move_FEN:
                                self.invalid_computer_move = True
                        # if self.computer_move_FEN and not self.computer_move_FEN_reached and computer_move_first_tok and computer_move_first_tok != new_dgt_first_token:
                        #     invalid_comp_move = True
                        #     print "Invalid computer move!"
                        # print "Checking for legal moves"
                        # print "pref fen: {0}".format(self.dgt_fen)
                        # print "new fen: {0}".format(new_dgt_fen)
                        m = self.get_legal_move(self.dgt_fen, new_dgt_fen)
                        # print "After legal move check is complete"
                        # print m
                        if not m:
                            # print "Checking for quick input"
                            # print "comp_move_fen: {0}".format(self.computer_move_FEN)
                            # If the user made a quick move, try to see if the current position is playable from the computer_move_FEN
                            if self.computer_move_FEN and self.is_fen(self.computer_move_FEN):
                                # print "checking for comp_move_fen legal move"
                                m = self.get_legal_move(self.computer_move_FEN, new_dgt_fen)
#                                print "Quick move made"
#                             print "After quick input check"
                        if m:
                            # print "Got Legal move"
#                            print "Move processed: {0}".format(m)
                            self.previous_dgt_fen = self.dgt_fen
                            self.register_move(m)# Start player clock!

                            new_dgt_fen = self.fen_to_move(new_dgt_fen, self.turn)

                            self.dgt_fen = new_dgt_fen
                            return m
                        else:
                            # print "No legal move found"
                            last_move_fen = sf.get_fen(self.pyfish_fen,  self.move_list[:-1])
                            last_move_fen_first_tok = last_move_fen.split()[0]

                            if new_dgt_first_token == last_move_fen_first_tok:
                                self.dgt_fen = last_move_fen
                                if len(self.move_list) > 0:
                                    self.perform_undo()
                                    return "undo"

                elif new_dgt_fen:
                    # print "elif new_dgt_fen"
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

    def get_score(self, tokens):
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


    # def position(self, move_list, pos='startpos'):
    #     sf.position(pos, move_list)
    #     self.move_list = move_list

    def generate_move_list(self, all_moves, eval=None, start_move_num = 1):
        score = ""
        if start_move_num % 2 == 0:
            turn_sep = '..'
        else:
            turn_sep = ''
        if eval is not None:
            score = str(eval) + " " + turn_sep

        for i, mv in it.izip(it.count(start_move_num), all_moves):
            # move = "b"
            if i % 2 == 1:
                score += "%d." % ((i + 1) / 2)
                # move = "w"
            if mv:
            #                if raw:
                score += "%s " % mv
                # if i % 6 == 0:
                #     score += "\n"
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

    def get_san(self, moves):
        prev_fen = sf.get_fen(self.pyfish_fen,  self.move_list)
            # print prev_fen
        return sf.to_san(prev_fen, moves)

    def parse_score(self, line):
        print "Got line: " + line
        tokens = line.split()
        score = self.get_score(tokens)
        if score:
            if self.turn == BLACK:
                score *=-1
            self.score = score
        if self.play_mode == ANALYSIS_MODE:
            line_index = tokens.index('pv')
            # print "line_index : {0}".format(line_index)
            if line_index > -1:
                pv = self.get_san(tokens[line_index+1:])
                # print "pv : {0}".format(pv)

                if len(pv)>0:
                    self.score_count += 1
                    # print "score_count : {0}".format(self.score_count)
                    if self.score_count > 5:
                        self.score_count = 0
                    if piface and self.score_count == 1:
                        # first_mv = pv[0]
                        if self.use_tb and score == 151:
                            score = 'TB: 1-0'
                        if self.use_tb and score == -151:
                            score = 'TB: 0-1'
                        # separator = ".." if self.turn == BLACK else ""
                        # print self.generate_move_list(pv, eval=score, start_move_num=len(self.move_list)+1)

                        output = self.generate_move_list(pv, eval=score, start_move_num=len(self.move_list)+1)

                        # if not self.engine_output:
                        #     self.engine_output = output
                        #     self.write_to_piface(self.engine_output, clear = True)
                        #
                        # if self.engine_output != output:
                        #     self.engine_output = output
                        if not self.silent:
                        #     self.write_to_piface("Secret Analysis", clear=True)
                        # else:
                            self.write_to_piface(output, clear=True)

                        #cad.lcd.write(str(score)+' ')
                        #                    cad.lcd.write(self.generate_move_list(pv, eval=score, start_move_num=len(self.move_list)+1))
                        print output
                    else:
                        print self.generate_move_list(pv, eval=score, start_move_num=len(self.move_list)+1)
                        print "\n"
        elif self.play_mode == GAME_MODE:
            best_move, self.ponder_move = self.parse_bestmove(line)

            if best_move:
                # print "best_move_san:{0}".format(best_move)
            #                print "best_move_san:{0}".format(sf.to_san([best_move])[0])
                self.last_output_move = self.get_san([best_move])[0]
                self.last_output_move_can = best_move
                # print "output_move: {0}".format(output_move)
                self.engine_computer_move = False
                self.computer_move_FEN_reached = False
                self.engine_searching = False
                if self.dgt_fen:
#                    print "dgt_fen : {0}".format(self.dgt_fen)
                    self.pre_computer_move_FEN = sf.get_fen(self.dgt_fen, [])
                    self.computer_move_FEN = sf.get_fen(self.dgt_fen, [best_move])
#                    print "dgt_fen : {0}".format(self.dgt_fen)
#                    print "comp_move_fen : {0}".format(self.computer_move_FEN)
#                    print self.move_list

                elif self.device == "human":
                    # Not using a DGT board, lets use a chessboard parser in python
                    # board = ChessBoard()
                    # for move in self.move_list:
                    #     board.addTextMove(move)
                    # board.addTextMove(output_move)
#                    print "Not using DGT board"
                    self.move_list.append(best_move)
                    if self.last_output_move:
                        self.san_move_list.append(self.last_output_move)
                    self.switch_turn()
                    # self.computer_move_FEN = board.getFEN()
                if self.last_output_move:
                    # if self.ponder_move and self.ponder_move != '(none)':
                    #     fen = sf.get_fen(self.pyfish_fen,  self.move_list)
                    #     self.ponder_move = sf.to_san(fen, self.ponder_move)[0]

                    if figlet:
                        print figlet.renderText(self.last_output_move)
                    else:
                        print "SAN best_move: {0}".format(self.last_output_move)
                    # print self.ponder_move
                    custom_bitmap = 0
                    if self.turn == BLACK:
                        custom_bitmap = 1

                    if self.ponder_move == '(none)':
                        self.write_to_piface(self.last_output_move + " (Book)", custom_bitmap=custom_bitmap, clear=True)
                        self.write_to_dgt("  book", beep=False, dots=False)
                        self.write_to_dgt(best_move, move=True, beep=True, dots=False)

                    else:
                        self.write_to_piface(self.last_output_move, custom_bitmap=custom_bitmap, clear=True)
                        self.write_to_dgt(best_move, move=True, beep=True, dots=False)


    def write_move(self, ply_count, san):
        if ply_count % 2 == 1:
            self.pgn_file.write(str(ply_count / 2 + 1))
            self.pgn_file.write(". ")
        self.pgn_file.write(san)
        self.pgn_file.write(" ")
        if ply_count % 20 == 0:
            self.pgn_file.write("\n")

            # return pgn;

    def write_pgn(self, san):
        ply_count = len(self.move_list)
        if ply_count == 1 or self.rewrite_pgn:
            self.pgn_file.write("\n[Event \"Picochess\"]\n")

            if self.pyfish_fen != 'startpos':
                self.pgn_file.write(" ( FEN: ")
                self.pgn_file.write(self.pyfish_fen)
                self.pgn_file.write(" )")


            if self.play_mode == ANALYSIS_MODE:
                self.pgn_file.write ("[White \"Analysis\"]\n")
                self.pgn_file.write ("[Black \"Analysis\"]\n")

            else:
                if self.engine_comp_color == WHITE:
                    self.pgn_file.write("[White \"Stockfish\"]\n")
                    self.pgn_file.write("[Black \"User\"]\n")

                else:
                    self.pgn_file.write("[White \"User\"]\n")
                    self.pgn_file.write("[Black \"Stockfish\"]\n")

        if self.rewrite_pgn:
            # Rewrite old moves
            for i, san in enumerate(self.san_move_list):
                self.write_move(i+1, san)
            self.rewrite_pgn = False
        else:
            self.write_move(ply_count, san)

    def reset_clocks(self):
        if not self.player_time:
            self.player_time = self.comp_time
        if not self.player_inc:
            self.player_inc = self.comp_inc
        # if not self.time_white or not self.time_black:
        self.time_white = int(self.comp_time)
        self.time_inc_white = int(self.comp_inc)
        self.time_black = int(self.player_time)
        self.time_inc_black = int(self.player_inc)

        if self.engine_comp_color == BLACK:
            self.time_white, self.time_black = self.time_black, self.time_white
            self.time_inc_white, self.time_inc_black = self.time_inc_black, self.time_inc_white

    def eng_process_move(self):
        print "processing move.."
        self.stop_engine()
        self.score_count = 0
        # self.position(self.move_list, pos='startpos')
        # Needed on the Pi!
        if arm:
            sleep(0.05)
        if self.play_mode == ANALYSIS_MODE:
            sf.go(self.pyfish_fen, moves=self.move_list, infinite=True)
        elif self.play_mode == GAME_MODE:
            if self.engine_computer_move:
                if self.clock_mode == FIXED_TIME:
                    if self.engine_comp_color == BLACK:
                    #                        print "comp_time: {0}".format(comp_time)
                        self.time_black = self.comp_time
                    #                        print "dgt_time: {0}".format(dgt.btime)

                    else:
                        self.time_white = self.comp_time
                    # print "Before sf.go()"
                    # print "move_list: "
                    # print self.move_list
                    # print "pyfish_fen: {0}".format(self.pyfish_fen)
                    sf.go(self.pyfish_fen, moves=self.move_list, movetime=self.comp_time)
#                    self.reset_clock_update()
                else:
                    # self.reset_clocks()

                    if self.clock_mode == BLITZ:
                        sf.go(self.pyfish_fen, moves=self.move_list, wtime=int(self.time_white), btime=int(self.time_black))
#                        print "starting wtime: {0}, starting btime: {1}".format(self.time_white, self.time_black)
                    elif self.clock_mode == BLITZ_FISCHER:
                        sf.go(self.pyfish_fen, moves=self.move_list, wtime=int(self.time_white), btime=int(self.time_black),
                            winc=int(self.time_inc_white), binc=int(self.time_inc_black))

#                    self.reset_clock_update()
        self.engine_searching = True

    def is_fen(self, fen):
        return len(fen.split()) == 6

    def screen_input(self):
        # print "screen_input mode"
        while True:
            m = raw_input("Enter command/move\n")
            # print "got command: {0}".format(m)
            if m == "quit":
                os._exit(0)
            if m == "undo":
                if len(self.move_list)>0:
                    self.move_list.pop()
                    self.san_move_list.pop()
                # board = ChessBoard()
                # for move in self.move_list:
                #     board.addTextMove(move)
                # board.addTextMove(m)
            else:
                self.register_move(m)
            move_queue.put(m)

    def poll_screen(self):
        thread = Thread(target=self.screen_input)
        thread.start()

    def get_polyglot_moves(self, fen, max_num_moves=4):
        key = sf.key(fen, [])
        # With latest pyfish, the below mod is no longer needed
        # key = ctypes.c_uint64(sf.key(fen, [])).value

        polyglot_moves = []
        for j, e in enumerate(self.polyglot_book.get_entries_for_position(key)):
            try:
                m = e["move"]
                # print m
                polyglot_moves.append((sf.to_san(fen, [m]), e["weight"], m))

            except ValueError:
                if m == "e1h1":
                    m = "e1g1"
                elif m == "e1a1":
                    m = "e1c1"
                elif m == "e8a8":
                    m = "e8c8"
                elif m == "e8h8":
                    m = "e8g8"
                polyglot_moves.append((sf.to_san(fen, [m]), e["weight"], m))
            if j >= max_num_moves:
                break
                # print sf.to_san(fen, [m])
        return polyglot_moves

    def update_castling_rights(self, fen):
        can_castle = False
        castling_fen = ''
        board = ChessBoard()
        board.setFEN(fen)
        b = board.getBoard()
        if b[-1][4] == "K" and b[-1][7] == "R":
            can_castle = True
            castling_fen += 'K'

        if b[-1][4] == "K" and b[-1][0] == "R":
            can_castle = True
            castling_fen += 'Q'

        if b[0][4] == "k" and b[0][7] == "r":
            can_castle = True
            castling_fen += 'k'

        if b[0][4] == "k" and b[0][0] == "r":
            can_castle = True
            castling_fen += 'q'

        if not can_castle:
            castling_fen = '-'

        self.pyfish_castling_fen = castling_fen
        # print "castling fen: {0}".format(castling_fen)

        # TODO: Support fen positions where castling is not possible even if king and rook are on right squares
        fen = fen.replace("KQkq", castling_fen)
        return fen

    def char_add(self, c, x):
        return chr(ord(c)+x)

    def button_event(self, event):
        # Button 0-4 are on the front
        # Button 5-7 are on the back, press is 5, left is 6, and right is 7
        print "You pressed",
        print event.pin_num
        # print event

        if self.current_menu == MenuRotation.MAIN:
            if event.pin_num == PlayMenu.LAST_MOVE:
                # Display last move
                if len(self.move_list) > 0:
                    self.write_to_piface("Last move: {0}".format(self.move_list[-1]), clear=True)
                    self.write_to_dgt(pyco.move_list[-1], move=True, beep=False)

            elif event.pin_num == PlayMenu.HINT:
               # if book move, show those first
                fen = sf.get_fen(self.pyfish_fen,  self.move_list)
                # print "fen: {0}".format(fen)
                book_moves = self.get_polyglot_moves(fen)
                # print book_moves
                if book_moves:
                    # Sort book entries by weight
                    book_moves = sorted(book_moves, key=lambda el: el[1], reverse=True)
                    output_str = "Book: "
                    last_index = len(book_moves)-1
                    for j, e in enumerate(book_moves):
                        # if not added_newline and len(output_str) + len(e[0][0]) >= 17:
                        #     output_str += "\n"
                        #     added_newline = True
                        output_str += " " + e[0][0]

                        if j != last_index:
                            output_str += ", "
                        if self.dgt.dgt_clock:
                            self.write_to_dgt(e[2], move=True, max_num_tries=1, beep=False)
                            sleep(1)

                    self.write_to_piface(output_str, clear=True)
                else:
                    self.write_to_piface("Ponder: {0}".format(self.ponder_move), clear=True)
                    self.write_to_dgt(self.ponder_move, move=True, max_num_tries=1, beep=False)

               # if not, then show a position hint
            elif event.pin_num == PlayMenu.EVAL:
                self.write_to_piface("Score: {0}".format(self.score), clear=True)
                if self.score:
                    self.write_to_dgt("{0}".format(self.score), move=False, beep=False, max_num_tries=1)
            elif event.pin_num == PlayMenu.SILENT:
                # Toggle silence
                self.silent = not self.silent
                message = "ON" if self.silent else "OFF"
                # if self.silent:
                #     sf.stop()

                self.write_to_piface("Silence {0}".format(message), clear=True)
                self.write_to_dgt("sil "+message, move=False, max_num_tries=1)

            elif event.pin_num == PlayMenu.SWITCH_MODE:
                if self.play_mode == GAME_MODE:
                    self.play_mode = ANALYSIS_MODE

                    self.pre_computer_move_FEN = None
                    self.invalid_computer_move = False
                    self.last_output_move = None
                    self.last_output_move_can = None

                    self.write_to_piface("Analysis mode", clear=True)
                    self.write_to_dgt("analyz", move=False, max_num_tries=1)

                else:
                    self.play_mode = GAME_MODE
                    self.write_to_piface("Game mode", clear=True)
                    self.write_to_dgt("  game", move=False, max_num_tries=1)


        if self.current_menu == MenuRotation.DATABASE:
            # Database options
            if 0 <= event.pin_num <= 4:
                pass

        if self.current_menu == MenuRotation.ENGINE:
            print "got {0}".format(event.pin_num)
            print "tbases is {0}".format(EngineMenu.TBASES)
            if 0 <= event.pin_num <= 4:
                if event.pin_num == EngineMenu.LEVEL:
                    self.level+=1
                    if self.level > 20:
                        self.level = 0
                    self.set_level(self.level)
                elif event.pin_num == EngineMenu.BOOK:
                    self.book_index+=1
                    if self.book_index > max_book_index:
                        self.book_index = 0
                    self.set_book(book_list[self.book_index])
                elif event.pin_num == EngineMenu.TIME:
                    self.time_control_mode+=1
                    if self.time_control_mode > max_time_control_index:
                        self.time_control_mode = 0
                    self.set_time_control(time_control_list[self.time_control_mode], self.time_control_mode)
                elif event.pin_num == EngineMenu.TBASES:
                    self.use_tb = not self.use_tb
                    status = "ON" if self.use_tb else "OFF"

                    if self.use_tb:
                        sf.set_option('SyzygyProbeLimit', 5)
                    else:
                        sf.set_option('SyzygyProbeLimit', 0)

                    msg = "Tablebases {0}".format(status)
                    print msg
                    self.write_to_piface(msg, clear=True)
                elif event.pin_num == EngineMenu.ENG_INFO:
                    self.write_to_piface(sf.info(), clear=True)
        if self.current_menu == MenuRotation.ALT_INPUT:
            if 0 <= event.pin_num <= 3:
                self.alt_input_entry[event.pin_num] = self.char_add(self.alt_input_entry[event.pin_num], 1)

                if self.alt_input_entry[0] > 'h':
                    self.alt_input_entry[0] = 'a'

                if self.alt_input_entry[1] > '8':
                    self.alt_input_entry[1] = '1'


                if self.alt_input_entry[2] > 'h':
                    self.alt_input_entry[2] = 'a'

                if self.alt_input_entry[3] > '8':
                    self.alt_input_entry[3] = '1'

                self.write_to_piface("".join(self.alt_input_entry), clear=True)

            elif event.pin_num == 4:
                # process move
                # m = raw_input("Enter command/move\n")
                # # print "got command: {0}".format(m)
                # if m == "quit":
                #     os._exit(0)
                # if m == "undo":
                #     if len(self.move_list)>0:
                #         self.move_list.pop()
                #
                #     # board = ChessBoard()
                #     # for move in self.move_list:
                #     #     board.addTextMove(move)
                #     # board.addTextMove(m)
                # else:
                m = "".join(self.alt_input_entry)
                self.write_to_piface("Validating..", clear=True)
                if m == "b1b1":
                    self.start_new_game()
                elif m == "e8e8":
                    self.set_comp_color(COMP_PLAYS_WHITE)
                elif m == "e1e1":
                    self.set_comp_color(COMP_PLAYS_BLACK)

                elif m == "a1a1":
                    # self.perform_undo()
                    move_queue.put("undo_pop")
                    sleep(2)
                    move_queue.put("undo_pop")

                    # Perform undo
                else:
                    prev_fen = sf.get_fen(self.pyfish_fen,  self.move_list)
                    if m in sf.legal_moves(prev_fen):
                        self.write_to_piface("Ok", clear=True)
                        self.register_move(m)
                        move_queue.put(m)
                        self.alt_input_entry = START_ALT_INPUT
                    else:
                        self.write_to_piface("Invalid Move", clear=True)

        if self.current_menu == MenuRotation.SYSTEM:
            if 0 <= event.pin_num <= 4:
                if event.pin_num == SystemMenu.IP:
                    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                    try:
                        s.connect(("google.com", 80))
                        self.write_to_piface(s.getsockname()[0], clear=True)
                        s.close()
                    # TODO: Better handling of exceptions of socket connect
                    except socket.error, v:
                        self.write_to_piface("No Internet Connection!", clear=True)

                if event.pin_num == SystemMenu.SHUTDOWN:
                    self.write_to_piface("Shutting Down! Bye", clear=True)
                    os.system("shutdown -h now")

                if event.pin_num == SystemMenu.RESTART:
                    self.write_to_piface("Restarting..", clear=True)
                    os.execl(sys.executable, *([sys.executable]+sys.argv))

                if event.pin_num == SystemMenu.UPDATE:
                    self.write_to_piface("Checking for Updates..", clear=True)
                    os.chdir(PROG_PATH)
                    p = subprocess.Popen(["/usr/bin/git", "fetch", "origin"],
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, bufsize=0)
                    p.communicate()

                    p = subprocess.Popen(["/usr/bin/git", "rev-list", "HEAD...origin/pycochess", "--count"],
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, bufsize=0)


                    stdout, stderr = p.communicate()
                    updates = int(stdout)
                    print updates
                    # print type(updates)
                    if updates == 0:
                        self.write_to_piface("No New Updates", clear=True)
                    else:
                        self.write_to_piface("Updates found. Updating...", clear=True)
                        os.system("cd {0};git pull".format(PROG_PATH))
                        self.write_to_piface("Restarting..", clear=True)
                        os.execl(sys.executable, *([sys.executable]+sys.argv))
        if event.pin_num == 6 or event.pin_num == 7:
            if event.pin_num == 6:
                if self.current_menu == 0:
                    self.current_menu = MenuRotation.length-1
                else:
                    self.current_menu -= 1

            if event.pin_num == 7:
                if self.current_menu == MenuRotation.length-1:
                    self.current_menu = 0
                else:
                    self.current_menu += 1

            if self.current_menu == MenuRotation.POSITION:
                self.write_to_piface("Setup Position", clear=True)
            elif self.current_menu == MenuRotation.MAIN:
                self.write_to_piface("Play Menu", clear=True)
            elif self.current_menu == MenuRotation.ALT_INPUT:
                self.write_to_piface("Alternate Input", clear=True)
                self.alt_input_entry = START_ALT_INPUT
            elif self.current_menu == MenuRotation.SYSTEM:
                self.write_to_piface("System Menu", clear=True)
            elif self.current_menu == MenuRotation.ENGINE:
                self.write_to_piface("Engine Menu", clear=True)
            elif self.current_menu == MenuRotation.DATABASE:
                self.write_to_piface("Database Menu", clear=True)

        # SCAN_POSITION, WHITE_TO_MOVE, BLACK_TO_MOVE, REVERSE_ORIENTATION = range(4)
        if self.current_menu == MenuRotation.POSITION:
            if event.pin_num == PositionMenu.TO_MOVE_TOGGLE:
                # print self.turn
                self.turn = BLACK if self.turn == WHITE else WHITE
                color = "White" if self.turn == WHITE else "Black"
                print "{0} to move".format(color)
                self.write_to_piface("{0} to move".format(color), clear=True)

            elif event.pin_num == PositionMenu.COMP_PLAY_TOGGLE:
                print "current engine color: {0}".format(self.engine_comp_color)
                self.engine_comp_color = WHITE if self.engine_comp_color == BLACK else BLACK
                color = "White" if self.engine_comp_color == WHITE else "Black"
                print "Computer plays {0}".format(color)
                self.write_to_piface("Computer plays {0}".format(color), clear=True)
            elif event.pin_num == PositionMenu.REVERSE_ORIENTATION:
                self.dgt.reverse_board()
                self.write_to_piface("Reverse Board", clear=True)
            elif event.pin_num == PositionMenu.SCAN_POSITION:
                # Scan current fen
                fen = self.fen_to_move(self.current_fen, self.turn)
                self.pyfish_fen = self.update_castling_rights(fen)
                print "pyfish_fen : {0}".format(self.pyfish_fen)
                self.move_list = []
                self.san_move_list = []

                self.write_to_piface("Scan Position", clear=True)
                if self.engine_comp_color == self.turn:
                    print "Forcing comp to move"
                    self.engine_computer_move = True
                    move_queue.put(FORCE_MOVE)

    def set_device(self, device):
        self.device = device


def update_clocks(pyco):
    Timer(1.0, update_clocks, [pyco]).start()
    pyco.update_clocks()


def process_undo(pyco, m):

    if m.startswith("undo") and len(pyco.move_list) > 0:
        pyco.write_to_piface("Undo - " + pyco.move_list[-1], clear=True)
        pyco.write_to_dgt("  undo")
        pyco.write_to_dgt(pyco.move_list[-1], move=True)

        if m == "undo_pop":
            pyco.perform_undo()
        pyco.switch_turn()
        # print "dgt_fen: {0}".format(pyco.dgt_fen)
        # print "pre_comp_dgt_fen: {0}".format(pyco.pre_computer_move_FEN)
        if pyco.pre_computer_move_FEN and pyco.dgt_fen.split(" ")[0] == pyco.pre_computer_move_FEN.split(" ")[0]:
            pyco.write_to_piface(pyco.last_output_move, clear=True)
            pyco.write_to_dgt(pyco.last_output_move_can, move=True)
        return True


if __name__ == '__main__':
    sf.set_option("OwnBook", "true")

    # In case someone has the pi rev A
    sf.set_option("Hash", 128)
    sf.set_option("Emergency Base Time", 1300)
    sf.set_option("Book File", BOOK_PATH+book_map[DEFAULT_BOOK_FEN][0]+ BOOK_EXTENSION)
    if piface:
        cad = pifacecad.PiFaceCAD()
        cad.lcd.blink_off()
        cad.lcd.cursor_off()
        cad.lcd.backlight_on()

        white_box = pifacecad.LCDBitmap([31,17,17,17,17,17,17,31])
        cad.lcd.store_custom_bitmap(0, white_box)
        black_box = pifacecad.LCDBitmap([31,31,31,31,31,31,31,31])
        cad.lcd.store_custom_bitmap(1, black_box)

        cad.lcd.write("Pycochess {0}".format(VERSION))

        # Lets assume this is the raspberry Pi for now..

        # Make this an option later
        # self.use_tb = True
    elif arduino:
        from nanpy import SerialManager
        from nanpy.lcd import Lcd
        from nanpy import Arduino
        from nanpy import serial_manager
        connection = SerialManager(device='/dev/cu.usbmodem411')
        # Arduino(serial_connection('/dev/tty.usbmodem411')).digitalRead(13)

        # time.sleep(3)
        try:
            lcd = Lcd([8, 9, 4, 5, 6, 7 ], [16, 2], connection=connection)
            lcd.printString('Pycochess {0}'.format(VERSION))
        except OSError:
            arduino = False

    arm = False
#    print os.uname()[4][:3]
    device = None
    if sys.argv:
        device = sys.argv[-1]
        print "Device :: {0}".format(device)
    if not device.startswith("/dev"):
        device = "/dev/ttyUSB0"

    try:
        if os.uname()[4][:3] == 'arm':
            pyco = Pycochess(device)
            arm = True
        else:
            pyco = Pycochess(device)
        pyco.connect()
    except OSError:
        print "DGT board not found\n"
        pyco.set_device("human")
        if piface:
            pyco.current_menu = MenuRotation.ALT_INPUT
            print "Trying piface input mode\n"

        else:
            print "Piface not found -- Trying keyboard input mode\n"
            pyco.poll_screen()

    # pyco.use_tb = True
    # sf.set_option('SyzygyPath', '/home/pi/syzygy/')

    if piface:
        listener = pifacecad.SwitchEventListener(chip=cad)
        for i in range(8):
            listener.register(i, pifacecad.IODIR_FALLING_EDGE, pyco.button_event)
        listener.activate()

    update_clocks(pyco)
    reached_comp_move = False

    print "Pycochess Successful Start!"

    while True:
        #print "Before acquire"
        m = move_queue.get()
        print "Board Updated!"
        if process_undo(pyco, m):
            if pyco.play_mode == GAME_MODE:
                continue

        if pyco.computer_move_FEN_reached:
            print "Comp_move FEN reached"
            custom_bitmap = 0
            if pyco.turn == BLACK:
                custom_bitmap = 1
            pyco.write_to_piface(pyco.last_output_move + " (Done)", custom_bitmap=custom_bitmap, clear=True)
            if pyco.clock_mode == FIXED_TIME:
                pyco.write_to_dgt("  done", beep=False)
            # pyco.engine_computer_move = False
            continue

        if pyco.invalid_computer_move:
            print "Invalid Computer Move"
            pyco.write_to_piface("{0} (Invalid Computer Move)".format(m), clear=True)
            pyco.write_to_dgt(" wrong")
            # sleep(2)
            # process_undo(pyco, "undo_pop")

            continue

        if pyco.engine_comp_color == pyco.turn or pyco.play_mode == ANALYSIS_MODE or m == FORCE_MOVE:
            # if pyco.play_mode != ANALYSIS_MODE:
            #     pyco.write_to_piface("Ok", clear=True)
            pyco.eng_process_move()
        else:
            print "Not processing move, not my turn"

