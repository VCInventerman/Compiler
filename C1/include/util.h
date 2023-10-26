#ifndef COMPILER_UTIL_H
#define COMPILER_UTIL_H

template <typename VarT, typename MatchT>
bool isAnyOf(VarT var, MatchT match) {
	for (auto& i : match) {
		if (var == i) {
			return true;
		}
	}
	return false;
}

template <typename VarT>
bool isAnyOf(VarT var, std::initializer_list<VarT> match) {
	for (auto& i : match) {
		if (var == i) {
			return true;
		}
	}
	return false;
}

template <typename VarT, typename MatchItrT>
bool isAnyOf(VarT var, MatchItrT begin, MatchItrT end) {
	for (MatchItrT i = begin; i < end; i++) {
		if (var == *i) {
			return true;
		}
	}
	return false;
}

#define FIND_ARRAY_SUBMEMBER(arr, val, member) (std::find_if(std::begin(arr), std::end(arr), [val](auto& i) { return i.member == val; }))
#define IS_ARRAY_SUBMEMBER_ANY_OF(arr, val, member) (std::find_if(std::begin(arr), std::end(arr), [val](auto& i) { return i.member == val; }) != std::end(arr))

template <typename IterableT, typename DataT>
int count(IterableT container, DataT target) {
	int cnt = 0;

	for (auto i = container.begin(); i != container.end(); i++) {
		if (*i == target) {
			cnt++;
		}
	}

	return cnt;
}

template <typename IterableT, typename DataT>
bool has(IterableT container, DataT target) {
	for (auto i = container.begin(); i != container.end(); i++) {
		if (*i == target) {
			return true;
		}
	}

	return false;
}

template <typename VarT, typename MatchT>
bool hasAnyOf(VarT container, MatchT match) {
	for (auto i = container.begin(); i != container.end(); i++) {
		if (isAnyOf(*i, match)) {
			return true;
		}
	}

	return false;
}

template <typename BufT, typename... ArgsT>
void appendAll(BufT& buf, ArgsT &&... args) {
	auto impl = [&](auto i) { buf << i; };

	(impl(args), ...);
}

static constexpr const std::array<uint32_t, 6> WHITESPACE = { ' ', '\t', '\n', '\v', '\f', '\r' };

template <typename ItrT>
ItrT skipWhiteSpace(ItrT readCursor, ItrT end) {
	while (readCursor != end && isAnyOf(*readCursor, WHITESPACE)) {
		readCursor++;
	}

	return readCursor;
}

inline void nop() {}

#endif