import re

leveldb = True
try:
    import leveldb
except ImportError:
    leveldb = False

INDEX_FILE_POS = "last_pos"
DB_HEADER_MAP = {"White": 0, "WhiteElo": 1, "Black": 2,
                 "BlackElo": 3, "Result": 4, "Date": 5, "Event": 6, "Site": 7,
                 "ECO": 8, INDEX_FILE_POS: 9, "FEN": 10}

tag_regex = re.compile(r"\[([A-Za-z0-9]+)\s+\"(.*)\"\]")
movetext_regex = re.compile((r"\n"
                             r"    (\\;.*?[\\n\\r])\n"
                             r"    |(\\{.*?[^\\\\]\\})\n"
                             r"    |(\\$[0-9]+)\n"
                             r"    |(\\()\n"
                             r"    |(\\))\n"
                             r"    |(\\*|1-0|0-1|1/2-1/2)\n"
                             r"    |(\n"
                             r"        ([a-hKQRBN][a-hxKQRBN1-8+#=\\-]{1,6}\n"
                             r"        |O-O(?:\\-O)?)\n"
                             r"        ([\\?!]{1,2})*\n"
                             r"    )\n"
                             r"    "
                            ), re.DOTALL | re.VERBOSE)

class Game(object):
    """The root node of a game."""
    def __init__(self, start_comment="", headers=None):
        if headers is None:
            self.__headers = {}
        else:
            if not headers.game == self:
                raise ValueError("Header bag assigned to a different game.")
            self.__headers = headers
            # print headers.__headers
        self.__moves = []

    def set_headers(self, headers):
        self.__headers = headers

    @property
    def headers(self):
        """A `chess.GameHeaderBag` holding the headers of the game."""
        return self.__headers

    def set_moves(self, moves):
        self.__moves = moves

    @property
    def moves(self):
        return self.__moves


class ChessDatabase(object):
    def __init__(self,  db_index_folder, **kwargs):
        # Needs a database index folder (created for the PGN),
        # refer to https://github.com/sshivaji/polyglot for info on creating a database index for a PGN file
        # The index also contains the location of the actual PGN file
        self.db_index=leveldb.LevelDB(db_index_folder)

    def query_position(self, pos_hash, max_games=-1):
        # pos_hash is the polyglot hash of the position
        # max_games is the max number of games to return, -1 means unlimited
        if self.db_index is not None:
            # Is not None is very important here, though not pythonic.
            # if not self.db_index will consume a lot of time as it will
            # actually check that the database is not empty via a range iteration scan!
            # Perhaps this should be reported as a leveldb wrapper issue?
            if type(pos_hash) is not str:
                pos_hash = str(pos_hash)
            try:
                game_ids = self.db_index.Get(pos_hash).split(',')[:max_games]
            except KeyError, e:
                print "Position key not found!"
                game_ids = []

            # More processing?
            return game_ids

    # def go_to_position(self, pos_hash, game):
    #     import stockfish as sf
    #     sf.to_can()

    def load_game(self, game_num):
        first = self.db_index.Get("game_{0}_data".format(game_num)).split("|")[DB_HEADER_MAP[INDEX_FILE_POS]]

#        if game_num+1 < self.pgn_index[INDEX_TOTAL_GAME_COUNT]:
#            second = self.db_index_book.Get("game_{0}_{1}".format(game_num+1,INDEX_FILE_POS))

#        second = self.pgn_index["game_index_{0}".format(game_num+1)][INDEX_FILE_POS]
        try:
            second = self.db_index.Get("game_{0}_data".format(game_num+1)).split("|")[DB_HEADER_MAP[INDEX_FILE_POS]]
            second = int(second)
        except KeyError:
            second = None

        with open(self.db_index.Get("pgn_filename")) as f:
            first = int(first)

            f.seek(first)
            line = 1
            content = []
            while line:
                line = f.readline()
                pos = f.tell()
                if second and pos >= second:
                    break
                # print pos
                content.append(line)
        # Return content of the game
        return content

    @classmethod
    def open_game(cls, text):
        # Add variation support later
        game = Game()
        movetext = ""

        for i, line in enumerate(text):
            # print line
            # Decode and strip the line.
            line = line.decode('latin-1').strip()
            # print line

            if i == 0 and not line.startswith('[') and line.endswith(']'):
                line = "[" + line
            # print line
            # Skip empty lines and comments.
            if not line or line.startswith("%"):
                continue

            # Check for tag lines.
            tag_match = tag_regex.match(line)
            if tag_match:
                tag_name = tag_match.group(1)
                tag_value = tag_match.group(2).replace("\\\\", "\\").replace("\\[", "]").replace("\\\"", "\"")

                game.headers[tag_name] = tag_value
            # Parse movetext lines.
            else:
                movetext += line

        # process movetext
        moves = movetext.split(" ")

        raw_moves = []

        for m in moves:
            last_index = len(m) - 1
            index = m.find('.')
            if -1 < index < last_index:
                raw_moves.append(m[index+1:])
            elif index == -1:
                raw_moves.append(m)
        raw_moves.pop()
        game.set_moves(raw_moves)

        return game






