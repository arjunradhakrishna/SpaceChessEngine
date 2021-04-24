#include "counting_board.h"
#include "squares.h"
#include "direction.h"
#include <sstream>

namespace {
	space::PieceType toPieceType(char c) {
		using namespace space;
		// Pawn, EnPessantCapturablePawn, Rook, Knight, Bishop, Queen, King, None
		switch (c)
		{
		case 'p':
		case 'P':
			return PieceType::Pawn;
		case 'r':
		case 'R':
			return PieceType::Rook;
		case 'n':
		case 'N':
			return PieceType::Knight;
		case 'b':
		case 'B':
			return PieceType::Bishop;
		case 'q':
		case 'Q':
			return PieceType::Queen;
		case 'k':
		case 'K':
			return PieceType::King;
		default:
			throw std::runtime_error(std::string("Unrecognizable piece type '") + c + "'");
		}
	}
}

// Trivial CBoard functions
namespace space {
	inline std::optional<Piece> CBoard::getPiece(Position position) const {
		auto piece = pieces[position.rank][position.file];
		if (piece.pieceType == PieceType::None) return {};
		return piece;
	}

	inline bool CBoard::isUnderAttack(Position position, Color attackingColor) const {
		return (attackedBy[(int)attackingColor][position.rank][position.file] != 0)
			|| (attackedByKnight[(int)attackingColor][position.rank][position.file] != 0);
	}

	inline bool CBoard::isUnderAttackFromDirection(Position position, Color attackingColor, Direction direction) const {
		return (attackedBy[(int)attackingColor][position.rank][position.file] & direction) != 0;
	}

	inline bool CBoard::isUnderCheck(Color color, std::optional<Position> targetKingPosition) const {
		auto& position = kingPosition[(int)color];
		auto attackingColor = oppositeColor(color);
		return isUnderAttack(position, attackingColor);
	}

	inline bool CBoard::canCastleLeft(Color color) const {
		auto& castlingRightsForColor = castlingRights[(int)color];
		auto dir = color == Color::White ? LongCastle : ShortCastle;
		return castlingRightsForColor[dir];
	}

	inline bool CBoard::canCastleRight(Color color) const {
		auto& castlingRightsForColor = castlingRights[(int)color];
		auto dir = color == Color::White ? ShortCastle : LongCastle;
		return castlingRightsForColor[dir];
	}
}

// Boiler plate stuff
namespace space {
	using namespace space::squares;

	std::unique_ptr<CBoard> CBoard::startPosition() {
		auto board = std::make_unique<CBoard>();

		// Who moves
		board->nextMover = Color::White;

		// Initial empty squares
		for (auto f = 0; f <= 7; f++) 
			for (auto r = 0; r <= 7; r++) 
				board->pieces[r][f] = { PieceType::None, Color::White };

		// Initial white pieces
		board->pieces[a1.rank][a1.file] = board->pieces[h1.rank][h1.file] = { PieceType::Rook, Color::White };
		board->pieces[b1.rank][b1.file] = board->pieces[g1.rank][g1.file] = { PieceType::Knight, Color::White };
		board->pieces[c1.rank][c1.file] = board->pieces[f1.rank][f1.file] = { PieceType::Bishop, Color::White };
		board->pieces[d1.rank][d1.file] = { PieceType::Queen, Color::White };
		board->pieces[e1.rank][e1.file] = { PieceType::King, Color::White };
		for (auto f = 0; f <= 7; f++) board->pieces[1][f] = { PieceType::Pawn, Color::White };

		// Initial black pieces
		board->pieces[a8.rank][a8.file] = board->pieces[h8.rank][h8.file] = { PieceType::Rook, Color::Black };
		board->pieces[b8.rank][b8.file] = board->pieces[g8.rank][g8.file] = { PieceType::Knight, Color::Black };
		board->pieces[c8.rank][c8.file] = board->pieces[f8.rank][f8.file] = { PieceType::Bishop, Color::Black };
		board->pieces[d8.rank][d8.file] = { PieceType::Queen, Color::Black };
		board->pieces[e8.rank][e8.file] = { PieceType::King, Color::Black };
		for (auto f = 0; f <= 7; f++) board->pieces[6][f] = { PieceType::Pawn, Color::Black };

		// King positions
		board->kingPosition[(int)Color::White] = e1;
		board->kingPosition[(int)Color::Black] = e8;


		// Castling rights
		board->castlingRights[(int)Color::White][ShortCastle] = true;
		board->castlingRights[(int)Color::White][LongCastle]  = true;
		board->castlingRights[(int)Color::Black][ShortCastle] = true;
		board->castlingRights[(int)Color::Black][LongCastle]  = true;

		// Attacked by and attacked by knight
		for (auto r = 0; r <= 7; r++) {
			for (auto f = 0; f <= 7; f++) {
				board->attackedBy[(int)Color::White][r][f] = 0;
				board->attackedBy[(int)Color::Black][r][f] = 0;
				board->attackedByKnight[(int)Color::White][r][f] = 0;
				board->attackedByKnight[(int)Color::Black][r][f] = 0;
			}
		}

		// Rank 1
		// a1 not attacked
		// b1 by rook on a1
		board->attackedBy[(int)Color::White][b1.rank][b1.file] = Direction::West;
		// c1 by queen on d1
		board->attackedBy[(int)Color::White][c1.rank][c1.file] = Direction::East;
		// d1 by king on e1
		board->attackedBy[(int)Color::White][d1.rank][d1.file] = Direction::East;
		// e1 by queen on d1
		board->attackedBy[(int)Color::White][e1.rank][e1.file] = Direction::West;
		// f1 by king on e1
		board->attackedBy[(int)Color::White][f1.rank][f1.file] = Direction::West;
		// g1 by king on h1
		board->attackedBy[(int)Color::White][g1.rank][g1.file] = Direction::East;
		// h1 not attacked

		// Rank 2
		// a2 by rook on a1
		board->attackedBy[(int)Color::White][a2.rank][a2.file] = Direction::South;
		// b2 by bishop on c1
		board->attackedBy[(int)Color::White][b2.rank][b2.file] = Direction::SouthEast;
		// c2 by queen on d1
		board->attackedBy[(int)Color::White][c2.rank][c2.file] = Direction::SouthEast;
		// d2 by bishop on d1, king on e1, bishop on c1, knight on b1
		board->attackedBy[(int)Color::White][d2.rank][d2.file] = 
			Direction::SouthWest | Direction::South | Direction::SouthEast;
		board->attackedByKnight[(int)Color::White][d2.rank][d2.file] = KnightDirection::Clock08;
		// e2 by bishop on f1, king on e1, queen on d1 , knight on b1
		board->attackedBy[(int)Color::White][e2.rank][e2.file] = 
			Direction::SouthWest | Direction::South | Direction::SouthEast;
		board->attackedByKnight[(int)Color::White][e2.rank][e2.file] = KnightDirection::Clock04;
		// f2 by king on e1
		board->attackedBy[(int)Color::White][f2.rank][f2.file] = Direction::SouthWest;
		// g2 by bishop on f1
		board->attackedBy[(int)Color::White][g2.rank][g2.file] = Direction::SouthWest;
		// h2 by rook on h1
		board->attackedBy[(int)Color::White][h2.rank][h2.file] = Direction::South;

		// Rank 3
		for (auto f = 1; f <= 6; f++)
			board->attackedBy[(int)Color::White][2][f] = Direction::SouthEast | Direction::SouthWest;
		board->attackedBy[(int)Color::White][2][0] = Direction::SouthEast;
		board->attackedBy[(int)Color::White][2][7] = Direction::SouthWest;
		board->attackedByKnight[(int)Color::White][2][0] = KnightDirection::Clock05;
		board->attackedByKnight[(int)Color::White][2][2] = KnightDirection::Clock07;
		board->attackedByKnight[(int)Color::White][2][5] = KnightDirection::Clock05;
		board->attackedByKnight[(int)Color::White][2][7] = KnightDirection::Clock07;

		// Rank 6
		for (auto f = 1; f <= 6; f++)
			board->attackedBy[(int)Color::Black][5][f] = Direction::NorthEast | Direction::NorthWest;
		board->attackedBy[(int)Color::Black][5][0] = Direction::NorthEast;
		board->attackedBy[(int)Color::Black][5][7] = Direction::NorthWest;
		board->attackedByKnight[(int)Color::Black][5][0] = KnightDirection::Clock01;
		board->attackedByKnight[(int)Color::Black][5][2] = KnightDirection::Clock11;
		board->attackedByKnight[(int)Color::Black][5][5] = KnightDirection::Clock01;
		board->attackedByKnight[(int)Color::Black][5][7] = KnightDirection::Clock11;

		// Rank 7
		// a7 by rook on a8
		board->attackedBy[(int)Color::Black][a7.rank][a7.file] = Direction::North;
		// b7 by bishop on c8
		board->attackedBy[(int)Color::Black][b7.rank][b7.file] = Direction::NorthEast;
		// c7 by queen on d8
		board->attackedBy[(int)Color::Black][c7.rank][c7.file] = Direction::NorthEast;
		// d7 by bishop on d8, king on e8, bishop on c8, knight on b8
		board->attackedBy[(int)Color::Black][d7.rank][d7.file] = 
			Direction::NorthWest | Direction::North | Direction::NorthEast;
		board->attackedByKnight[(int)Color::Black][d7.rank][d7.file] = KnightDirection::Clock10;
		// e7 by bishop on f8, king on e8, queen on d8 , knight on b8
		board->attackedBy[(int)Color::Black][e7.rank][e7.file] = 
			Direction::NorthWest | Direction::North | Direction::NorthEast;
		board->attackedByKnight[(int)Color::Black][e7.rank][e7.file] = KnightDirection::Clock02;
		// f7 by king on e8
		board->attackedBy[(int)Color::Black][f7.rank][f7.file] = Direction::NorthWest;
		// g7 by bishop on f8
		board->attackedBy[(int)Color::Black][g7.rank][g7.file] = Direction::NorthWest;
		// h7 by rook on h8
		board->attackedBy[(int)Color::Black][h7.rank][h7.file] = Direction::North;

		// Rank 8
		// a8 not attacked
		// b8 by rook on a8
		board->attackedBy[(int)Color::Black][b8.rank][b8.file] = Direction::West;
		// c8 by queen on d8
		board->attackedBy[(int)Color::Black][c8.rank][c8.file] = Direction::East;
		// d8 by king on e8
		board->attackedBy[(int)Color::Black][d8.rank][d8.file] = Direction::East;
		// e8 by queen on d8
		board->attackedBy[(int)Color::Black][e8.rank][e8.file] = Direction::West;
		// f8 by king on e8
		board->attackedBy[(int)Color::Black][f8.rank][f8.file] = Direction::West;
		// g8 by king on h8
		board->attackedBy[(int)Color::Black][g8.rank][g8.file] = Direction::East;
		// h8 not attacked

		return board;
	}

	std::unique_ptr<CBoard> CBoard::clone() const {
		auto newBoard = std::make_unique<CBoard>();
		newBoard->nextMover = nextMover;
		newBoard->pieces = pieces;
		newBoard->kingPosition = kingPosition;
		newBoard->castlingRights = castlingRights;
		newBoard->attackedBy = attackedBy;
		newBoard->attackedByKnight = attackedByKnight;
	}

	std::string CBoard::attackString() const {
		std::stringstream ss;
		auto fileNames = "abcdefgh";

		for (auto color : { Color::White, Color::Black }) {
			ss << "Attacks from ";
			ss << (color == Color::White ? "white" : "black");
			ss << " (ortho and diagonal):" << std::endl;

			ss << "    |";
			for (int f = 0; f <= 7; f++) {
				ss << " " << (char)(f + 'a') << " |";
			}
			ss << std::endl;

			ss << "    |";
			for (int f = 0; f <= 7; f++) {
				ss << "---|";
			}
			ss << std::endl;

			for (int r = 7; r >= 0; r--) {
				ss << (r+1) << "   |";
				for (int f = 0; f <= 7; f++) {
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::NorthWest) ? "\u2196" : " ");
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::North) ? "\u2191" : " ");
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::NorthEast) ? "\u2197" : " ");
					ss << "|";
				}
				ss << std::endl;

				ss << (r+1) << "   |";
				for (int f = 0; f <= 7; f++) {
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::West) ? "\u2190" : " ");
					ss << " ";
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::East) ? "\u2192" : " ");
					ss << "|";
				}
				ss << std::endl;

				ss << (r+1) << "   |";
				for (int f = 0; f <= 7; f++) {
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::SouthWest) ? "\u2199" : " ");
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::South) ? "\u2193" : " ");
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::SouthEast) ? "\u2198" : " ");
					ss << "|";
				}
				ss << std::endl;

				ss << (r+1) << "   |";
				for (int f = 0; f <= 7; f++) {
					ss << "---|";
				}
				ss << std::endl;
			}
		}
		/*
		for (auto color : { Color::White, Color::Black }) {
			ss << "Attacks from ";
			ss << (color == Color::White ? "white" : "black");
			ss << " (ortho and diagonal):" << std::endl;
			for (int r = 7; r >= 0; r--) {
				ss << (r+1) << "   |";

				for (auto d = 0; d < directions.size(); d++) {
					for (int f = 0; f <= 7; f++) {
						auto da = directions[d];
						bool attacked = isUnderAttackFromDirection({r, f}, color, da.first);
						ss << (attacked ? da.second : "\u00b7");
					}
					ss << "|  ";
					if (d != directions.size() - 1) ss << (r+1) << "|";
				}
				ss << std::endl;
			}
			for (int i = 0; i < 8; i++) ss << "     " << fileNames;
			ss << std::endl;
		}
		*/

		return ss.str();
	}

	void CBoard::updateUnderAttack() {
		for (auto r = 0; r <= 7; r++) {
			for (auto f = 0; f <= 7; f++) {
				updateUnderAttackFrom({r, f});
			}
		}
	}

	void CBoard::updateUnderAttackFrom(Position position) {
		auto maybePiece = getPiece(position);
		if (!maybePiece.has_value()) return;
		auto piece = maybePiece.value();

		auto r = position.rank;
		auto f = position.file;

		// Bishop, Rook, Queen
		auto& directions =
			(piece.pieceType == PieceType::Queen)  ? allDirections : (
			(piece.pieceType == PieceType::Rook)   ? cardinalDirections : (
			(piece.pieceType == PieceType::Bishop) ? diagonalDirections : (
			noDirections)));
		for (auto direction : directions) {
			auto offset = directionToOffset(direction);
			for (int cr = r + offset.first, cf = f + offset.second;
			     cr >= 0 && cr <= 7 && cf >= 0 && cr <= 7;
				 cr += offset.first, cf += offset.second
			) {
				attackedBy[(int)piece.color][cr][cf] |= oppositeDirection(direction);
				if (pieces[cr][cf].pieceType != PieceType::None) break;
			}
		}

		// Knight
		if (piece.pieceType == PieceType::Knight) {
			for (auto kdirection : knightDirections) {
				auto offset = knightDirectionToOffset(kdirection);
				auto cr = r + offset.first;
				auto cf = f + offset.second;
				if (cr < 0 || cr > 7 || cf < 0 || cf > 7) continue;
				attackedByKnight[(int)piece.color][cr][cf] |= oppositeKnightDirection(kdirection);
			}
		}

		// Kings
		if (piece.pieceType == PieceType::King) {
			for (auto direction : allDirections) {
				auto offset = directionToOffset(direction);
				auto cr = r + offset.first;
				auto cf = f + offset.second;
				if (cr < 0 || cr > 7 || cf < 0 || cf > 7) continue;
				attackedBy[(int)piece.color][cr][cf] |= oppositeDirection(direction);
			}
		}

		// Pawns
		if (piece.pieceType == PieceType::Pawn || piece.pieceType == PieceType::EnPassantCapturablePawn) {
			auto rdiff = piece.color == Color::White ? 1 : -1;
			for (auto direction : diagonalDirections) {
				auto offset = directionToOffset(direction);
				if (offset.first != rdiff) continue;
				auto cr = r + offset.first;
				auto cf = f + offset.second;
				if (cr < 0 || cr > 7 || cf < 0 || cf > 7) continue;
				attackedBy[(int)piece.color][cr][cf] |= oppositeDirection(direction);
			}
		}
	}

	std::unique_ptr<CBoard> CBoard::fromFen(const Fen& fen) {
		auto board = std::make_unique<CBoard>();

		auto ss = std::stringstream(fen.fen);

		std::string pieces; ss >> pieces;
		auto rank = 7;
		auto file = 0;
		for (auto c : pieces) {
			if (c == '/') {
				rank -= 1;
				file = 0;
			}
			else if (c >= '1' && c <= '8') {
				int skips = c - '0';
				for (int i = 0; i < skips; i++) {
					board->pieces[rank][file + i] = { PieceType::None, Color::White };
				}
				file += skips;
			}
			else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
				auto color = (c >= 'a' && c <= 'z') ? Color::Black : Color::White;
				auto pieceType = toPieceType(c);
				board->pieces[rank][file] = { pieceType, color };
				file += 1;
				if (pieceType == PieceType::King) {
					board->kingPosition[(int)color] = { rank, file };
				}
			}

		}

		char mover; ss >> mover;
		board->nextMover = mover == 'w' ? Color::White : Color::Black;

		std::string castling; ss >> castling;
		for (auto c : castling) {
			if (c == 'K') board->castlingRights[(int)Color::White][ShortCastle] = true;
			if (c == 'Q') board->castlingRights[(int)Color::White][LongCastle] = true;
			if (c == 'k') board->castlingRights[(int)Color::White][ShortCastle] = true;
			if (c == 'q') board->castlingRights[(int)Color::White][LongCastle] = true;
		}

		std::string enpassant; ss >> enpassant;
		if (enpassant != "-") {
			auto enpassantFile = enpassant[0] - 'a';
			auto enpassantRank = enpassant[1] - '1';

			if (enpassantRank == 2) {
				board->pieces[3][enpassantFile].pieceType = PieceType::EnPassantCapturablePawn;
			}
			else {
				board->pieces[4][enpassantFile].pieceType = PieceType::EnPassantCapturablePawn;
			}
		}

		board->updateUnderAttack();
		return board;
	}
}