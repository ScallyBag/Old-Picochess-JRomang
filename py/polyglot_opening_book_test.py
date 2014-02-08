from polyglot_opening_book import PolyglotOpeningBook
import stockfish as sf

if __name__ == "__main__":
    p = PolyglotOpeningBook('book.bin')
    # fen = 'startpos'
    fen = 'r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4'
    key = sf.key(fen, [])
    for e in p.get_entries_for_position(key):
        try:
            m = e["move"]
            print sf.to_san(fen, [m])
        except ValueError:
            if m == "e1h1":
                m = "e1g1"
            elif m == "e1a1":
                m = "e1c1"
            elif m == "e8a8":
                m = "e8c8"
            elif m == "e8h8":
                m = "e8g8"
            print sf.to_san(fen, [m])