#ifndef COMPILER_UTIL_H
#define COMPILER_UTIL_H

template <typename VarT, typename MatchT>
bool isAnyOf(VarT var, MatchT& match) {
	for (auto& i : match) {
		if (var == i) {
			return true;
		}
	}
	return false;
}

#endif