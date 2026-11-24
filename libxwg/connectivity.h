// connectivity.h --- 単語の連結性を判定。
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"
#include <queue>

// 連結性を判定。
inline BOOL
check_connectivity(const std::unordered_set<MStringW>& words, MStringW& nonconnected) {
	if (words.size() <= 1)
		return TRUE;

	std::vector<MStringW> vec_words(words.begin(), words.end()); // 単語群。
	std::queue<size_t> queue; // 待ち行列。
	std::unordered_set<size_t> indexes;
	queue.emplace(0); // 待ち行列に初期の種を添える。。

	while (!queue.empty()) {
		const size_t index0 = queue.front();
		indexes.insert(index0);
		queue.pop(); // 種を取り除く。

		auto& w0 = vec_words[index0];
		for (size_t index1 = 0; index1 < vec_words.size(); ++index1) {
			if (index0 == index1)
				continue;

			auto& w1 = vec_words[index1];
			for (auto ch0 : w0) {
				for (auto ch1 : w1) {
					if (ch0 == ch1) {
						if (indexes.count(index1) == 0) {
							queue.emplace(index1);
							goto skip;
						}
					}
				}
			}
skip:;
		}
	}

	for (size_t i = 0; i < vec_words.size(); ++i) {
		if (indexes.count(i) == 0) {
			nonconnected = vec_words[i];
			return FALSE;
		}
	}

	return TRUE;
}
