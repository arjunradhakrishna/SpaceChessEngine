
#include "algo_dumbo_impl.h"
#include <chess/board_impl.h>
#include <limits>


namespace algo_dumbo_impl {

	void exploreStates(
			StateScores & stateScores,
			const StateSet& stateSet,
			int curDepth,
			space::Color whoPlaysNext,
			const space::AlgoDumboConfig & config)
	{
		// termination of recursion
		if (curDepth >= config.maxDepth || stateSet.empty())
		{
			forEachState(
					stateSet,
					[&config, curDepth](StateHandle stateHandle)
					{
						double score = computeBasicScore(getState(stateHandle), config);
						setScore(stateHandle, curDepth, score);

					});
			return;
		}


		// collect states for next level
		StateSet nextLevel;
		forEachState(
				stateSet,
				[&stateScores, curDepth, &config, whoPlaysNext, &nextLevel] (StateHandle stateHandle)
				{
					const auto & state = getState(stateHandle);
					auto board = stateToBoard(state);
					if (getNumUniqueStates(stateScores) > config.maxNumStates)
					{
						double score = computeBasicScore(getState(stateHandle), config);
						setScore(stateHandle, curDepth, score);
					}
					else if (board->isCheckMate())
					{
						double score = config.maxScore * getScoreFactorForColor(whoPlaysNext);
						setScore(stateHandle, curDepth, score);
					}
					else if (board->isStaleMate())
					{
						setScore(stateHandle, curDepth, 0);
					}
					else
					{
						auto validMoves = board->getValidMoves();
						for (const auto & mxb: validMoves)
						{
							const auto & board = mxb.second;
							auto state = boardToState(*board);
							addState(stateScores, nextLevel, state, config.maxDepth);
						}
					}
				});


		// recurse into next level
		space::Color opponent = (whoPlaysNext == space::Color::Black ? space::Color::White : space::Color::Black);
		exploreStates(stateScores, nextLevel, curDepth + 1, opponent, config);



		// compute best score for each state
		Comparator secondScoreBetterForMe = getComparatorForColor(whoPlaysNext);
		forEachState(
				stateSet,
				[&stateScores, curDepth, secondScoreBetterForMe](StateHandle stateHandle){
					if (!std::isnan(getScore(stateHandle, curDepth)))
						return;
					const auto & state = getState(stateHandle);
					auto board = stateToBoard(state);
					auto validMoves = board->getValidMoves();
					double bestNextScore = NAN;
					for (const auto & mxb: validMoves)
					{
						const auto & nextBoard = mxb.second;
						auto nextState = boardToState(*nextBoard);
						double nextScore = getScore(stateScores, nextState, curDepth + 1);
						if (std::isnan(bestNextScore) || secondScoreBetterForMe(bestNextScore, nextScore))
							bestNextScore = nextScore;
					}
					setScore(stateHandle, curDepth, bestNextScore);
				});
		
	}

	double computeBasicScore(
			const State& state,
			const space::AlgoDumboConfig& config)
	{
		auto board = stateToBoard(state);
		double myScoreFactor = getScoreFactorForColor(board->whoPlaysNext());
		double oppScoreFactor = myScoreFactor * -1;

		// stalemate is a draw
		if (board->isStaleMate())
			return 0;

		// checkmate is maxScore
		if (board->isCheckMate())
			// if i'm under check mate then i lost
			return config.maxScore * oppScoreFactor;


		// init resulting total score to 0
		double totalScore = 0;

		// score per each valid move of current player
		auto validMoves = board->getValidMoves();
		totalScore += validMoves.size() * config.validMoveScore * myScoreFactor;

		// get score for valid moves of opponent in next state
		// averaged across all moves i can make right now
		for (const auto & move_x_board : validMoves)
		{
			const auto & nextBoard = *move_x_board.second;
			if (nextBoard.isCheckMate())
				// i have a move that forces checkmate, i win
				return config.maxScore * myScoreFactor;

			totalScore +=
				config.validMoveScore               // score per valid move
				* nextBoard.getValidMoves().size()  // number of valid moves for that state
				* oppScoreFactor                    // score factor for opponent
				/ validMoves.size();                // average out across all possible next states
		}


		// TODO: compute score for protecting/threatening pieces		


		// score for pieces
		for (int rank = 0; rank < 8; ++rank)
		{
			for (int file = 0; file < 8; ++file)
			{
				space::Position pos(rank, file);
				auto optPiece = board->getPiece(pos);
				if (optPiece)
				{
					double scoreFactor = getScoreFactorForColor(optPiece->color);
					switch (optPiece->pieceType)
					{
						case space::PieceType::Pawn:
							totalScore += scoreFactor * config.pawnScore;
							break;
						case space::PieceType::Rook:
							totalScore += scoreFactor * config.rookScore;
							break;
						case space::PieceType::Knight:
							totalScore += scoreFactor * config.knightScore;
							break;
						case space::PieceType::Bishop:
							totalScore += scoreFactor * config.bishopScore;
							break;
						case space::PieceType::Queen:
							totalScore += scoreFactor * config.queenScore;
							break;
						default:
							break;
					}
				}
			}
		}
		return totalScore;
	}






	//-----------------------------------------------------------------------
	// state to board
	inline bool getBool(const State& state, int& idx)
	{
		return state[idx++];
	}
	inline int getInteger(const State& state, int& idx, int numBits)
	{
		int result = 0;
		while(numBits-- > 0)
		{
			result = result * 2 + (getBool(state, idx) ? 1 : 0);
		}
		return result;
	}
	inline space::PieceType getPieceType(const State& state, int& idx)
	{
		return static_cast<space::PieceType>(getInteger(state, idx, 3));
	}
	inline space::Color getColor(const State& state, int& idx)
	{
		return (getBool(state, idx) ? space::Color::Black : space::Color::White);
	}
	space::IBoard::Ptr stateToBoard(const State& state)
	{
		std::array<std::array<space::Piece, 8>, 8> pieces;
		int si = 0;
		for (int rank = 0; rank < 8; ++rank)
			for (int file = 0; file < 8; ++file)
			{
				auto pieceType = getPieceType(state, si);
				auto color = getColor(state, si);
				pieces[rank][file] = space::Piece{ pieceType, color };
			}
		bool canWhiteCastleLeft = getBool(state, si);
		bool canWhiteCastleRight = getBool(state, si);
		bool canBlackCastleLeft = getBool(state, si);
		bool canBlackCastleRight = getBool(state, si);
		space::Color whoPlaysNext = getColor(state, si);
		return std::make_shared<space::BoardImpl>(
				pieces,
				canWhiteCastleLeft,
				canWhiteCastleRight,
				canBlackCastleLeft,
				canBlackCastleRight,
				whoPlaysNext);
	}





    //-----------------------------------------------------------------------
	// board to state
	void setBool(State& state, int& si, bool b)
	{
		state[si++] = b;
	}
	void setInteger(State& state, int& si, int numBits, int integer)
	{
		int reverseInteger = 0;
		for (int i = 0; i < numBits; ++i)
		{
			reverseInteger *= 2;
			reverseInteger += (integer % 2);
			integer /= 2;
		}
		while(numBits--)
		{
			setBool(state, si, (reverseInteger % 2 == 1));
			reverseInteger /= 2;
		}
	}
	void setPieceType(State& state, int& si, space::PieceType pieceType)
	{
		setInteger(state, si, 3, static_cast<int>(pieceType));
	}
	void setColor(State& state, int & si, space::Color color)
	{
		setBool(state, si, (color == space::Color::Black));
	}
	State boardToState(const space::IBoard& board)
	{
		State state;
		int si = 0;
		for (int rank = 0; rank < 8; ++rank)
			for (int file = 0; file < 8; ++file)
			{
				auto optPiece = board.getPiece(space::Position(rank, file));
				space::PieceType pieceType = (optPiece ? optPiece->pieceType : space::PieceType::None);
				space::Color color = (optPiece ? optPiece->color : space::Color::Black);
				setPieceType(state, si, pieceType);
				setColor(state, si, color);
			}
		setBool(state, si, board.canCastleLeft(space::Color::White));
		setBool(state, si, board.canCastleRight(space::Color::White));
		setBool(state, si, board.canCastleLeft(space::Color::Black));
		setBool(state, si, board.canCastleRight(space::Color::Black));
		setColor(state, si, board.whoPlaysNext());
		return state;
	}







	//-----------------------------------------------------------------------------------------
	//state utilities
	void addState(StateScores& stateScores, const State& state, int maxDepth)
	{
		if (!stateScores.count(state))
			stateScores[state] = std::vector(maxDepth, std::numeric_limits<double>::quiet_NaN());
	}
	void setScore(StateScores& stateScores, const State& state, int depth, double score)
	{
		stateScores[state][depth] = score;
	}
	double getScore(const StateScores& stateScores, const State& state, int depth)
	{
		return stateScores.find(state)->second.operator[](depth);
	}
	void addState(StateScores& stateScores, StateSet& stateSet, const State& state, int maxDepth)
	{
		auto handle = stateScores.find(state);
		if (handle == stateScores.end())
			handle = stateScores.insert(std::make_pair(state, std::vector(maxDepth, std::numeric_limits<double>::quiet_NaN()))).first;
		stateSet.insert(handle);
	}
	void addState(StateSet& stateSet, const StateHandle& stateHandle)
	{
		stateSet.insert(stateHandle);
	}
	void setScore(StateHandle stateHandle, int curDepth, double score)
	{
		stateHandle->second.operator[](curDepth) = score;
	}
	double getScore(StateHandle stateHandle, int curDepth)
	{
		return stateHandle->second.operator[](curDepth);
	}

} // end namespace algo_dumbo_impl

































