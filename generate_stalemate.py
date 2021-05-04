import chess


def process_file(filename):
    game = ""
    with open(filename, "r") as f:
        while True:
            line = f.readline()
            if not line:
                break

            if line.startswith("[Site"):
                game = line
            if line.startswith("1. "):
                handle_game(game, line)

def handle_game(game, move_line):
    tokens = move_line.split(" ")
    board = chess.Board()
    i = 0
    while i < len(tokens) - 1:  # -1 to ignore the result (1-0, 0-1, 1/2-1/2)
        token = tokens[i].strip(' !?')
        if token[0].isdigit():
            pass
        elif token == "{":
            while token != "}":
                i += 1
                token = tokens[i]
        else:
            board.push_san(token)
        i += 1
    if not token.endswith("#"):
        moves = board.legal_moves
        if moves.count() == 0:
            print(game)
            print(board)

if __name__ == "__main__":
    process_file("chess_test/games/lichess_db_standard_rated_2013-01.pgn")
