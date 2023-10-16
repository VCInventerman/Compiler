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

template <typename VarT, typename MatchItrT>
bool isAnyOf(VarT var, MatchItrT begin, MatchItrT end) {
	for (MatchItrT i = begin; i < end; i++) {
		if (var == *i) {
			return true;
		}
	}
	return false;
}

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

#endif