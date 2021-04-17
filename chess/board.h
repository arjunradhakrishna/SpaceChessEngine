#pragma once

#include <memory>
#include <string>
#include <optional>
#include <map>

namespace space {
	enum class Color { White, Black };
	enum class PieceType { Pawn, EnPassantCapturablePawn, Rook, Knight, Bishop, Queen, King, None };
	struct Position {
		int rank;
		int file;
		Position(int v_rank, int v_file) : rank(v_rank), file(v_file) {}
		Position(std::string san) : Position(san[1] - '1', san[0] - 'a') { }
	};
	struct Move {
		int sourceRank;
		int sourceFile;
		int destinationRank;
		int destinationFile;
		PieceType promotedPiece; // for pawn Promotion only
		Move() {}
		Move(int v_sourceRank, int v_sourceFile, int v_destinationRank, int v_destinationFile, PieceType v_promotedPiece = PieceType::None) :
			sourceRank(v_sourceRank), sourceFile(v_sourceFile), destinationRank(v_destinationRank), destinationFile(v_destinationFile), promotedPiece(v_promotedPiece)
		{}
		bool operator <(const Move& that) const;
	};
	struct Piece {
		PieceType pieceType;
		Color color;
		char as_char() const;
	};
	class IBoard {
	public:
		using Ptr = std::shared_ptr<IBoard>;
		using MoveMap = std::map<Move, Ptr>;
		virtual Color whoPlaysNext() const = 0;
		virtual std::optional<Piece> getPiece(Position position) const = 0;
		
		// (Left and right from that player's point of view)
		virtual bool canCastleLeft(Color color) const = 0;
		virtual bool canCastleRight(Color color) const = 0;

		virtual bool isStaleMate() const = 0;
		virtual bool isCheckMate() const = 0;
		virtual bool isUnderCheck(
			Color color,
			std::optional<Position> targetKingPosition = std::nullopt
		) const = 0;
		virtual std::optional<Ptr> updateBoard(Move move) const = 0;
		virtual MoveMap getValidMoves() const = 0;
	};

}
