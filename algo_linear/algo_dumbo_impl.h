#pragma once

#include "algo_dumbo.h"
#include <bitset>
#include <stdexcept>
#include <set>



namespace algo_dumbo_impl {

	const std::size_t StateSize = 64*4 + 5; // (board size 64) 
	                                        //    * (3 bits for piece type + 1 bit for color) 
	                                        // + (4 castling bits) + (1bit for who moves next);
	using State = std::bitset<StateSize>;
	struct StateCompare {
		bool operator()(const State& l, const State& r) const
		{
			for (int i = StateSize - 1; i >= 0; --i)
				if (l[i] ^ r[i]) return r[i];
			return false;
		}
	};
	space::IBoard::Ptr stateToBoard(const State& state);
	State boardToState(const space::IBoard& board);

	using StateScores = std::map<State, std::vector<double>, StateCompare>;
	using StateHandle = StateScores::iterator;

	inline std::size_t getNumUniqueStates(const StateScores& stateScores) { return stateScores.size(); }
	void addState(StateScores& stateScores, const State& state, int maxDepth);
	void setScore(StateScores& stateScores, const State& state, int depth, double score);
	double getScore(const StateScores& stateScores, const State& state, int depth);
	inline const State& getState(StateHandle stateHandle) { return stateHandle->first; }


	struct StateHandleCompare {
		bool operator()(StateHandle left, StateHandle right) const {
			return StateCompare()(left->first, right->first);
		}
	};
	using StateSet = std::set<StateHandle, StateHandleCompare>;
	void addState(StateScores& stateScores, StateSet& stateSet, const State& state, int maxDepth);
	void addState(StateSet& stateSet, const StateHandle& stateHandle);
	void setScore(StateHandle stateHandle, int curDepth, double score);
	double getScore(StateHandle stateHandle, int curDepth);
	template<typename TFunc> void forEachState(const StateSet& ss, TFunc tfunc)
	{
		for (auto stateHandle: ss)
			tfunc(stateHandle);
	}



	inline double getScoreFactorForColor(space::Color color) 
	{ 
		return (color == space::Color::White) 
			? 1.0 
			: -1.0;
	}

	typedef bool (*Comparator)(double, double);

	inline Comparator getComparatorForColor(space::Color color)
	{
		if (color == space::Color::White)
			return [](double l, double r) -> bool { return l < r; };
		else
			return [](double l, double r) -> bool { return l > r; };
	}
	

	void exploreStates(
			StateScores& stateScores,
			const StateSet& stateSet,
			int curDepth,
			space::Color whoPlaysNext,
			const space::AlgoDumboConfig& config);

	double computeBasicScore(
			const State& state,
			const space::AlgoDumboConfig& config);

	


} //end namespace algo_dumbo_impl




