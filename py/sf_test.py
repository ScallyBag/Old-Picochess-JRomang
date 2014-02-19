import unittest
import stockfish as sf


class SFishTest(unittest.TestCase):

    def get_legal_move(self, from_fen, to_fen):
        to_fen_first_tok = to_fen.split()[0]
        for m in sf.legal_moves(from_fen):
            cur_fen = sf.get_fen(from_fen,[m])
            cur_fen_first_tok = str(cur_fen).split()[0]
#            print "cur_token:{0}".format(cur_fen_first_tok)
#            print "to_token:{0}".format(to_fen_first_tok)
            if cur_fen_first_tok == to_fen_first_tok:
                return m

    def test_legal_move_gen(self):
        from_fen = "7k/1pr3pp/p4p2/4pP1P/8/1qnP2Q1/3R1PP1/3BK3 b - - 0 1"
        to_fen = "7k/1pr3pp/p4p2/4pP1P/8/q1nP2Q1/3R1PP1/3BK3 w - - 0 1"

        assert self.get_legal_move(from_fen, to_fen) == 'b3a3'


if __name__ == "__main__":
    unittest.main()
