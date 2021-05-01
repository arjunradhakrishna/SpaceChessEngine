#pragma once

#include <chess/board.h>

#include <memory>
#include <vector>
#include <string>

namespace space {


	struct TestPosition {
		std::string position;
		std::map<Move, std::string> moveMap;
	};
	typedef std::shared_ptr<const TestPosition> TestPositionCPtr;


	TestPositionCPtr startingPosition();
	TestPositionCPtr queenTest1();
	std::vector<TestPositionCPtr> getAllTestPositions();

	std::string printAllMoves(const std::string& position);


} // end namespace space
