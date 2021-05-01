#include "test_positions.h"

#include <chess/fen.h>
#include <chess/board_impl.h>

#include <sstream>


namespace space {


	TestPositionCPtr startingPosition()
	{
		auto initializeStartingPosition = []() {
			return TestPositionCPtr(new TestPosition{
				"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
				{{Move{1, 0, 2, 0}, "rnbqkbnr/pppppppp/8/8/8/P7/1PPPPPPP/RNBQKBNR b KQkq - 1 1"},
				 {Move{1, 0, 3, 0}, "rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR b KQkq a3 1 1"},
				 {Move{1, 1, 2, 1}, "rnbqkbnr/pppppppp/8/8/8/1P6/P1PPPPPP/RNBQKBNR b KQkq - 1 1"},
				 {Move{1, 1, 3, 1}, "rnbqkbnr/pppppppp/8/8/1P6/8/P1PPPPPP/RNBQKBNR b KQkq b3 1 1"},
				 {Move{1, 2, 2, 2}, "rnbqkbnr/pppppppp/8/8/8/2P5/PP1PPPPP/RNBQKBNR b KQkq - 1 1"},
				 {Move{1, 2, 3, 2}, "rnbqkbnr/pppppppp/8/8/2P5/8/PP1PPPPP/RNBQKBNR b KQkq c3 1 1"},
				 {Move{1, 3, 2, 3}, "rnbqkbnr/pppppppp/8/8/8/3P4/PPP1PPPP/RNBQKBNR b KQkq - 1 1"},
				 {Move{1, 3, 3, 3}, "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 1 1"},
				 {Move{1, 4, 2, 4}, "rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR b KQkq - 1 1"},
				 {Move{1, 4, 3, 4}, "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 1 1"},
				 {Move{1, 5, 2, 5}, "rnbqkbnr/pppppppp/8/8/8/5P2/PPPPP1PP/RNBQKBNR b KQkq - 1 1"},
				 {Move{1, 5, 3, 5}, "rnbqkbnr/pppppppp/8/8/5P2/8/PPPPP1PP/RNBQKBNR b KQkq f3 1 1"},
				 {Move{1, 6, 2, 6}, "rnbqkbnr/pppppppp/8/8/8/6P1/PPPPPP1P/RNBQKBNR b KQkq - 1 1"},
				 {Move{1, 6, 3, 6}, "rnbqkbnr/pppppppp/8/8/6P1/8/PPPPPP1P/RNBQKBNR b KQkq g3 1 1"},
				 {Move{1, 7, 2, 7}, "rnbqkbnr/pppppppp/8/8/8/7P/PPPPPPP1/RNBQKBNR b KQkq - 1 1"},
				 {Move{1, 7, 3, 7}, "rnbqkbnr/pppppppp/8/8/7P/8/PPPPPPP1/RNBQKBNR b KQkq h3 1 1"},
				 {Move{0, 1, 2, 0}, "rnbqkbnr/pppppppp/8/8/8/N7/PPPPPPPP/R1BQKBNR b KQkq - 1 1"},
				 {Move{0, 1, 2, 2}, "rnbqkbnr/pppppppp/8/8/8/2N5/PPPPPPPP/R1BQKBNR b KQkq - 1 1"},
				 {Move{0, 6, 2, 5}, "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1"},
				 {Move{0, 6, 2, 7}, "rnbqkbnr/pppppppp/8/8/8/7N/PPPPPPPP/RNBQKB1R b KQkq - 1 1"}
				}
			
			});
		};

		static TestPositionCPtr result = initializeStartingPosition();
		return result;
	}


	TestPositionCPtr queenPosition1()
	{
		auto initializeQueenPosition1 = []() {
			return 
				TestPositionCPtr(new TestPosition{
					"k7/8/8/6Q1/8/8/8/K7 w - - 0 1",
					{{Move{0, 0, 0, 1}, "k7/8/8/6Q1/8/8/8/1K6 b - - 1 1"},
					 {Move{0, 0, 1, 1}, "k7/8/8/6Q1/8/8/1K6/8 b - - 1 1"},
					 {Move{0, 0, 1, 0}, "k7/8/8/6Q1/8/8/K7/8 b - - 1 1"},

					 // move along file
					 {Move{4, 6, 5, 6}, "k7/8/6Q1/8/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 6, 6}, "k7/6Q1/8/8/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 7, 6}, "k5Q1/8/8/8/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 3, 6}, "k7/8/8/8/6Q1/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 2, 6}, "k7/8/8/8/8/6Q1/8/K7 b - - 1 1"},
					 {Move{4, 6, 1, 6}, "k7/8/8/8/8/8/6Q1/K7 b - - 1 1"},
					 {Move{4, 6, 0, 6}, "k7/8/8/8/8/8/8/K5Q1 b - - 1 1"},

					 // move along rank
					 {Move{4, 6, 4, 0}, "k7/8/8/Q7/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 4, 1}, "k7/8/8/1Q6/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 4, 2}, "k7/8/8/2Q5/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 4, 3}, "k7/8/8/3Q4/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 4, 4}, "k7/8/8/4Q3/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 4, 5}, "k7/8/8/5Q2/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 4, 7}, "k7/8/8/7Q/8/8/8/K7 b - - 1 1"},

					 // move along a1h8 diagonal
					 {Move{4, 6, 0, 2}, "k7/8/8/8/8/8/8/K1Q5 b - - 1 1"},
					 {Move{4, 6, 1, 3}, "k7/8/8/8/8/8/3Q4/K7 b - - 1 1"},
					 {Move{4, 6, 2, 4}, "k7/8/8/8/8/4Q3/8/K7 b - - 1 1"},
					 {Move{4, 6, 3, 5}, "k7/8/8/8/5Q2/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 5, 7}, "k7/8/7Q/8/8/8/8/K7 b - - 1 1"},


					 // move along a8h1 diagonal
					 {Move{4, 6, 3, 7}, "k7/8/8/8/7Q/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 5, 5}, "k7/8/5Q2/8/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 6, 4}, "k7/4Q3/8/8/8/8/8/K7 b - - 1 1"},
					 {Move{4, 6, 7, 3}, "k2Q4/8/8/8/8/8/8/K7 b - - 1 1"}
					 
					}
				});
		};
		static TestPositionCPtr result = initializeQueenPosition1();
		return result;
	}



	std::vector<TestPositionCPtr> getAllTestPositions()
	{
		return {
			startingPosition(),
			queenPosition1()
		};
	}

	std::string printAllMoves(const std::string& position)
	{
		auto board = BoardImpl::fromFen(position);
		auto moveMap = board->getValidMoves();
		std::stringstream ss;
		ss << "{\"" << position << "\",\n";
		ss  << "  {\n";
		for (const auto & mxb: moveMap)
		{
			const auto & m = mxb.first;
			const auto & b = mxb.second;
			ss << "    {Move{" << m.sourceRank << ", " 
				               << m.sourceFile << ", " 
							   << m.destinationRank << ", "
							   << m.destinationFile << "}, "
					<< '"' << Fen::fromBoard(b, 1, 1).fen << "\"},\n";
		}
		ss << "  }\n}";
		return ss.str();
	}


} // end namespace space
