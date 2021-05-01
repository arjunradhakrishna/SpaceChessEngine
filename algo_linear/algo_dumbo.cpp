#include "algo_dumbo.h"
#include "algo_dumbo_impl.h"
#include <chess/board.h>
#include <chess/board_impl.h>
#include <bitset>
#include <stdexcept>
#include <set>


namespace space {

	space::AlgoDumboConfig::AlgoDumboConfig() :
		maxDepth(5),
		maxNumStates(1000 * 1000),
		maxScore(10*1000),
		pawnScore(1),
		rookScore(5),
		knightScore(3),
		bishopScore(3),
		queenScore(8),
		validMoveScore(1)
	{ }

	space::AlgoDumboConfig::AlgoDumboConfig(const nlohmann::json& config)
	{
		throw std::runtime_error("AlgoDumboConfig::AlgoDumboConfig(nlohmann::json) not yet implemented");
	}


	Move AlgoDumbo::getNextMove(IBoard::Ptr board) {

		using namespace algo_dumbo_impl;

		if (board->isCheckMate())
			throw std::runtime_error("Cannot find next move for a board on check mate.");
		if (board->isStaleMate())
			throw std::runtime_error("Cannot find next move for a board on stale mate.");

		std::vector<std::pair<Move, State> > movesAndStates;
		StateScores stateScores;
		StateSet stateSet;
		
		for (const auto& moveXboard: board->getValidMoves()) {
			State state = boardToState(*moveXboard.second);
			movesAndStates.push_back(std::make_pair(moveXboard.first, state));
			addState(stateScores, stateSet, state, m_config.maxDepth);
		}

		if (movesAndStates.empty())
			throw std::runtime_error("Assertion error: if no checkmate and stalemate, then there should have been valid moves.");

		exploreStates(
				stateScores,
				stateSet,
				0,
				board->whoPlaysNext(),
				m_config);

		double bestScore = getScore(stateScores, movesAndStates.front().second, 0);
		Move bestMove = movesAndStates.front().first;
		Comparator scoreIsLess = getComparatorForColor(board->whoPlaysNext());
		for (const auto & moveAndState: movesAndStates)
		{
			double score = getScore(stateScores, moveAndState.second, 0);
			if (scoreIsLess(bestScore, score))
			{
				bestScore = score;
				bestMove = moveAndState.first;
			}
		}
		
		return bestMove;

	}

	AlgoDumbo::AlgoDumbo() {
		throw std::runtime_error("AlgoDumbo::AlgoDumbo not yet implemented");
	}

	AlgoDumbo::AlgoDumbo(const nlohmann::json& config) {
		throw std::runtime_error("AlgoDumbo::AlgoDumbo(json) not yet implemented");
	}


} // end namespace space






