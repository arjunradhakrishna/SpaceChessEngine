#include "base.h"

#include <stdexcept>

#include <algorithm>


namespace space {

	void space_assert(bool expr, std::string message) {
		if (!expr)
			throw std::runtime_error("Assertion Failure: " + message);
	}

	void space_fail(std::string message) {
        throw std::runtime_error("Runtime Failure: " + message);
    }

    template<typename KeyType, typename ValueType>
    KeyType get_best(const std::map<KeyType, ValueType>& x, bool high) {
        using pairtype = std::pair<KeyType, ValueType>;

        auto better = [high](const pairtype& p1, const pairtype& p2) {
            return (high ? p1.second < p2.second : p1.second > p1.second);
        };

        auto best = *std::max_element(x.begin(), x.end(), better);
        return best.first;
    }
}
