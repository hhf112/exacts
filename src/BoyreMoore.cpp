// reference:
//   https://www.geeksforgeeks.org/boyer-moore-algorithm-good-suffix-heuristic/
//   https://www.geeksforgeeks.org/boyer-moore-algorithm-for-pattern-searching/
#include "../BoyreMoore.h"

#include <string>
#include <vector>
using namespace std;

using index = std::ptrdiff_t;

void BoyreMoore::badCharHeuristic(const string &str, size_t size) {
  size_t i;
  for (i = 0; i < size; i++) badchar[(int)str[i]] = i;
}

void BoyreMoore::preprocess_strong_suffix(const string &pat, size_t m) {
  index_t i = static_cast<index_t>(m), j = static_cast<index_t>(m + 1);
  bpos[i] = j;
  while (i > 0) {
    while (j <= m && pat[i - 1] != pat[j - 1]) {
      if (shift[j] == m) shift[j] = j - i;
      j = bpos[j];
    }
    i--;
    j--;
    bpos[i] = j;
  }
}
void BoyreMoore::preprocess_case2(const string &pat, size_t m) {
  size_t i, j;
  j = bpos[0];
  for (i = 0; i <= m; i++) {
    if (shift[i] == m) shift[i] = j;
    if (i == j) j = bpos[j];
  }
}
