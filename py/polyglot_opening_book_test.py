from polyglot_opening_book import PolyglotOpeningBook
import stockfish as sf
import ctypes

if __name__ == "__main__":
    p = PolyglotOpeningBook('book.bin')
    # fen = 'startpos'
    fen = 'rnbqkbnr/pppp1ppp/4p3/8/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 2'
    # conversion until pyfish is updated
    key = ctypes.c_uint64(sf.key(fen, [])).value

    for e in p.get_entries_for_position(key):
        try:
            m = e["move"]
            print (sf.to_san(fen, [m]), e["weight"])
        except ValueError:
            if m == "e1h1":
                m = "e1g1"
            elif m == "e1a1":
                m = "e1c1"
            elif m == "e8a8":
                m = "e8c8"
            elif m == "e8h8":
                m = "e8g8"
            print (sf.to_san(fen, [m]), e["weight"])