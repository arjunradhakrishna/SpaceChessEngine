#include "test_positions.h"

#include <gtest/gtest.h>
#include <chess/board.h>
#include <chess/board_impl.h>
#include <chess/fen.h>
#include <algo_linear/algoLinear.h>
#include <algo_linear/algo_dumbo.h>

#include <sstream>

TEST(BoardSuite, StartingBoardTest) {
	using namespace space;
	IBoard::Ptr startingBoard = BoardImpl::getStartingBoard();
	auto startingFen = Fen::fromBoard(startingBoard, 0, 1);
	std::string expectedStartingFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	ASSERT_EQ(startingFen.fen, expectedStartingFen);
	ASSERT_EQ(Fen::fromBoard(BoardImpl::fromFen(startingFen), 0, 1).fen, expectedStartingFen);

}


TEST(BoardSuite, BoardBasicsTest) {
	using namespace space;
	ASSERT_EQ(2 + 2, 5 - 1);
	IBoard::Ptr startingBoard = BoardImpl::getStartingBoard();
	auto aa = startingBoard->getPiece({ 0,4 });
	ASSERT_EQ(aa.has_value(), true);
	ASSERT_EQ(aa.value().color, Color::White);
	ASSERT_EQ(aa.value().pieceType, PieceType::King);

	auto ab = startingBoard->getPiece({ 3,4 });
	ASSERT_EQ(ab.has_value(), false);

	auto ac = startingBoard->whoPlaysNext();
	ASSERT_EQ(ac, Color::White);

	bool ad = startingBoard->canCastleLeft(Color::White) &&
				startingBoard->canCastleLeft(Color::Black) &&
				startingBoard->canCastleRight(Color::White) &&
				startingBoard->canCastleRight(Color::Black);
	ASSERT_TRUE(ad);


}

TEST(BoardSuite, BoardUpdateTest) {
	using namespace space;
	IBoard::Ptr startingBoard = BoardImpl::getStartingBoard();

	auto ae = startingBoard->updateBoard({ 1,0,2,0 });
	ASSERT_TRUE(ae.has_value());

	std::string aefen = Fen::fromBoard(ae.value(), 0, 1).fen;
	ASSERT_NO_THROW(ae);
	std::string aefenExpected = "rnbqkbnr/pppppppp/8/8/8/P7/1PPPPPPP/RNBQKBNR b KQkq - 0 1";
	ASSERT_EQ(aefen, aefenExpected);



}

TEST(BoardSuite, BoardMovesTest) {
	using namespace space;
/*	BoardImpl::Ptr startingBoard = BoardImpl::getStartingBoard();
	auto& allMoves = startingBoard->getValidMoves();
	ASSERT_EQ(allMoves.size(), 20);

	auto ba = startingBoard->updateBoard({ 1,0,2,0 });
	ASSERT_TRUE(ba.has_value());

	auto bb = ba.value()->getValidMoves();
	ASSERT_EQ(bb.size(), 20);
*/
/*	std::string foolsMateFEN = "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR b KQkq - 2 3";
	auto bc = BoardImpl::fromFen(Fen::Fen(foolsMateFEN));
	auto bc2 = bc->isCheckMate();
	ASSERT_TRUE(!bc2);

	auto bc3 = bc->getValidMoves();
	std::vector<Position> moves;
	for (auto it = bc3.begin(); it != bc3.end(); it++) {
		Move m = it->first;
		if (m.sourceRank == 7 && m.sourceFile == 5)
			moves.push_back({ m.destinationRank, m.destinationFile });
	}
	ASSERT_GT(bc3.size(), 0);
*/
	std::string arabianFen = "7k/7R/4KN2/8/8/8/8/8 w - - 2 3";
	auto arabian = BoardImpl::fromFen(arabianFen);
	auto bd = arabian->getValidMoves();
	ASSERT_GT(bd.size(), 0);

	auto bd2 = Fen::moves2string(arabian, bd);

	ASSERT_GT(bd2.size(), 0);
//	Fen::moves2string(arabian, bd);



}

TEST(BoardSuite, TestMoveLibraries)
{
	auto testPositions = space::getAllTestPositions();
	for (auto tp: testPositions)
	{
		auto board = space::BoardImpl::fromFen(tp->position);
		auto validMoves = board->getValidMoves();
		const auto & expectedValidMoves = tp->moveMap;
		ASSERT_EQ(validMoves.size(), expectedValidMoves.size());
		for (auto & mxb: validMoves)
		{
			std::stringstream movess;
			movess << "{ "
				<< mxb.first.sourceRank << ", " 
				<< mxb.first.sourceFile << ", "
				<< mxb.first.destinationRank << ", "
				<< mxb.first.destinationFile << "}";
			auto evmIt = expectedValidMoves.find(mxb.first);
			ASSERT_TRUE(evmIt != expectedValidMoves.end()) 
				<< "Board presented unexpected move " << movess.str();
			const auto & expectedBoardFen = evmIt->second;
			auto boardFen = space::Fen::fromBoard(mxb.second, 1, 1).fen;
			ASSERT_EQ(boardFen, expectedBoardFen) 
				<< "Unexpected board via move " << movess.str();
		}
	}
}


TEST(AlgoSuite, AlgoLinearTest) {
	std::vector<double> wts01 = {1, 5, 4, 4, 10};
	using namespace space;

	Fen boardfen = Fen("8/8/2kq1r2/8/2KBNR2/8/8/8 b - - 0 0");
	auto b0 = BoardImpl::fromFen(boardfen);

    //auto aa = AlgoLinearDepthOne(wts01);
	auto aa = AlgoLinearDepthTwoExt(5, wts01);

	Move m0 = aa.getNextMove(b0);

	auto b1 = b0->updateBoard(m0);

	ASSERT_TRUE(b1.has_value());

	Move m1 = aa.getNextMove(b1.value());

	auto b2 = b1.value()->updateBoard(m1);

	ASSERT_TRUE(b2.has_value());


}
