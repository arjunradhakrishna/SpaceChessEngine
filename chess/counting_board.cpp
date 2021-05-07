#include "counting_board.h"
#include "squares.h"
#include "direction.h"
#include "debug.h"
#include <sstream>
#include <string>
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

	inline const std::vector<Direction>& getMoveDirections(const PieceType pieceType) {
		switch (pieceType) {
			case PieceType::Queen: 
			case PieceType::King:
				return allDirections;
			case PieceType::Rook:
				return cardinalDirections;
			case PieceType::Bishop:
				return diagonalDirections;
			case PieceType::Knight:
			case PieceType::Pawn:
			default:
				space_fail("Calling get directions on non-directional piece.");
		}
	}

	inline const bool isLongPiece(const PieceType pieceType) {
		return pieceType == PieceType::Queen
			|| pieceType == PieceType::Rook
			|| pieceType == PieceType::Bishop;
	}

	bool isClearBetween(const CBoard* board, const Position& sourcePosition, const Position& destinationPosition) {
		auto [dir, distance] = getDirectionAndDistance(sourcePosition, destinationPosition);
		auto offset = directionToOffset(dir);
		auto sourceRank = sourcePosition.rank;
		auto sourceFile = sourcePosition.file;
		for (auto i = 1; i < distance; i++) {
			auto p = board->getPiece({ sourceRank + i * offset.first, sourceFile + i * offset.second });
			if (p.has_value()) return false;
		}
		return true;
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

	std::unique_ptr<CBoard> CBoard::startPosition() {
		return fromFen(Fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
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
}

namespace space {
	using namespace space::squares;

	void CBoard::updateUnderAttack() {
		for (auto r = 0; r <= 7; r++)
			for (auto f = 0; f <= 7; f++)
				updateUnderAttackFrom({r, f});
	}

	void CBoard::updateUnderAttackFrom(Position position) {
		auto maybePiece = getPiece(position);
		if (!maybePiece.has_value()) return;
		auto piece = maybePiece.value();

		auto r = position.rank;
		auto f = position.file;

		// Bishop, Rook, Queen
		if (isLongPiece(piece.pieceType)) {
			for (auto direction : getMoveDirections(piece.pieceType)) {
				auto offset = directionToOffset(direction);
				for (int cr = r + offset.first, cf = f + offset.second;
					cr >= 0 && cr <= 7 && cf >= 0 && cf <= 7;
					cr += offset.first, cf += offset.second
				) {
					attackedBy[(int)piece.color][cr][cf] |= oppositeDirection(direction);
					if (pieces[cr][cf].pieceType != PieceType::None) break;
				}
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
		if (move.destinationRank < 0 || move.destinationRank > 7) return false;
		if (move.destinationFile < 0 || move.destinationFile > 7) return false;
		if (move.sourceRank < 0 || move.sourceRank > 7) return false;
		if (move.sourceFile < 0 || move.sourceFile > 7) return false;

		auto movingPiece = pieces[move.sourceRank][move.sourceFile];
		auto movingPieceType = movingPiece.pieceType;
		auto capturedPiece = pieces[move.destinationRank][move.destinationFile];
		if (movingPiece.color != nextMover || // Moving wrong color piece
			movingPieceType == PieceType::None || // Moving no piece
			(capturedPiece.pieceType != PieceType::None && capturedPiece.color == nextMover) || // Capturing own piece.
			(movingPieceType != PieceType::Pawn && move.promotedPiece != PieceType::None) // Promoting non-pawn piece
		) {
			return false;
		}

		// Move semantics
		auto isEnPassant = false;
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

				// ?? King moves in the direction of the attack.
				// ?? Say rook on e1 attacks king on f1. King can't move to g1.
				break;
			case PieceType::Pawn:
				auto startRank = nextMover == Color::White ? 1 : 6;
				auto backRank = nextMover == Color::White ? 7 : 0;
				// Move in wrong direction
				if ((move.sourceRank < move.destinationRank) != (nextMover == Color::White)) return false;
				// Back rank, but no promotion
				if (move.destinationRank == backRank && move.promotedPiece == PieceType::None) return false;
				// Not back rank, but promotion
				if (move.destinationRank != backRank && move.promotedPiece != PieceType::None) return false;
				// Move 1 square
				if (fileDiff == 0 && rankDiff == 1 &&
					pieces[move.destinationRank][move.destinationFile].pieceType == PieceType::None) break;
				// Move 2 squares from starting square
				if (fileDiff == 0 && rankDiff == 2 && move.sourceRank == startRank &&
					pieces[move.destinationRank][move.destinationFile].pieceType == PieceType::None) break;
				// Standard capture
				if (fileDiff == 1 && rankDiff == 1 &&
				    pieces[move.destinationRank][move.destinationFile].pieceType != PieceType::None &&
					pieces[move.destinationRank][move.destinationFile].color != nextMover) break;
				// Enpassant capture
				if (fileDiff == 1 && rankDiff == 1 && enPassantSquare.has_value() &&
				    move.destinationRank == enPassantSquare.value().rank &&
				    move.destinationFile == enPassantSquare.value().file) {
						isEnPassant = true;
						break;
				}
				return false;
		}

		Position sourcePosition {move.sourceRank, move.sourceFile};
		Position destinationPosition {move.destinationRank, move.destinationFile};
		// Not jumping over pieces (unless knight)
		if (movingPieceType != PieceType::Knight && !isClearBetween(this, sourcePosition, destinationPosition))
			return false;

		// Some piece that was pinned to the king moves off that
		// file, rank, or diagonal.
		auto kingPos = kingPosition[(int)nextMover];
		if (movingPieceType != PieceType::King && isPinnedToKing(sourcePosition, nextMover)) {
			if (!inDirection(kingPos, destinationPosition)) return false;
			auto [ sourceDir, sourceDist ] = getDirectionAndDistance(kingPos, sourcePosition);
			auto [ destDir, destDist ] = getDirectionAndDistance(kingPos, destinationPosition);
			if (sourceDir != destDir) return false;
		}

		// This is a very special case. En passant is the only move that can remove
		// two pieces from a line. So, we can have issues even if the pawn is not pinned.
		if (isEnPassant && kingPos.rank == sourcePosition.rank) {
			auto dir = kingPos.file < sourcePosition.file ? 1 : -1;
			for (int fDiff = 1; fDiff <= 7; fDiff++) {
				auto f = kingPos.file + dir * fDiff;
				if (f < 0 || f > 7) break;
				auto piece = pieces[kingPos.rank][f];
				if (piece.pieceType == PieceType::None) continue; // Skip empty squares
				if (f == sourcePosition.file || f == destinationPosition.file) continue; // Skip en-passant stuff
				if (piece.color == nextMover) break; // Found piece of same color.
				if (piece.pieceType == PieceType::Rook || piece.pieceType == PieceType::Queen) return false;
				break; // Found piece of other color, but not rook or queen
			}
		}

		// Your king is in check and you don't fix it.
		if (isUnderCheck(nextMover) && !isValidResponseToCheck(move)) return false;

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
		if (!isClearBetween(this, {move.sourceRank, rookFile}, {move.sourceRank, move.sourceFile}))
			return false;

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

	bool CBoard::isValidResponseToCheck(Move move) const {
		auto otherColor = oppositeColor(nextMover);
		auto movingPieceType = pieces[move.sourceRank][move.sourceFile].pieceType;
		auto kingPos = kingPosition[(int)nextMover];
		auto numChecks = std::popcount(attackedByKnight[(int)otherColor][kingPos.rank][kingPos.file]) +
			std::popcount(attackedBy[(int)otherColor][kingPos.rank][kingPos.file]);
		Position sourcePosition = { move.sourceRank, move.sourceFile };
		Position destinationPosition = { move.destinationRank, move.destinationFile };
		auto [moveDir, moveDist] = getDirectionAndDistance(kingPos, destinationPosition);

		if (movingPieceType == PieceType::King) { // King move
			if (attackedBy[(int)otherColor][move.destinationRank][move.destinationFile]) return false;
			if (attackedByKnight[(int)otherColor][move.destinationRank][move.destinationFile]) return false;

			for (auto attackingPosition: getAllAttackingPositions(kingPos, otherColor)) {
				auto attackingPiece = pieces[attackingPosition.rank][attackingPosition.file];
				if (attackingPiece.pieceType == PieceType::Knight) continue;
				auto [checkDir, checkDist] = getDirectionAndDistance(attackingPosition, kingPos);
				if (checkDir == moveDir && attackingPiece.pieceType != PieceType::Pawn) return false;
				if (checkDir == moveDir && move.promotedPiece != PieceType::None) return false;
			}
			return true;
		}

		// If it is a double check, then the king has to move. 
		if (numChecks > 1) return false;

		auto attackingPosition = getAttackingPosition(kingPos, otherColor);
		auto attackingPiece = pieces[attackingPosition.rank][attackingPosition.file];

		// Also, moving piece cannot be pinned.
		if (isPinnedToKing(sourcePosition, nextMover)) {
			// Knight can't move even when under pin.
			if (movingPieceType == PieceType::Knight) return false;

			// Everything else can move in the same or opposite direction of the pin.
			auto [direction, dist] = getDirectionAndDistance(kingPos, sourcePosition);
			if (moveDir != direction && moveDir != oppositeDirection(direction)) {
				return false;
			}
		}

		// Some other piece moved.
		// Capture
		if (attackingPosition.rank == move.destinationRank &&
			attackingPosition.file == move.destinationFile) {
			return true;
		} else if (
			enPassantSquare.has_value() && destinationPosition == enPassantSquare.value() &&
			attackingPosition.file == enPassantSquare.value().file &&
			attackingPosition.rank == (nextMover == Color::White ? 4 : 3)
			&& movingPieceType == PieceType::Pawn) {
			return true;
		}

		// Blocks
		// Can't block knight checks.
		if (attackingPiece.pieceType != PieceType::Knight && inDirection(kingPos, destinationPosition)) {
			auto [blockDir, blockDist] = getDirectionAndDistance(kingPos, destinationPosition);
			auto [attackDir, attackDist] = getDirectionAndDistance(kingPos, attackingPosition);
			if (blockDir == attackDir && blockDist <= attackDist) {
				// debug << "\tMoving piece blocks check." << std::endl;
				return true;
			}
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
		space_fail("Not under attack.");
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

	bool CBoard::hasLegalMoveFrom(Position fromPos, Color color) const {
		space_assert(!isUnderCheck(color), "Currently we are not handling checks.");

		auto fRank = fromPos.rank, fFile = fromPos.file;
		auto piece = pieces[fRank][fFile];
		if (piece.pieceType == PieceType::None || piece.color != color) return false;

		// For other pieces, 
		// 1. We first find a move.
		// 2. Check if the piece is pinned to the king.
		// 3. If so, do additional stuff. 
		auto baseRank = color == Color::White ? 0 : 7;
		auto& directions =
			(piece.pieceType == PieceType::Queen)  ? allDirections : (
			(piece.pieceType == PieceType::Rook)   ? cardinalDirections : (
			(piece.pieceType == PieceType::Bishop) ? diagonalDirections : (
			(piece.pieceType == PieceType::King)   ? allDirections : (
			noDirections))));
		auto pawnDir = color == Color::White ? 1 : -1;
		switch (piece.pieceType) {
			case PieceType::Knight:
				for (auto kDir : knightDirections) {
					auto offset = knightDirectionToOffset(kDir);
					auto dRank = fRank + offset.first;
					auto dFile = fFile + offset.second;
					if (dRank < 0 || dRank > 7 || dFile < 0 || dFile > 7) continue;
					if (pieces[dRank][dFile].pieceType != PieceType::None &&
						pieces[dRank][dFile].color == color) continue;
					if (isValidMove({fRank, fFile, dRank, dFile})) return true;
				}
			case PieceType::King:
				if (fRank == baseRank && fFile == 4 &&
				    pieces[baseRank][4].pieceType == PieceType::King
				 && pieces[baseRank][4].color == color) {
					// Castling
					if (isValidMove({fRank, fFile, fRank, fFile + 2})
					 || isValidMove({fRank, fFile, fRank, fFile - 2})) {
						 return true;
					 }
				 }
			case PieceType::Queen:
			case PieceType::Bishop:
			case PieceType::Rook:
				// We can get away with checking only 1 step moves because we aren't
				// under check.
				for (auto dir : directions) {
					auto offset = directionToOffset(dir);
					auto dRank = fRank + offset.first;
					auto dFile = fFile + offset.second;
					if (isValidMove({ fRank, fFile, dRank, dFile })) {
						// debug << "\tFound move to " << (char)('a' + dFile) << dRank << std::endl;
						return true;
					}
				}
				break;
			case PieceType::Pawn:
				// debug << "Checking pawn moves from: " << (char)(fFile + 'a') << (fRank + 1) << std::endl;
				// Regular moves and promotions
				if (isValidMove({ fRank, fFile, fRank + pawnDir, fFile })) return true;
				if (isValidMove({ fRank, fFile, fRank + pawnDir, fFile, PieceType::Queen })) return true;
				// Two step moves
				if (fRank == baseRank && isValidMove({ fRank, fFile, fRank + 2 * pawnDir, fFile })) return true;
				// Captures and enpassant captures
				for (auto fDiff : { 1, -1 }) {
					if (isValidMove({ fRank, fFile, fRank + pawnDir, fFile + fDiff })) return true;
				}
				break;
			default: space_assert(false, "Unknown piece type");
		}

		return false;
	}

	bool CBoard::isStaleMate() const {
		// debug << "Checking stale mate for " << (nextMover == Color::White ? "White" : "Black") << std::endl;
		if (isUnderCheck(nextMover)) return false;
		// debug << "Is not under check." << std::endl;

		for (int r = 0; r <= 7; r++) {
			for (int f = 0; f <= 7; f++) {
				auto piece = pieces[r][f];
				if (piece.pieceType == PieceType::None || piece.color != nextMover) continue;

				if (hasLegalMoveFrom({r, f}, nextMover)) {
					// debug << "Have move from " << (char)(f + 'a') << (r + 1) << std::endl;
					return false;
				}
			}
		}
		return true;
	}

	std::vector<Move> CBoard::getValidMoveList() const {
		auto backRank = nextMover == Color::White ? 7 : 0;
		std::vector<Move> result;
		for (auto sr = 0; sr <= 7; sr++)
		for (auto sf = 0; sf <= 7; sf++) {
			auto pieceType = pieces[sr][sf].pieceType;
			if (pieceType == PieceType::None || pieces[sr][sf].color != nextMover) continue;

			if (isLongPiece(pieceType)) {
				auto directions = getMoveDirections(pieceType);
				for (auto dir : directions) {
					auto offset = directionToOffset(dir);
					for (int i = 1; i <= 7; i++) {
						auto dr = sr + i * offset.first;
						auto df = sf + i * offset.second;
						if (dr < 0 || dr > 7 || df < 0 || df > 7) break;
						auto m = Move(sr, sf, dr, df);
						if (isValidMove(m)) result.push_back(m);
						if (pieces[dr][df].pieceType != PieceType::None) break;
					}
				}
			}
			else if (pieceType == PieceType::Knight) {
				for (auto kDir : knightDirections) {
					auto offset = knightDirectionToOffset(kDir);
					auto dr = sr + offset.first;
					auto df = sf + offset.second;
					if (dr < 0 || dr > 7 || df < 0 || df > 7) continue;
					auto m = Move(sr, sf, dr, df);
					if (isValidMove(m)) result.push_back(m);
				}
			}
			else if (pieceType == PieceType::King) {
				for (auto dir : allDirections) {
					auto offset = directionToOffset(dir);
					auto dr = sr + offset.first;
					auto df = sf + offset.second;
					if (dr < 0 || dr > 7 || df < 0 || df > 7) continue;
					auto m = Move(sr, sf, dr, df);
					if (isValidMove(m)) result.push_back(m);
				}
				auto baseRank = nextMover == Color::White ? 0 : 7;
				if (sr == baseRank && sf == 4) {
					auto m = Move(sr, sf, sr, sf + 2);
					if (isValidMove(m)) result.push_back(m);
					m = Move(sr, sf, sr, sf - 2);
					if (isValidMove(m)) result.push_back(m);
				}
			}
			else if (pieceType == PieceType::Pawn) {
				// 1 step, 2 step, capture, promotion.
				auto dir = nextMover == Color::White ? 1 : -1;
				auto backRank = nextMover == Color::White ? 7 : 0;
				for (auto m : {
					Move(sr, sf, sr + dir, sf),      // Std 1 step move 
					Move(sr, sf, sr + 2 * dir, sf),  // 2 steps
					Move(sr, sf, sr + dir, sf + 1),  // Capture left
					Move(sr, sf, sr + dir, sf - 1),  // Capture right
				}) {
					if (m.destinationRank == backRank) {
						for (auto pt : { PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight }) {
							m.promotedPiece = pt;
							if (isValidMove(m)) result.push_back(m);
						}
					}
					else {
						if (isValidMove(m)) result.push_back(m);
					}
				}
			}
		}
		return result;
	}

	std::string CBoard::getValidMoveString() const {
		auto result = getValidMoveList();

		std::vector<std::string> resultStr;
		for (auto m : result) {
			std::stringstream ss;
			auto sr = m.sourceRank;
			auto sf = m.sourceFile;
			auto dr = m.destinationRank;
			auto df = m.destinationFile;
			ss << (char)(sf + 'a') << (sr+1);
			ss << (char)(df + 'a') << (dr+1);
			switch (m.promotedPiece) {
				case PieceType::Queen: ss << 'q'; break;
				case PieceType::Rook: ss << 'r'; break;
				case PieceType::Bishop: ss << 'b'; break;
				case PieceType::Knight: ss << 'n'; break;
			}
			resultStr.push_back(ss.str());
		}

		sort(resultStr.begin(), resultStr.end());

		std::stringstream ss;
		for (auto s : resultStr) ss << s << " ";

		return ss.str();
	}
}
