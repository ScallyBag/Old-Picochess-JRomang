from polyglot_opening_book import PolyglotOpeningBook
import stockfish as sf

if __name__ == "__main__":
    p = PolyglotOpeningBook('book.bin')
    key = sf.key('startpos', [])
    fen = sf.get_fen('startpos', [])
    for e in p.get_entries_for_position(key):
        print e