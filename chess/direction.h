#pragma once

#include "common/base.h"

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

	static std::string directionToString(Direction dir) {
		switch (dir) {
			case Direction::North:     return "North";
			case Direction::South:     return "South";
			case Direction::East:      return "East";
			case Direction::West:      return "West";
			case Direction::NorthEast: return "NorthEast";
			case Direction::NorthWest: return "NorthWest";
			case Direction::SouthEast: return "SouthEast";
			case Direction::SouthWest: return "SouthWest";
		}
	}

	constexpr std::pair<int, int> directionToOffset(Direction direction) {
		switch (direction) {
			case Direction::North:     return {  1,  0 };
			case Direction::South:     return { -1,  0 };
			case Direction::East:      return {  0,  1 };
			case Direction::West:      return {  0, -1 };
			case Direction::NorthEast: return {  1,  1 };
			case Direction::NorthWest: return {  1, -1 };
			case Direction::SouthEast: return { -1,  1 };
			case Direction::SouthWest: return { -1, -1 };
		}
		space_assert(false, "Unknown direction.");
		return { 0, 0 };
	}

	constexpr Direction oppositeDirection(Direction direction) {
		switch (direction) {
			case Direction::North:     return Direction::South;
			case Direction::South:     return Direction::North;
			case Direction::East:      return Direction::West;
			case Direction::West:      return Direction::East;
			case Direction::NorthEast: return Direction::SouthWest;
			case Direction::NorthWest: return Direction::SouthEast;
			case Direction::SouthEast: return Direction::NorthWest;
			case Direction::SouthWest: return Direction::NorthEast;
		}
		space_assert(false, "Unknown knight direction.");
		return Direction::North;
	}

	// Helpers for iteration.
	static const std::vector<Direction> allDirections {
		Direction::North, Direction::South, Direction::East, Direction::West,
		Direction::NorthEast, Direction::NorthWest,
		Direction::SouthEast, Direction::SouthWest,
	};
	static const std::vector<Direction> cardinalDirections {
		Direction::North,
		Direction::South,
		Direction::East,
		Direction::West
	};
	static const std::vector<Direction> diagonalDirections {
		Direction::NorthEast,
		Direction::NorthWest,
		Direction::SouthEast,
		Direction::SouthWest,
	};
	static const std::vector<Direction> noDirections {};

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

	constexpr std::pair<int, int> knightDirectionToOffset(KnightDirection direction) {
		switch (direction) {
			case KnightDirection::Clock01: return {  2,  1 };
			case KnightDirection::Clock02: return {  1,  2 };
			case KnightDirection::Clock04: return { -1,  2 };
			case KnightDirection::Clock05: return { -2,  1 };
			case KnightDirection::Clock07: return { -2, -1 };
			case KnightDirection::Clock08: return { -1, -2 };
			case KnightDirection::Clock10: return {  1, -2 };
			case KnightDirection::Clock11: return {  2, -1 };
		}
		space_assert(false, "Unknown knight direction.");
		return { 0, 0 };
	}

	constexpr KnightDirection oppositeKnightDirection(KnightDirection direction) {
		switch (direction) {
			case KnightDirection::Clock01: return KnightDirection::Clock07;
			case KnightDirection::Clock02: return KnightDirection::Clock08;
			case KnightDirection::Clock04: return KnightDirection::Clock10;
			case KnightDirection::Clock05: return KnightDirection::Clock11;
			case KnightDirection::Clock07: return KnightDirection::Clock01;
			case KnightDirection::Clock08: return KnightDirection::Clock02;
			case KnightDirection::Clock10: return KnightDirection::Clock04;
			case KnightDirection::Clock11: return KnightDirection::Clock05;
		}
		space_assert(false, "Unknown knight direction.");
		return KnightDirection::Clock01;
	}
}