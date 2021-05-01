#pragma once

#include <vector>
#include <memory>
#include "board.h"
#include "direction.h"
#include "fen.h"

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
	class CBoard : public IBoard {
	private: // All state
		static constexpr auto ShortCastle = 0;
		static constexpr auto LongCastle = 1;

		Color nextMover;

		std::array<std::array<Piece, 8>, 8> pieces;

		// Index: white (0) or black (1)
		std::array<Position, 2> kingPosition;

		// First index: white (0) or black (1)
		// Second index: short (0) or long (1)
		// Not that this is not left vs right. It is short vs long.
		std::array<std::array<bool, 2>, 2> castlingRights;

		// First index: white or black
		std::array<std::array<std::array<unsigned char, 8>, 8>, 2> attackedBy { 0 };
		std::array<std::array<std::array<unsigned char, 8>, 8>, 2> attackedByKnight { 0 };

	public:
		// Game state
		bool isStaleMate() const override {}
		bool isCheckMate() const override {}

		// Moves and updates
		std::optional<Ptr> updateBoard(Move move) const override;
		MoveMap getValidMoves() const override {}
		bool isValidMove(Move move) const;
		bool isValidCastle(Move move) const;
		bool isValidResponseToCheck(Move move, bool checkForPins=true) const;

		// Initialization
		static Ptr fromFen(const Fen& fen);
		static std::unique_ptr<CBoard> startPosition();
		std::string attackString() const;

	private:
		std::unique_ptr<CBoard> clone() const;
		inline bool isUnderAttack(Position position, Color attackingColor) const;
		inline bool isUnderAttackFromDirection(Position position, Color attackingColor, Direction direction) const;
		Position getAttackingPosition(Position position, Color attackingColor) const;

		void updateUnderAttack(); // Recomputes fully.
		void updateUnderAttackFrom(Position position);

		// We are going to use these to make moves.
		void removePieceAt(Position position);
		void addPieceAt(Piece piece, Position position);

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
