#pragma once

#include <vector>
#include <memory>
#include "board.h"

/*	
 * Here is the main idea behind the board. We maintain the usual structures:
 * a. Who moves?
 * b. Where are the pieces?
 * 
 * Additionally, to compute check, checkmate, and stalemate efficiently, we
 * maintain the following:
 * c. King positions for both kings
 * 
 * To make updating checks, checkmates, etc faster, we also store:
 * d. For each square and color, which directions (ortho and diagonal) is it
 *    under attack from?
 * e. We could also add this for knight directions.
 *    Based on memory considerations, we can eliminate e
 * 
 * Now, to determine checks, we just check if the king position is under attack.
 * For ortho and diagonal attacks, we have stored it in (d). For knight moves,
 * we explicitly check.
 * 
 * Checks are checkmates under two conditions:
 * 1. King can't move to any empty square around it
 * 2. No piece can capture the checking piece.
 *    -- the checking piece is under attack
 *    -- the capturing piece is not pinned
 * 3. No piece can block the check
 */

namespace space {
	enum Direction : unsigned char {
		North         =   1,
		South         =   2,
		East          =   4,
		West          =   8,
		NorthEast     =  16,
		NorthWest     =  32,
		SouthEast     =  64,
		SouthWest     = 128,
	};

	// Helper for iteration.
	static const std::vector<Direction> directions {
		Direction::North, Direction::South, Direction::East, Direction::West,
		Direction::NorthEast, Direction::NorthWest,
		Direction::SouthEast, Direction::SouthWest,
	};

	enum KnightDirection : unsigned char {
		Clock01    =   1,
		Clock02    =   2,
		Clock04    =   4,
		Clock05    =   8,
		Clock07    =  16,
		Clock08    =  32,
		Clock10    =  64,
		Clock11    = 128,
	};

	// Helper for iteration.
	static const std::vector<KnightDirection> knightDirections {
		KnightDirection::Clock01, KnightDirection::Clock02, 
		KnightDirection::Clock04, KnightDirection::Clock05, 
		KnightDirection::Clock07, KnightDirection::Clock08, 
		KnightDirection::Clock10, KnightDirection::Clock11, 
	};

	class CBoard : public IBoard {
	private: // All state
		static constexpr auto ShortCastle = 0;
		static constexpr auto LongCastle = 0;

		Color nextMover;

		std::array<std::array<Piece, 8>, 8> pieces;

		// Index: white (0) or black (1)
		std::array<Position, 2> kingPosition;

		// First index: white (0) or black (1)
		// Second index: short (0) or long (1)
		// Not that this is not left vs right. It is short vs long.
		std::array<std::array<bool, 2>, 2> castlingRights;

		// First index: white or black
		std::array<std::array<std::array<unsigned char, 8>, 8>, 2> attackedBy;
		std::array<std::array<std::array<unsigned char, 8>, 8>, 2> attackedByKnight;

	public:
		bool isStaleMate() const override {}
		bool isCheckMate() const override {}
		std::optional<Ptr> updateBoard(Move move) const override {}
		MoveMap getValidMoves() const override {}

		static std::unique_ptr<CBoard> startPosition();
		std::string attackString() const;

	private:
		std::unique_ptr<CBoard> clone() const;
		inline bool isUnderAttack(Position position, Color attackingColor) const;
		// Is capturable

	// Trivial functions
	public:
		inline Color whoPlaysNext() const override { return nextMover; }
		inline std::optional<Piece> getPiece(Position position) const override;
		inline bool isUnderCheck (
			Color color,
			std::optional<Position> targetKingPosition = std::nullopt
		) const override;
		inline bool canCastleLeft(Color color) const override;
		inline bool canCastleRight(Color color) const override;
	};
}