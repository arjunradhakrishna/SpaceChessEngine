#include <gtest/gtest.h>

#include "test_positions.h"

#include <chess/board_impl.h>

#include <algo_linear/algo_dumbo.h>
#include <algo_linear/algo_dumbo_impl.h>


	// TODO: implement all tests
	// testStateOperations();
	// testStateHandleCompare();
	// testStateScoreOperations();
	// testComparatorForColor();
	// testExploreStates();
	// testComputeBasicScore();
	// testConfigParsing();


TEST(AlgoDumboSuite, StateCompareTest)
{
	using namespace algo_dumbo_impl;
	State a, b;
	StateCompare c;
	auto equals = [&a, &b, &c]{ return !c(a, b) && !c(b, a); };
	auto lt = [&a, &b, &c]{ return c(a, b) && !c(b, a); };
	auto gt = [&a, &b, &c]{ return !c(a, b) && c(b, a); };
	ASSERT_TRUE(equals()) << "Default states should be equal.";

	a[0] = true;
	ASSERT_TRUE(gt()) << "True at pos 0 should be greater than false.";

	b[1] = true;
	ASSERT_TRUE(lt()) << "True at pos 1 should be greater than pos 0.";

	a[1] = b[0] = true;
	ASSERT_TRUE(equals()) << "True values should be compared correctly.";

	a[a.size() - 1] = true;
	ASSERT_TRUE(gt()) << "Last position is also compared.";
}




TEST(AlgoDumboSuite, StateBoardConversionTest)
{
	auto testPositions = space::getAllTestPositions();
	for (auto tp: testPositions)
	{
		auto board = space::BoardImpl::fromFen(tp->position);
		auto state = algo_dumbo_impl::boardToState(*board);
		auto board2 = algo_dumbo_impl::stateToBoard(state);
		ASSERT_EQ(board->canCastleLeft(space::Color::White), board2->canCastleLeft(space::Color::White));
		ASSERT_EQ(board->canCastleLeft(space::Color::Black), board2->canCastleLeft(space::Color::Black));
		ASSERT_EQ(board->canCastleRight(space::Color::White), board2->canCastleRight(space::Color::White));
		ASSERT_EQ(board->canCastleRight(space::Color::Black), board2->canCastleRight(space::Color::Black));
		ASSERT_EQ(board->isStaleMate(), board2->isStaleMate());
		ASSERT_EQ(board->isCheckMate(), board2->isCheckMate());
		ASSERT_EQ(board->whoPlaysNext(), board2->whoPlaysNext());
		for (int rank = 0; rank < 7; ++rank)
		{
			for (int file = 0; file < 7; ++file)
			{
				space::Position pos(rank, file);
				auto leftPiece = board->getPiece(pos);
				auto rightPiece = board2->getPiece(pos);
				ASSERT_EQ(leftPiece.has_value(), rightPiece.has_value());
				if (leftPiece.has_value())
				{
					ASSERT_EQ(leftPiece.value().pieceType, rightPiece.value().pieceType);
					ASSERT_EQ(leftPiece.value().color, rightPiece.value().color);
				}
			}
		}
	}
}
