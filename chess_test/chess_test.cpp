#include <algo_linear/algo_dumbo.h>
#include <algo_linear/algoLinear.h>
#include <chess/board.h>
#include <chess/board_impl.h>
#include <chess/counting_board.h>
#include <chess/debug.h>
#include <chess/fen.h>
#include <chess/pgn.h>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <set>
#include "test_positions.h"
#include "stalemates.h"


namespace test_utils {
	void validate_ply(space::Ply ply) {
		// Recreate the move and check.
		std::string move;
		if (ply.is_short_castle) {
			move = "O-O";
		}
		else if (ply.is_long_castle) {
			move = "O-O-O";
		}
		else {
			move = (ply.piece.pieceType == space::PieceType::Pawn) ? move : (move + ply.piece.as_char());
			move = move + ply.disambiguation;
			move = move + (ply.is_capture ? "x" : "");
			move = move + (char)('a' + ply.destination.file);
			move = move + (char)('1' + ply.destination.rank);
		}
		if (ply.is_promotion) {
			move = (move + "=") + ply.promotion_piece.as_char();
		}
		if (ply.is_checkmate) {
			move = move + "#";
		}
		else if (ply.is_check) {
			move = move + "+";
		}

		move = move + ply.annotation;
		ASSERT_EQ(move, ply.move);
	}

	void validate_game_moves(space::Game& game, bool is_stalemate) {
		std::shared_ptr<space::IBoard> board = std::move(space::CBoard::fromFen(game.starting_position));
		// space::debug << board->as_string(true, true) << std::endl;
		// space::debug << static_cast<space::CBoard*>(board.get())->attackString() << std::endl;

		for (auto ply : game.plies) {
			// std::cout << board->getValidMoveString() << std::endl;
			((space::CBoard*)board.get())->getValidMoveList();

			// space::debug << ply.move_number
			//           << (ply.color == space::Color::White ? ". " : "... ")
			//           << ply.move
			//           << std::endl;
			test_utils::validate_ply(ply);

			auto move = ply.to_move(board.get());
			ASSERT_TRUE(move.has_value());

			auto board_opt = board->updateBoard(move.value());
			ASSERT_TRUE(board_opt.has_value());
			board = board_opt.value();

			// space::debug << board->as_string(true, true) << std::endl;
			// space::debug << static_cast<space::CBoard*>(board.get())->attackString() << std::endl;

			auto opp_color = ply.color == space::Color::White
				? space::Color::Black
				: space::Color::White;
			auto board_in_check = board->isUnderCheck(opp_color);
			ASSERT_EQ(board_in_check, ply.is_check);

			auto board_in_checkmate = board->isCheckMate();
			auto move_is_checkmate = ply.is_checkmate;
			ASSERT_EQ(board_in_checkmate, ply.is_checkmate);
		}
		((space::CBoard*)board.get())->getValidMoveList();
		// std::cout << board->getValidMoveString() << std::endl;

		auto board_in_stalemate = board->isStaleMate();
		ASSERT_EQ(board_in_stalemate, is_stalemate);
	}
}

/*
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
*/

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

/*
	std::string arabianFen = "7k/7R/4KN2/8/8/8/8/8 w - - 2 3";
	auto arabian = BoardImpl::fromFen(arabianFen);
	auto bd = arabian->getValidMoves();
	ASSERT_GT(bd.size(), 0);

	auto bd2 = Fen::moves2string(arabian, bd);

	ASSERT_GT(bd2.size(), 0);
//	Fen::moves2string(arabian, bd);
*/

}

/*
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

TEST(BoardSuite, FenEnpassantablePawnTest) {
	using namespace space;

	auto fen = Fen("2rr2k1/p5pb/7p/1p5P/KPq3P1/8/8/8 w - b6 0 38");
	auto board = BoardImpl::fromFen(fen);
	ASSERT_TRUE(board->enPassantSquare.has_value());
	ASSERT_EQ(board->enPassantSquare.value().rank, 5);
	ASSERT_EQ(board->enPassantSquare.value().file, 1);

	fen = Fen("2rr2k1/p5pb/7p/1p5P/1Pp2qP1/K7/8/8 b - b3 0 1");
	board = BoardImpl::fromFen(fen);
	ASSERT_TRUE(board->enPassantSquare.has_value());
	ASSERT_EQ(board->enPassantSquare.value().rank, 2);
	ASSERT_EQ(board->enPassantSquare.value().file, 1);
}

TEST(BoardSuite, EnpassantablePawnCheckTest) {
	using namespace space;

	auto fen = Fen("2rr2k1/p5pb/7p/1p5P/KPq3P1/8/8/8 w - b6 0 38");
	auto board = BoardImpl::fromFen(fen);
	ASSERT_TRUE(board->enPassantSquare.has_value());
	ASSERT_EQ(board->enPassantSquare.value().rank, 5);
	ASSERT_EQ(board->enPassantSquare.value().file, 1);

	ASSERT_TRUE(board->isUnderCheck(Color::White));
}

TEST(BoardSuite, CastlingFlagUpdate) {
	using namespace space;

	// Capture on h8
	auto fen = Fen("3rk2r/1p2np2/pNp3q1/2PnQ1pp/1P6/P2P3P/4BPP1/1R3RK1 w k - 1 24");
	auto board = BoardImpl::fromFen(fen);
	auto Qxh8 = Move(4, 4, 7, 7);
	auto newBoard = board->updateBoard(Qxh8);
	ASSERT_TRUE(newBoard.has_value());
	ASSERT_FALSE(newBoard.value()->canCastleLeft(Color::Black));

	// Capture on a8
	fen = Fen("r3k2r/8/8/3Q4/8/8/8/R3K2R w KQkq - 0 1");
	board = BoardImpl::fromFen(fen);
	auto Qxa8 = Move(4, 3, 7, 0);
	newBoard = board->updateBoard(Qxa8);
	ASSERT_TRUE(newBoard.has_value());
	ASSERT_FALSE(newBoard.value()->canCastleRight(Color::Black));

	// Capture on a1
	fen = Fen("r3k2r/8/8/8/3q4/8/8/R3K2R b KQkq - 0 1");
	board = BoardImpl::fromFen(fen);
	auto Qxa1 = Move(3, 3, 0, 0);
	newBoard = board->updateBoard(Qxa1);
	ASSERT_TRUE(newBoard.has_value());
	ASSERT_FALSE(newBoard.value()->canCastleLeft(Color::White));

	// Capture on h1
	fen = Fen("r3k2r/8/8/8/4q3/8/8/R3K2R b KQkq - 0 1");
	board = BoardImpl::fromFen(fen);
	auto Qxh1 = Move(3, 4, 0, 7);
	newBoard = board->updateBoard(Qxh1);
	ASSERT_TRUE(newBoard.has_value());
	ASSERT_FALSE(newBoard.value()->canCastleRight(Color::White));
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

TEST(BoardSuite, PGNParseFromPositionTest) {
	using namespace space;

	auto f = std::fstream(
		"games/from_position.pgn",
		std::ios_base::in
	);
	auto game = PGN::parse(f);
	f.close();
	test_utils::validate_game_moves(*game);
}

TEST(CBoardSuite, Initialization) {
	using namespace space;

	auto board1 = CBoard::startPosition();
	debug << board1->as_string(true, true) << std::endl;
	debug << board1->attackString() << std::endl;

	debug << "------------" << std::endl;

	auto fen = Fen("1qrb4/r3Qp1k/p1R1p1pp/4P3/1P1PBP2/4P1B1/6PP/6K1 w - - 3 33");
	auto board2 = (CBoard&)*CBoard::fromFen(fen);
	debug << board2.as_string(true, true) << std::endl;
	debug << board2.attackString() << std::endl;
}
*/

TEST(BoardSuite, PGNParseTest) {
	using namespace space;

	auto f = std::fstream(
		"games/lichess_db_standard_rated_2013-01.pgn",
		std::ios_base::in
	);
	// auto games = PGN::parse_many(f, 1000);
	auto games = PGN::parse_all(f);
	f.close();

	for (auto& game : games) {
		// if (game.metadata["Site"] !=  "https://lichess.org/60fzkejt") continue;
		std::cout << game.metadata["Site"] << std::endl;
		auto is_stalemate =
			std::find(stalemates.begin(), stalemates.end(), game.metadata["Site"]) != stalemates.end();

		test_utils::validate_game_moves(game, is_stalemate);
	}
}