import chess # See https://python-chess.readthedocs.io/en/latest/ for full module documentation

name = "SimpleMaterial" # The evaluation name (displayed in engine name in the future)
author = "jromang" # The evaluation author (displayed in engine name in the future)

# This is the evaluation function
# You have to return a score in centipawns
def evaluate(board):
    score = 0
    score += (len(board.pieces(chess.PAWN, chess.WHITE)) - len(board.pieces(chess.PAWN, chess.BLACK))) * 100
    score += (len(board.pieces(chess.KNIGHT, chess.WHITE)) - len(board.pieces(chess.KNIGHT, chess.BLACK))) * 300
    score += (len(board.pieces(chess.BISHOP, chess.WHITE)) - len(board.pieces(chess.BISHOP, chess.BLACK))) * 300
    score += (len(board.pieces(chess.ROOK, chess.WHITE)) - len(board.pieces(chess.ROOK, chess.BLACK))) * 500
    score += (len(board.pieces(chess.QUEEN, chess.WHITE)) - len(board.pieces(chess.QUEEN, chess.BLACK))) * 900
    return score


# This is a testing function, you can call it from command line ('python3 evaluate.py')
# to check if your evaluation function is working
if __name__ == "__main__":
    test_board=chess.Board() # You can pass a FEN postion here
    print(str(test_board))
    print("Evaluation:"+str(evaluate(test_board)))