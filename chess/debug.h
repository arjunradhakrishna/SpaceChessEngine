#pragma once

#include <iostream>
#include <sstream>

namespace space {
	struct NullStream : public std::stringstream {
		NullStream(): std::stringstream() {}
	};

	template<typename T>
	NullStream& operator<<(NullStream& ns, const T&) { return ns; }

	static auto& debug = std::cout;
	// static auto debug = NullStream();
}