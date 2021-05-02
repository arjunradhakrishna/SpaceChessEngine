#include "pgn.h"
#include "board_impl.h"
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <cctype>
#include <cassert>

namespace {
    space::PieceType to_piece_type(char p) {
        switch (p) {
            case 'Q': return space::PieceType::Queen;
            case 'K': return space::PieceType::King;
            case 'N': return space::PieceType::Knight;
            case 'B': return space::PieceType::Bishop;
            case 'R': return space::PieceType::Rook;
            case 'P': return space::PieceType::Pawn;
            default : return space::PieceType::None;
        }
    }

    int to_file(char c) { return c - 'a'; }

    int to_rank(char c) { return c - '1'; }

    space::Ply parse_move(std::string move, int move_number, space::Color side) {
        int pos = move.length() - 1;

        // Annotations: ?, !
        while (move[pos] == '!' || move[pos] == '?') pos -= 1;
        std::string annotation = pos == move.length() - 1 ? "" : move.substr(pos + 1);

        // Checks: +, ++, #
        bool is_check = move[pos] == '+' || move[pos] == '#';
        bool is_checkmate = (move[pos] == '+' && move[pos-1] == '+') || move[pos] == '#';
        while (move[pos] == '+' || move[pos] == '#') pos -= 1;

        // Castling: O-O, O-O-O
        bool is_long_castle = move.substr(0, 5) == "O-O-O";
        bool is_short_castle = !is_long_castle && move.substr(0, 3) == "O-O";

        // Promotion: =Q, =N, etc
        space::Piece promotion_piece = { space::PieceType::None, side };
        bool is_promotion = !is_long_castle && !is_short_castle && move[pos - 1] == '=';
        if (is_promotion) {
            char promotion_piece_char = is_promotion ? move[pos] : '-';
            promotion_piece.pieceType = to_piece_type(promotion_piece_char);
            promotion_piece.color = side;
            pos = pos - 2;
        }

        // Destination: a3, b5, etc
        auto destination_str =
            is_short_castle ? (side == space::Color::White ? "g1" : "g8") :
            is_long_castle  ? (side == space::Color::White ? "c1" : "c8") :
            move.substr(pos - 1, 2);
        pos = (is_short_castle || is_long_castle) ? -1 : pos - 2;
        auto destination = space::Position(destination_str);

        // Capture symbol: 'x'
        bool is_capture = !is_short_castle && !is_long_castle && pos >= 0 && move[pos] == 'x';
        pos = is_capture ? pos - 1 : pos;

        // Piece: Q, N, B, R, K
        auto piece = space::Piece();
        piece.pieceType = to_piece_type(
            (is_short_castle || is_long_castle) ? 'K' :
            isupper(move[0]) ? move[0] :
            'P'
        );
        piece.color = side;

        // Disambiguation: a, 4, a5
        // For pawn captures axb3, a is the disambiguation
        int start = isupper(move[0]) ? 1 : 0;
        std::string disambiguation = (!is_short_castle && !is_long_castle && pos >= 0)
            ? move.substr(start, pos - start + 1)
            : "";

        // Create the ply object.
        space::Ply ply;
        ply.move = move;
        ply.color = side;
        ply.move_number = move_number;
        ply.is_check = is_check;
        ply.is_checkmate = is_checkmate;
        ply.destination = destination;
        ply.is_capture = is_capture;
        ply.is_short_castle = is_short_castle;
        ply.is_long_castle = is_long_castle;
        ply.piece = piece;
        ply.is_promotion = is_promotion;
        ply.promotion_piece = promotion_piece;
        ply.disambiguation = disambiguation;
        ply.annotation = annotation;

        return ply;
    }

    std::vector<space::Ply> parse_moves(std::string movetext) {
        std::vector<space::Ply> moves;
        space::Color side = space::Color::White;
        int move_number = -1;

        auto ss = std::stringstream(movetext);
        std::string s;
        while (!ss.eof()) {
            ss >> s;
            if (s == "{") {
                char c = 0;
                while (c != '}') ss >> c;
                continue;
            }

            // Termination markers
            if (s == "0-1" || s == "1-0" || s == "1/2-1/2" || s == "*") break;

            // Is it a move number?
            if (isdigit(s[0]) && s[s.length() - 1] == '.') {
                // White moves have single . at the end. Black moves have ...
                side = (s[s.length() - 2] == '.' && s[s.length() - 3] == '.')
                    ? space::Color::Black
                    : space::Color::White;
                move_number = atoi(s.c_str());
                continue;
            }

            // It is a move (There are more complex pgn constructs, but we ignore them.)
            auto ply = parse_move(s, move_number, side);
            moves.push_back(ply);

            // Flip sides for next move in case the next ply doesn't have a move number.
            side = side == space::Color::White ? space::Color::Black : space::Color::White;
        }

        return moves;
    }
}

namespace space {
    // For now, assume everything is well-formatted.
    // This is a poor parser.
    std::unique_ptr<Game> PGN::parse(std::istream& input) {
        std::string line;
        std::string starting_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        std::map<std::string, std::string> metadata;

        while (std::getline(input, line) && line.length() != 0 && line[0] == '[') {
            // We could avoid a couple of allocations, by directly reading from
            // input. Can't be bothered.
            auto ss = std::stringstream(line);

            char c; ss >> c;
            if (c != '[') return std::unique_ptr<Game>(nullptr); // Malformed?

            std::string name, value;
            ss >> name;

            std::getline(ss, value);
            if (value[0] != ' '
             || value[1] != '"'
             || value[value.length() - 1] != ']'
             || value[value.length() - 2] != '"'
            ) {
                return std::unique_ptr<Game>(nullptr);
            }
            value = value.substr(2, value.length() - 4);
            metadata[name] = value;

            if (name == "FEN") starting_position = value;
        }
        if (input.eof()) return std::unique_ptr<Game>(nullptr);

        std::stringbuf movetext_buf;
        while (std::getline(input, line) && line.length() != 0) {
            movetext_buf.sputn(line.c_str(), line.length());
        }
        auto movetext = movetext_buf.str();
        auto moves = parse_moves(movetext);

        return std::make_unique<Game>(Fen(starting_position), moves, metadata);
    }

    std::vector<Game> PGN::parse_all(std::istream& input) {
        return parse_many(input, 0);
    }

    std::vector<Game> PGN::parse_many(std::istream& input, int limit) {
        std::vector<Game> games;
        while (!input.eof()) {
            auto game = PGN::parse(input);
            if (game) {
                games.push_back(*game);
                if (limit != 0 && games.size() == limit) break;
            }
        }
        return games;
    }

    std::optional<Move> Ply::to_move(IBoard* board) const {
        auto move = Move(
            -1,
            -1,
            destination.rank,
            destination.file,
            promotion_piece.pieceType
        );

        if (disambiguation.length() == 2) {
            move.sourceFile = to_file(disambiguation[0]);
            move.sourceRank = to_rank(disambiguation[1]);
            return move;
        }

        if (is_short_castle || is_long_castle) {
            move.sourceFile = 4;
            move.sourceRank = color == Color::White ? 0 : 7;
            return move;
        }

        int dir = (color == Color::White ? 1 : -1);
        std::optional<Position> source_position;
        switch (piece.pieceType) {
            case PieceType::Pawn:
                if (is_capture) {
                    move.sourceRank = destination.rank - dir;
                    move.sourceFile = to_file(disambiguation[0]);
                    return move;
                }
                else {
                    move.sourceFile = destination.file;
                    move.sourceRank = destination.rank - dir;
                    if (board->getPiece(Position(move.sourceRank, move.sourceFile)).has_value()) return move;

                    move.sourceRank = destination.rank - dir - dir;
                    if (board->getPiece(Position(move.sourceRank, move.sourceFile)).has_value()) return move;
                }
                return {};

            case PieceType::King:
                source_position = try_source_offsets(board, space::internals::MoveOffsets::king_offsets);
                if (!source_position.has_value()) return {};

                move.sourceRank = source_position.value().rank;
                move.sourceFile = source_position.value().file;
                return move;

            case PieceType::Rook:
                source_position = try_source_offsets(board, space::internals::MoveOffsets::orthogonal_offsets);
                if (!source_position.has_value()) return {};

                move.sourceRank = source_position.value().rank;
                move.sourceFile = source_position.value().file;
                return move;

            case PieceType::Bishop:
                source_position = try_source_offsets(board, space::internals::MoveOffsets::diagonal_offsets);
                if (!source_position.has_value()) return {};

                move.sourceRank = source_position.value().rank;
                move.sourceFile = source_position.value().file;
                return move;

            case PieceType::Queen:
                source_position = try_source_offsets(board, space::internals::MoveOffsets::diagonal_offsets);
                if (!source_position.has_value()) {
                    source_position = try_source_offsets(board, space::internals::MoveOffsets::orthogonal_offsets);
                }
                if (!source_position.has_value()) return {};

                move.sourceRank = source_position.value().rank;
                move.sourceFile = source_position.value().file;
                return move;

            case PieceType::Knight:
                source_position = try_source_offsets(board, space::internals::MoveOffsets::knight_offsets);
                if (!source_position.has_value()) return {};

                move.sourceRank = source_position.value().rank;
                move.sourceFile = source_position.value().file;
                return move;

            default:
                return {};
        }
    }

    bool Ply::satisfies_disambiguation(Position source_position) const {
        for (auto c : disambiguation) {
            if (isdigit(c) && source_position.rank != c - '1') return false;
            if (isalpha(c) && source_position.file != c - 'a') return false;
        }
        return true;
    }

    std::optional<Position> Ply::try_source_offsets(IBoard* board,
                          std::vector<std::vector<std::pair<int, int>>> offset_lists) const {

        std::vector<Position> candidate_positions;
        for (auto& offset_list : offset_lists) {
            for (auto& offset : offset_list) {
                auto rank = destination.rank + offset.first;
                auto file = destination.file + offset.second;
                if (rank < 0 || rank > 7 || file < 0 || file > 7) continue;

                auto pos = Position(rank, file);
                auto pos_piece_opt = board->getPiece(pos);
                if (!pos_piece_opt.has_value()) continue;

                auto pos_piece = pos_piece_opt.value();
                if (pos_piece.pieceType == piece.pieceType && pos_piece.color == piece.color && satisfies_disambiguation(pos)) {
                    candidate_positions.push_back(pos);
                }
                break;
            }
        }

        if (candidate_positions.size() == 0) return {};
        if (candidate_positions.size() == 1) return candidate_positions[0];

        // We can have more than one valid candidate, in that case, all except one
        // candidate will put own king in check.
        for (auto& source : candidate_positions) {
            auto move = Move(
                source.rank, source.file,
                destination.rank, destination.file
            );
            auto new_board = board->updateBoard(move);
            if (new_board.has_value() && !new_board.value()->isUnderCheck(color)) return source;
        }
        return {};
    }
}
