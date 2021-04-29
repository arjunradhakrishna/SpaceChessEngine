#include "counting_board.h"
#include "squares.h"
#include "direction.h"
#include <sstream>
#include <bit>
#include <cmath>

namespace {
	using namespace space;

	PieceType toPieceType(char c) {
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

	bool inDirection(Position source, Position destination) {
		auto rankDiff = abs(source.rank - destination.rank);
		auto fileDiff = abs(source.file - destination.file);
		return rankDiff == 0 || fileDiff == 0 || rankDiff == fileDiff;
	}

	std::pair<Direction, int> getDirectionAndDistance(Position source, Position destination) {
		Direction direction;
		int distance;
		auto rankDiff = source.rank - destination.rank;
		auto fileDiff = source.file - destination.file;
		if (source.rank == destination.rank) {
			direction = source.file < destination.file ? Direction::East : Direction::West;
			distance = abs(fileDiff);
		}
		else if (source.file == destination.file) {
			direction = source.file < destination.file ? Direction::North : Direction::South;
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

	std::optional<IBoard::Ptr> CBoard::updateBoard(Move move) const {
		if (!isValidMove(move)) return {};

		auto newBoard = clone();

		if (pieces[move.sourceRank][move.destinationRank].pieceType == PieceType::King &&
			abs(move.sourceFile - move.destinationFile) == 2) {
			// Castling.
			// Remove king. Remove rook. Add rook. Add king.
			auto rookFile = move.destinationRank > move.sourceRank ? 7 : 0;
			auto rookDestinationFile = move.destinationRank > move.sourceRank ? 5 : 3;
			auto rookRank = nextMover == Color::White ? 0 : 7;
			newBoard->removePieceAt({ move.sourceRank, move.sourceFile });
			newBoard->removePieceAt({ rookRank, rookFile });
			newBoard->addPieceAt({ PieceType::King, nextMover }, { move.destinationRank, move.destinationFile });
			newBoard->addPieceAt({ PieceType::Rook, nextMover }, { move.destinationRank, rookFile });
		}
		else if (pieces[move.sourceRank][move.sourceFile].pieceType == PieceType::Pawn
			&& move.sourceFile != move.destinationFile
			&& pieces[move.destinationRank][move.destinationFile].pieceType == PieceType::None
		) {
			// En passant
			// Remove captured pawn. Remove capturing pawn. Add capturing pawn.
			newBoard->removePieceAt({ move.sourceRank, move.sourceFile });
			newBoard->removePieceAt({ move.sourceRank, move.destinationFile });
			newBoard->addPieceAt({ PieceType::Pawn, nextMover }, { move.destinationRank, move.destinationFile });
		}
		else {
			newBoard->removePieceAt({ move.sourceRank, move.sourceFile });
			if (pieces[move.destinationRank][move.destinationFile].pieceType != PieceType::None) {
				newBoard->removePieceAt({ move.destinationRank, move.destinationFile });
			}
			newBoard->addPieceAt(pieces[move.sourceRank][move.sourceFile], { move.destinationRank, move.destinationFile });
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
		switch (movingPieceType) {
			case PieceType::Queen: if (!isOrtho && !isDiagonal) return false; break;
			case PieceType::Rook: if (!isOrtho) return false; break;
			case PieceType::Bishop: if (!isDiagonal) return false; break;
			case PieceType::Knight:
				if (!((rankDiff == 2 && fileDiff == 1) || (rankDiff == 1 && fileDiff == 2))) return false;
				break;
			case PieceType::King:
				if (!isOrtho && !isDiagonal) return false;
				if (fileDiff == 2 && rankDiff == 0 && !isValidCastle(move)) return false;
				if (rankDiff > 1 || fileDiff > 1) return false;
				if (isUnderAttack({ move.destinationRank, move.destinationFile}, otherColor)) return false;
				break;
			case PieceType::Pawn:
			case PieceType::EnPassantCapturablePawn:
				auto startRank = nextMover == Color::White ? 1 : 6;
				auto enpassantRank = nextMover == Color::White ? 4 : 3;
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
				if (fileDiff == 1 && rankDiff == 1 && move.sourceRank == enpassantRank &&
				    pieces[enpassantRank][move.destinationFile].pieceType == PieceType::EnPassantCapturablePawn) break;
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
		if (inDirection(kingPos, sourcePosition)) {
			// Can be pinned.
			auto [direction, dist] = getDirectionAndDistance(kingPos, sourcePosition);
			if (isUnderAttackFromDirection(sourcePosition, otherColor, oppositeDirection(direction))) {
				return false;
			}
		}

		// Your king is in check and you don't fix it.
		if (isUnderCheck(nextMover)) return isValidResponseToCheck(move, false);

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

		if (inDirection(kingPos, destinationPosition) && inDirection(kingPos, attackingPosition)) {
			auto [blockDir, blockDist] = getDirectionAndDistance(kingPos, destinationPosition);
			auto [attackDir, attackDist] = getDirectionAndDistance(kingPos, destinationPosition);
			if (blockDir == attackDir && blockDist <= attackDist) return true;
		}

		return false;
	}

	Position CBoard::getAttackingPosition(Position position, Color attackingColor) const {
		auto attacks = attackedBy[(int)attackingColor][position.rank][position.file];
		auto knightAttacks = attackedByKnight[(int)attackingColor][position.rank][position.file];
		if (attacks != 0) {
			for (auto dir : allDirections) {
				if (attacks & dir != 0) {
					auto offset = directionToOffset(dir);
					for (int i = 0; i < 7; i++) {
						auto r = position.rank + i * offset.first;
						auto f = position.file + i * offset.second;
						if (pieces[r][f].pieceType != PieceType::None) return { r, f };
					}
				}
			}
		}
		for (auto kdir : knightDirections) {
			if (knightAttacks & kdir) {
				auto offset = knightDirectionToOffset(kdir);
				return { position.rank + offset.first, position.file + offset.second };
			}
		}
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
			     cr >= 0 && cr <= 7 && cf >= 0 && cr <= 7;
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
		if (piece.pieceType == PieceType::Pawn || piece.pieceType == PieceType::EnPassantCapturablePawn) {
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
}