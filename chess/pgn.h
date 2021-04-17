#pragma once

#include "board.h"
#include "fen.h"
#include <iostream>

namespace space {
	struct Ply {
	public:
		std::string move;

		Position destination = Position(0, 0);

		Piece promotion_piece;

		Piece piece;

		Color color;

		int move_number;

		bool is_check;

		bool is_checkmate;

		bool is_capture;

		bool is_promotion;

		bool is_short_castle;

		bool is_long_castle;

		std::string disambiguation;

		std::string annotation;

		std::optional<Move> to_move(IBoard* board) const;

	private:
		std::optional<Position> try_source_offsets(
			IBoard* board,
			std::vector<std::vector<std::pair<int, int>>> offset_lists
		) const;
		bool satisfies_disambiguation(Position source_position) const;
	};

	class Game {
	public:
		Fen starting_position;

		std::vector<Ply> plies;

		std::map<std::string, std::string> metadata;

		Game(Fen _fen, std::vector<Ply> _plies, std::map<std::string, std::string> _metadata) :
			starting_position(_fen), plies(_plies), metadata(_metadata) {}
	};

	class PGN {
	public:
		static std::unique_ptr<Game> parse(std::istream& input);

		static std::vector<Game> parse_all(std::istream& input);

		static std::vector<Game> parse_many(std::istream& input, int limit);
	};
}
