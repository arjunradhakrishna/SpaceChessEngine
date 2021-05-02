#include "counting_board.h"
#include "squares.h"
#include "direction.h"
#include "debug.h"
#include <sstream>
#include <bit>
#include <cmath>
#include <iostream>

namespace {
	using namespace space;

	PieceType toPieceType(char c) {
		// Pawn, Rook, Knight, Bishop, Queen, King, None
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

	bool inDirection(Position source, Position destination) {
		auto rankDiff = abs(source.rank - destination.rank);
		auto fileDiff = abs(source.file - destination.file);
		return rankDiff == 0 || fileDiff == 0 || rankDiff == fileDiff;
	}

	std::pair<Direction, int> getDirectionAndDistance(Position source, Position destination) {
		Direction direction;
		int distance;
		auto rankDiff = destination.rank - source.rank;
		auto fileDiff = destination.file - source.file;
		if (source.rank == destination.rank) {
			direction = source.file < destination.file ? Direction::East : Direction::West;
			distance = abs(fileDiff);
		}
		else if (source.file == destination.file) {
			direction = source.rank < destination.rank ? Direction::North : Direction::South;
			distance = abs(rankDiff);
		}
		else if (abs(rankDiff) == abs(fileDiff) && rankDiff > 0 && fileDiff > 0) {
			direction = Direction::NorthEast;
			distance = abs(rankDiff);
		}
		else if (abs(rankDiff) == abs(fileDiff) && rankDiff > 0 && fileDiff < 0) {
			direction = Direction::NorthWest;
			distance = abs(rankDiff);
		}
		else if (abs(rankDiff) == abs(fileDiff) && rankDiff < 0 && fileDiff > 0) {
			direction = Direction::SouthEast;
			distance = abs(rankDiff);
		}
		else if (abs(rankDiff) == abs(fileDiff) && rankDiff < 0 && fileDiff < 0) {
			direction = Direction::SouthWest;
			distance = abs(rankDiff);
		}
		return { direction, distance };
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

	inline bool CBoard::isUnderAttackFromKnightDirection(Position position, Color attackingColor, KnightDirection knightDirection) const {
		return (attackedByKnight[(int)attackingColor][position.rank][position.file] & knightDirection) != 0;
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
		return newBoard;
	}

	std::string CBoard::attackString() const {
		std::stringstream ss;
		auto fileNames = "abcdefgh";

		for (auto color : { Color::White, Color::Black }) {
			ss << "Attacks from ";
			ss << (color == Color::White ? "white" : "black");
			ss << " (ortho and diagonal):" << std::endl;

			ss << "|";
			for (int f = 0; f <= 7; f++) {
				ss << "----|";
			}
			ss << std::endl;

			for (int r = 7; r >= 0; r--) {
				ss << "|";
				for (int f = 0; f <= 7; f++) {
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::NorthWest) ? "\u2196" : " ");
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::North) ? "\u2191\u2191" : "  ");
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::NorthEast) ? "\u2197" : " ");
					ss << "|";
				}
				ss << std::endl;

				ss << "|";
				for (int f = 0; f <= 7; f++) {
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::West) ? "\u2190" : " ");
					ss << (char)(f + 'a') << (r+1);
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::East) ? "\u2192" : " ");
					ss << "|";
				}
				ss << std::endl;

				ss << "|";
				for (int f = 0; f <= 7; f++) {
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::SouthWest) ? "\u2199" : " ");
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::South) ? "\u2193\u2193" : "  ");
					ss << (isUnderAttackFromDirection({r,f}, color, Direction::SouthEast) ? "\u2198" : " ");
					ss << "|";
				}
				ss << std::endl;

				ss << "|";
				for (int f = 0; f <= 7; f++) {
					ss << "----|";
				}
				ss << std::endl;
			}
		}

		for (auto color : { Color::White, Color::Black }) {
			ss << "Attacks from ";
			ss << (color == Color::White ? "white" : "black");
			ss << " (knights):" << std::endl;

			for (int f = 0; f <= 7; f++) {
				for (int r = 0; r <= 7; r++) {
					for (auto kDir : knightDirections) {
						if (isUnderAttackFromKnightDirection({r,f}, color, kDir)) {
							auto offset = knightDirectionToOffset(kDir);
							ss << "\t" << (char)('a' + f) << (r + 1) << " is attacked from ";
							ss << "\t" << (char)('a' + f + offset.second) << (r + offset.first + 1);
							ss << std::endl;
						}
					}
				}
			}
		}
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
			     cr >= 0 && cr <= 7 && cf >= 0 && cf <= 7;
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
		if (piece.pieceType == PieceType::Pawn) {
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

	IBoard::Ptr CBoard::fromFen(const Fen& fen) {
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
				if (pieceType == PieceType::King) {
					board->kingPosition[(int)color] = { rank, file };
				}
				file += 1;
			}
		}

		char mover; ss >> mover;
		board->nextMover = mover == 'w' ? Color::White : Color::Black;

		std::string castling; ss >> castling;
		for (auto c : castling) {
			if (c == 'K') board->castlingRights[(int)Color::White][ShortCastle] = true;
			if (c == 'Q') board->castlingRights[(int)Color::White][LongCastle] = true;
			if (c == 'k') board->castlingRights[(int)Color::Black][ShortCastle] = true;
			if (c == 'q') board->castlingRights[(int)Color::Black][LongCastle] = true;
		}

		std::string enpassant; ss >> enpassant;
		if (enpassant != "-") {
			auto enpassantFile = enpassant[0] - 'a';
			auto enpassantRank = enpassant[1] - '1';
			board->enPassantSquare = { enpassantRank, enpassantFile };
		}

		board->updateUnderAttack();
		return board;
	}

	std::optional<IBoard::Ptr> CBoard::updateBoard(Move move) const {
		if (!isValidMove(move)) return {};

		auto newBoard = clone();
		auto movingPiece = pieces[move.sourceRank][move.sourceFile];

		newBoard->nextMover = oppositeColor(nextMover);
		if (movingPiece.pieceType == PieceType::King) {
			newBoard->kingPosition[(int)nextMover] = { move.destinationRank, move.destinationFile };
		}
		newBoard->enPassantSquare = {};

		auto baseRank = nextMover == Color::White ? 0 : 7;
		// Update own castling rights.
		if (movingPiece.pieceType == PieceType::King) {
			newBoard->castlingRights[(int)nextMover][ShortCastle] = false;
			newBoard->castlingRights[(int)nextMover][LongCastle] = false;
		}
		else if (move.sourceRank == baseRank && move.sourceFile == 0) { // No need to check if it is a rook
			newBoard->castlingRights[(int)nextMover][LongCastle] = false;
		}
		else if (move.sourceRank == baseRank && move.sourceFile == 7) {
			newBoard->castlingRights[(int)nextMover][ShortCastle] = false;
		}

		// Update opposition castling rights.
		auto backRank = nextMover == Color::White ? 7 : 0;
		auto otherColor = oppositeColor(nextMover);
		if (move.destinationRank == backRank && move.destinationFile == 0) { // No need to check if it is a rook
			newBoard->castlingRights[(int)otherColor][LongCastle] = false;
		}
		else if (move.destinationRank == backRank && move.destinationFile == 7) {
			newBoard->castlingRights[(int)otherColor][ShortCastle] = false;
		}

		// Set en passant square
		if (movingPiece.pieceType == PieceType::Pawn && abs(move.sourceRank - move.destinationRank) == 2) {
			auto enPassantRank = nextMover == Color::White ? 2 : 5;
			newBoard->enPassantSquare = { enPassantRank, move.sourceFile };
		}

		if (movingPiece.pieceType == PieceType::King && abs(move.sourceFile - move.destinationFile) == 2) {
			// Castling.
			// Remove king. Remove rook. Add rook. Add king.
			auto rookFile = move.destinationFile > move.sourceFile ? 7 : 0;
			auto rookDestinationFile = move.destinationFile > move.sourceFile ? 5 : 3;
			auto rookRank = nextMover == Color::White ? 0 : 7;
			newBoard->removePieceAt({ move.sourceRank, move.sourceFile });
			newBoard->removePieceAt({ rookRank, rookFile });
			newBoard->addPieceAt({ PieceType::King, nextMover }, { move.destinationRank, move.destinationFile });
			newBoard->addPieceAt({ PieceType::Rook, nextMover }, { move.destinationRank, rookDestinationFile });
		}
		else if (movingPiece.pieceType == PieceType::Pawn
			&& move.sourceFile != move.destinationFile
			&& pieces[move.destinationRank][move.destinationFile].pieceType == PieceType::None
		) {
			// En passant
			// Remove capturing pawn. Remove captured pawn. Add capturing pawn.
			newBoard->removePieceAt({ move.sourceRank, move.sourceFile });
			newBoard->removePieceAt({ move.sourceRank, move.destinationFile });
			newBoard->addPieceAt({ PieceType::Pawn, nextMover }, { move.destinationRank, move.destinationFile });
		}
		else if (move.promotedPiece != PieceType::None) {
			newBoard->removePieceAt({ move.sourceRank, move.sourceFile });
			if (pieces[move.destinationRank][move.destinationFile].pieceType != PieceType::None) {
				newBoard->removePieceAt({ move.destinationRank, move.destinationFile });
			}
			newBoard->addPieceAt({ move.promotedPiece, nextMover }, { move.destinationRank, move.destinationFile });
		}
		else {
			newBoard->removePieceAt({ move.sourceRank, move.sourceFile });
			if (pieces[move.destinationRank][move.destinationFile].pieceType != PieceType::None) {
				newBoard->removePieceAt({ move.destinationRank, move.destinationFile });
			}
			newBoard->addPieceAt(movingPiece, { move.destinationRank, move.destinationFile });
		}
		return newBoard;
	}

	bool CBoard::isValidMove(Move move) const {
		auto otherColor = oppositeColor(nextMover);

		// Trivial things.
		if (
			pieces[move.sourceRank][move.sourceFile].color != nextMover || // Moving wrong color piece
			pieces[move.sourceRank][move.sourceFile].pieceType == PieceType::None || // Moving no piece
			(pieces[move.destinationRank][move.destinationFile].pieceType != PieceType:: None
			 && pieces[move.destinationRank][move.destinationFile].color == nextMover) // Capturing own piece.
		) {
			return false;
		}

		// Move semantics
		auto movingPieceType = pieces[move.sourceRank][move.sourceFile].pieceType;
		auto rankDiff = abs(move.sourceRank - move.destinationRank);
		auto fileDiff = abs(move.sourceFile - move.destinationFile);
		auto isOrtho = fileDiff == 0 || rankDiff == 0;
		auto isDiagonal = fileDiff == rankDiff;
		auto isCastle = fileDiff == 2 && rankDiff == 0;
		switch (movingPieceType) {
			case PieceType::Queen: if (!isOrtho && !isDiagonal) return false; break;
			case PieceType::Rook: if (!isOrtho) return false; break;
			case PieceType::Bishop: if (!isDiagonal) return false; break;
			case PieceType::Knight:
				if (!((rankDiff == 2 && fileDiff == 1) || (rankDiff == 1 && fileDiff == 2))) return false;
				break;
			case PieceType::King:
				if (!isOrtho && !isDiagonal) return false;
				if (isCastle && !isValidCastle(move)) return false;
				if (!isCastle && (rankDiff > 1 || fileDiff > 1)) return false;
				if (isUnderAttack({ move.destinationRank, move.destinationFile}, otherColor)) return false;
				break;
			case PieceType::Pawn:
				auto startRank = nextMover == Color::White ? 1 : 6;
				// Move in wrong direction
				if ((move.sourceRank < move.destinationRank) != (nextMover == Color::White)) return false;
				// Move 1 square
				if (fileDiff == 0 && rankDiff == 1) break;
				// Move 2 squares from starting square
				if (fileDiff == 0 && rankDiff == 2 && move.sourceRank == startRank) break;
				// Standard capture
				if (fileDiff == 1 && rankDiff == 1 &&
				    pieces[move.destinationRank][move.destinationFile].pieceType != PieceType::None) break;
				// Enpassant capture
				if (fileDiff == 1 && rankDiff == 1 && enPassantSquare.has_value() &&
				    move.destinationRank == enPassantSquare.value().rank &&
				    move.destinationFile == enPassantSquare.value().file) break;
				return false;
		}

		// Not jumping over pieces (unless knight)
		if (movingPieceType != PieceType::Knight) {
			// Everything between start and destination square should be empty.
			auto [dir, distance] = getDirectionAndDistance(
				{move.sourceRank, move.sourceFile},
				{move.destinationRank, move.destinationFile}
			);
			auto offset = directionToOffset(dir);
			for (auto i = 1; i < distance; i++) {
				auto p = pieces[move.sourceRank + i * offset.first][move.sourceFile + i * offset.second];
				if (p.pieceType != PieceType::None) return false;
			}
		}

		// Some piece that was pinned to the king moves off that
		// file, rank, or diagonal.
		auto kingPos = kingPosition[(int)nextMover];
		Position sourcePosition = { move.sourceRank, move.sourceFile };
		if (movingPieceType != PieceType::King && inDirection(kingPos, sourcePosition)) {
			// Can be pinned.
			auto [direction, dist] = getDirectionAndDistance(kingPos, sourcePosition);

			// Was there something between the moving piece and the king?
			auto blocked = false;
			auto offset = directionToOffset(direction);
			for (auto d = 1; d < dist; d++) {
				auto r = kingPos.rank + d * offset.first;
				auto f = kingPos.file + d * offset.second;
				if (pieces[r][f].pieceType != PieceType::None) {
					blocked = true;
					break;
				}
			}

			if (!blocked && isUnderAttackFromDirection(sourcePosition, otherColor, oppositeDirection(direction))) {
				return false;
			}
		}

		// Your king is in check and you don't fix it.
		if (isUnderCheck(nextMover)) {
			auto valid = isValidResponseToCheck(move, false);
		}

		return true;
	}

	bool CBoard::isValidCastle(Move move) const {
		// We assume that the right color and piece are moving.
		if (move.sourceRank != move.destinationRank) return false;
		if (abs(move.sourceFile - move.destinationFile) != 2) return false;
		auto castleType = move.destinationFile > move.sourceFile ? ShortCastle : LongCastle;
		if (!castlingRights[(int)nextMover][castleType]) return false;

		// Everything between king and rook should be empty.
		auto rookFile = castleType == ShortCastle ? 7 : 0;
		for (int f = std::min(rookFile, 4) + 1; f < std::max(rookFile, 4); f++) {
			if (pieces[move.sourceRank][f].pieceType != PieceType::None) return false;
		}

		// King should not castle out of, through, or in to check.
		auto attackingColor = oppositeColor(nextMover);
		auto middleFile = move.sourceFile + (castleType == ShortCastle ? 1 : -1);
		if (
			isUnderAttack({move.sourceRank, move.sourceFile}, attackingColor) ||
			isUnderAttack({move.sourceRank, move.destinationFile}, attackingColor) ||
			isUnderAttack({move.sourceRank, middleFile}, attackingColor)
		) {
			return false;
		}

		return true;
	}

	bool CBoard::isValidResponseToCheck(Move move, bool checkForPins) const {
		auto otherColor = oppositeColor(nextMover);
		auto movingPieceType = pieces[move.sourceRank][move.sourceFile].pieceType;
		if (movingPieceType == PieceType::King) { // King move
			return attackedBy[(int)otherColor][move.destinationRank][move.destinationFile] == 0;
		}

		// If it is a double check, then the king has to move. 
		auto kingPos = kingPosition[(int)nextMover];
		auto numKnightChecks = std::popcount(attackedByKnight[(int)otherColor][kingPos.rank][kingPos.file]);
		auto numOtherChecks = std::popcount(attackedBy[(int)otherColor][kingPos.rank][kingPos.file]);
		if (numOtherChecks + numKnightChecks > 1) return false;

		// Also, moving piece cannot be pinned.
		Position sourcePosition = { move.sourceRank, move.sourceFile };
		Position destinationPosition = { move.destinationRank, move.destinationFile };
		if (checkForPins) {
			if (inDirection(kingPos, sourcePosition)) {
				// Can be pinned.
				auto [direction, dist] = getDirectionAndDistance(kingPos, sourcePosition);
				if (isUnderAttackFromDirection(sourcePosition, otherColor, oppositeDirection(direction))) {
					return false;
				}
			}
		}

		// Some other piece moved.
		// Capture
		auto attackingPosition = getAttackingPosition(kingPos, otherColor);
		if (attackingPosition.rank == move.destinationRank &&
			attackingPosition.file == move.destinationFile) {
			return true;
		}

		// Blocks
		if (inDirection(kingPos, destinationPosition) && inDirection(kingPos, attackingPosition)) {
			auto [blockDir, blockDist] = getDirectionAndDistance(kingPos, destinationPosition);
			auto [attackDir, attackDist] = getDirectionAndDistance(kingPos, destinationPosition);
			if (blockDir == attackDir && blockDist <= attackDist) return true;
		}

		return false;
	}

	// Can we stream this? How do we do such things in C++?
	// Coroutines? Streams? Lazy evaluation?
	std::vector<Position> CBoard::getAllAttackingPositions(Position position, Color attackingColor) const {
		std::vector<Position> result;

		auto attacks = attackedBy[(int)attackingColor][position.rank][position.file];
		auto knightAttacks = attackedByKnight[(int)attackingColor][position.rank][position.file];
		if (attacks != 0) {
			for (auto dir : allDirections) {
				if (attacks & dir) {
					auto offset = directionToOffset(dir);
					for (int i = 1; i <= 7; i++) {
						auto r = position.rank + i * offset.first;
						auto f = position.file + i * offset.second;
						if (r < 0 || r > 7 || f < 0 || f > 7) break;
						if (pieces[r][f].pieceType != PieceType::None) {
							result.push_back({ r, f });
							break;
						}
					}
				}
			}
		}
		if (knightAttacks != 0) {
			for (auto kdir : knightDirections) {
				if (knightAttacks & kdir) {
					auto offset = knightDirectionToOffset(kdir);
					result.push_back({ position.rank + offset.first, position.file + offset.second });
				}
			}
		}

		return result;
	}

	Position CBoard::getAttackingPosition(Position position, Color attackingColor) const {
		auto attacks = attackedBy[(int)attackingColor][position.rank][position.file];
		auto knightAttacks = attackedByKnight[(int)attackingColor][position.rank][position.file];
		if (attacks != 0) {
			for (auto dir : allDirections) {
				if (attacks & dir) {
					auto offset = directionToOffset(dir);
					for (int i = 1; i <= 7; i++) {
						auto r = position.rank + i * offset.first;
						auto f = position.file + i * offset.second;
						if (r < 0 || r > 7 || f < 0 || f > 7) break;
						if (pieces[r][f].pieceType != PieceType::None) return { r, f };
					}
				}
			}
		}
		if (knightAttacks != 0) {
			for (auto kdir : knightDirections) {
				if (knightAttacks & kdir) {
					auto offset = knightDirectionToOffset(kdir);
					return { position.rank + offset.first, position.file + offset.second };
				}
			}
		}
		return { -1000000, -1000000 };
	}

	void CBoard::addPieceAt(Piece piece, Position position) {
		// Add piece.
		pieces[position.rank][position.file] = piece;

		// Block all directional attacks.
		for (auto direction : allDirections) {
			for (auto color : { Color::White, Color::Black }) {
				auto isAttacked = (attackedBy[(int)color][position.rank][position.file] & direction) != 0;
				if (!isAttacked) continue;

				auto offset = directionToOffset(oppositeDirection(direction));
				auto r = position.rank + offset.first;
				auto f = position.file + offset.second;
				while (0 <= r && r <= 7 && 0 <= f && f <= 7) {
					attackedBy[(int)color][r][f] &= (~direction);
					if (pieces[r][f].pieceType != PieceType::None) break;
					r += offset.first;
					f += offset.second;
				}
			}
		}

		// Add attacks by this piece.
		updateUnderAttackFrom(position);
	}

	void CBoard::removePieceAt(Position position) {
		auto piece = pieces[position.rank][position.file];
		pieces[position.rank][position.file] = { PieceType::None, Color::White };
		auto r = position.rank;
		auto f = position.file;

		// Remove attacks from this piece.
		auto& directions =
			(piece.pieceType == PieceType::Queen)  ? allDirections : (
			(piece.pieceType == PieceType::Rook)   ? cardinalDirections : (
			(piece.pieceType == PieceType::Bishop) ? diagonalDirections : (
			noDirections)));
		for (auto direction : directions) {
			auto offset = directionToOffset(direction);
			for (int cr = r + offset.first, cf = f + offset.second;
			     cr >= 0 && cr <= 7 && cf >= 0 && cf <= 7;
				 cr += offset.first, cf += offset.second
			) {
				attackedBy[(int)piece.color][cr][cf] &= (~oppositeDirection(direction));
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
				attackedByKnight[(int)piece.color][cr][cf] &= (~oppositeKnightDirection(kdirection));
			}
		}

		// Kings
		if (piece.pieceType == PieceType::King) {
			for (auto direction : allDirections) {
				auto offset = directionToOffset(direction);
				auto cr = r + offset.first;
				auto cf = f + offset.second;
				if (cr < 0 || cr > 7 || cf < 0 || cf > 7) continue;
				attackedBy[(int)piece.color][cr][cf] &= (~oppositeDirection(direction));
			}
		}

		// Pawns
		if (piece.pieceType == PieceType::Pawn) {
			auto rdiff = piece.color == Color::White ? 1 : -1;
			for (auto direction : diagonalDirections) {
				auto offset = directionToOffset(direction);
				if (offset.first != rdiff) continue;
				auto cr = r + offset.first;
				auto cf = f + offset.second;
				if (cr < 0 || cr > 7 || cf < 0 || cf > 7) continue;
				attackedBy[(int)piece.color][cr][cf] &= (~oppositeDirection(direction));
			}
		}

		// Unblock directional attacks.
		for (auto direction : allDirections) {
			for (auto color : { Color::White, Color::Black }) {
				auto isAttacked = (attackedBy[(int)color][position.rank][position.file] & direction) != 0;
				if (!isAttacked) continue;

				auto offset = directionToOffset(oppositeDirection(direction));

				// Pawn or king move?
				auto adjacentPiece = pieces[position.rank - offset.first][position.file - offset.second];
				if (adjacentPiece.pieceType == PieceType::King || adjacentPiece.pieceType == PieceType::Pawn) {
					continue;
				}

				auto r = position.rank + offset.first;
				auto f = position.file + offset.second;
				while (0 <= r && r <= 7 && 0 <= f && f <= 7) {
					attackedBy[(int)color][r][f] |= direction;
					if (pieces[r][f].pieceType != PieceType::None) break;
					r += offset.first;
					f += offset.second;
				}
			}
		}
	}

	bool CBoard::isCheckMate() const {
		if (!isUnderCheck(nextMover)) return false;
		auto oppColor = oppositeColor(nextMover);

		// Getting out of checks:
		// 1. King moves.
		// 2. Capture attacking piece (can't do if double check)
		// 3. Block the check (can't do if double check)
		auto kPos = kingPosition[(int)nextMover];
		auto kRank = kPos.rank;
		auto kFile = kPos.file;
		auto checkDirs = attackedBy[(int)oppColor][kRank][kFile];
		for (auto moveDir : allDirections) {
			auto offset = directionToOffset(moveDir);
			auto rank = kRank + offset.first;
			auto file = kFile + offset.second;
			if (rank < 0 || rank > 7 || file < 0 || file > 7) continue; // out of bounds

			if (pieces[rank][file].pieceType != PieceType::None &&
				pieces[rank][file].color == nextMover) continue;  // can't capture own pieces
			if (isUnderAttack({rank, file}, oppColor)) continue;

			// You can move away from the rank/file/diagonal the check is on.
			auto oppDir = oppositeDirection(moveDir);
			if ((moveDir & checkDirs) == 0 &&  (oppDir & checkDirs) == 0) {
				return false;
			}
			else {
				// You have to capture attacking piece or move away from a pawn check.
				if ((moveDir & checkDirs)) {
					// Moving towards attacking piece. Have to capture.
					// Whether the attacking piece is supported is already checked by isUnderAttack.
					if (pieces[rank][file].pieceType != PieceType::None) return false;
				}
				else {
					// Move away from attacking piece. Has to be a pawn.
					auto pawnOpt = pieces[kRank - offset.first][kFile - offset.second].pieceType;
					if (pawnOpt != PieceType::Pawn) continue;
				}
			}

			return false;  // Have non-attacked square.
		}
		// debug << "King has no legal moves." << std::endl;

		// Double checks. Can't capture attacking piece and can't block.
		auto numChecks = std::popcount(attackedBy[(int)oppColor][kRank][kFile]) + 
			std::popcount(attackedByKnight[(int)oppColor][kRank][kFile]);
		if (numChecks > 1) return true;
		// debug << "Not a double check." << std::endl;

		// Captures and blocks can be handled them together.
		// We should have a non-pinned piece that can move to the blocking or capturing square.
		auto attackingPosition = getAttackingPosition(kPos, oppColor);
		auto attackingPiece = pieces[attackingPosition.rank][attackingPosition.file].pieceType;
		// debug << "Attacking position: " << (char)('a' + attackingPosition.file) << (attackingPosition.rank + 1) << std::endl;
		if (attackingPiece == PieceType::Knight) { // Knights can't be blocked. Only captured.
			return !hasLegalMoveTo(attackingPosition, nextMover);
		}
		else {
			auto [attackDir, attackDist] = getDirectionAndDistance(kPos, attackingPosition);
			auto offset = directionToOffset(attackDir);
			for (auto i = 1; i <= attackDist; i++) { // <= to handle captures.
				auto rank = kRank + i * offset.first;
				auto file = kFile + i * offset.second;

				// Can a piece move here legally?
				if (hasLegalMoveTo({ rank, file }, nextMover)) return false;
			}
		}

		// En passants
		if (attackingPiece == PieceType::Pawn
			&& enPassantSquare.has_value()
			&& attackingPosition.file == enPassantSquare.value().file
			&& attackingPosition.rank == (nextMover == Color::White ? 4 : 3) // En passant
		) {
			for (auto fDiff : { 1, -1 }) {
				auto f = attackingPosition.file + fDiff;
				if (f < 0 || f > 7) continue;
				auto piece = pieces[attackingPosition.rank][f];
				if (piece.pieceType == PieceType::Pawn &&
				    piece.color == nextMover &&
					!isPinnedToKing({attackingPosition.rank, f}, nextMover)) {
					return false;
				}
			}
		}

		return true;
	}

	bool CBoard::isPinnedToKing(Position position, Color defendingColor) const {
		// This pin does not necessarily mean the piece can't move.
		// The pinned piece can potentially capture the pinning piece.
		auto kPos = kingPosition[(int)defendingColor];
		if (!inDirection(kPos, position)) {
			return false;
		}

		auto attackingColor = oppositeColor(defendingColor);
		auto [ dir, dist ] = getDirectionAndDistance(kPos, position);
		auto isAttackedFromDir = attackedBy[(int)attackingColor][position.rank][position.file] & dir;
		if (isAttackedFromDir == 0) return false;
		auto offset = directionToOffset(dir);

		// If there is something between the piece and the king, it is not a pin.
		for (auto i = 1; i < dist; i++) {
			if (pieces[kPos.rank + i * offset.first][kPos.file + i * offset.second].pieceType != PieceType::None) {
				return false;
			}
		}

		// We have a pin if the attacking piece is not a pawn or king.
		auto closeAttackingPiece = pieces[position.rank + offset.first][position.file + offset.second].pieceType;
		if (closeAttackingPiece == PieceType::King || closeAttackingPiece == PieceType::Pawn) {
			return false;
		}

		return true;
	}

	// Does not check for when the position itself has checks.
	bool CBoard::hasLegalMoveTo(Position toPos, Color color) const {
		// Pawn moves without capture.
		// Pawns can move to a position which they are not attacking.
		if (pieces[toPos.rank][toPos.file].pieceType == PieceType::None) {
			auto moveRankDiff = (color == Color::White) ? 1 : -1;
			Position oneStep = { toPos.rank - moveRankDiff, toPos.file };
			if (oneStep.rank >= 0 && oneStep.rank <= 7) {
				// debug << "Checking non-capture pawn move from: " << (char)(oneStep.file + 'a') << (oneStep.rank + 1) << std::endl;
				if (pieces[oneStep.rank][oneStep.file].pieceType == PieceType::Pawn
					&& pieces[oneStep.rank][oneStep.file].color == color && !isPinnedToKing(oneStep, color)) {
					return true;
				}
				// debug << "No one step move." << std::endl;
			}

			Position twoStep = { toPos.rank - 2 * moveRankDiff, toPos.file };
			if (twoStep.rank == ((color == Color::White) ? 1 : 6)) {
				if (twoStep.rank >= 0 && twoStep.rank <= 7) {
					// debug << "Checking non-capture pawn move from: " << (char)(twoStep.file + 'a') << (twoStep.rank + 1) << std::endl;
					if (pieces[twoStep.rank][twoStep.file].pieceType == PieceType::Pawn
						&& pieces[twoStep.rank][twoStep.file].color == color
						&& pieces[oneStep.rank][oneStep.file].pieceType == PieceType::None
						&& !isPinnedToKing(twoStep, color)) {
						return true;
					}
				}
			}
		}
		// debug << "No non-capture pawn moves." << std::endl;

		// No attacking moves at all.
		if (attackedBy[(int)color][toPos.rank][toPos.file] == 0 &&
			attackedByKnight[(int)color][toPos.rank][toPos.file] == 0) return false;

		// Everything else
		auto candidateFromPositions = getAllAttackingPositions(toPos, color);
		// for (auto p : candidateFromPositions) debug << "Found potential move from: " << (char)(p.file + 'a') << (p.rank + 1) << std::endl;
		auto kPos = kingPosition[(int)color];
		auto oppColor = oppositeColor(color);
		auto checkDirs = attackedBy[(int)oppColor][kPos.rank][kPos.file];
		for (auto fromPos : candidateFromPositions) {
			// debug << "Checking move from "
			    //   << (char)(fromPos.file + 'a') << (fromPos.rank + 1)
				//   << " to "
			    //   << (char)(toPos.file + 'a') << (toPos.rank + 1)
				//   << std::endl;
			auto piece = pieces[fromPos.rank][fromPos.file].pieceType;
			if (piece == PieceType::King) {
				if (isUnderAttack(toPos, oppColor)) continue;
				auto [moveDir, dist] = getDirectionAndDistance(kPos, toPos);
				if ((checkDirs & moveDir) || (checkDirs & oppositeDirection(moveDir))) continue;
			}
			else if (piece == PieceType::Pawn) {
				if (pieces[toPos.rank][toPos.file].pieceType != PieceType::None
					&& pieces[toPos.rank][toPos.file].color == oppColor 
					&& !isPinnedToKing(fromPos, color)) { // Should be a capture
					return true;
				}
			}
			else {
				if (!isPinnedToKing(fromPos, color)) {
					return true;
				}

				// Legal move is not knight

				// Even if you are pinned, you could move in the same file/rank/diagonal.
				// Legal move is not knight
				
				auto [ fDir, fDist ] = getDirectionAndDistance(kPos, fromPos);
				auto [ tDir, tDist ] = getDirectionAndDistance(kPos, toPos);
				if (fDir == tDir) return true;
			}
		}

		return false;
	}
}
