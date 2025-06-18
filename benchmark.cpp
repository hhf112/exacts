#include <climits>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <vector>

#include "BoyreMoore.h"

using namespace std;

#define BADCHARS 256

using namespace std::chrono;

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr
        << "wrong usage\n \t try: search <filename> <pattern> <max matches> \n";
    return 1;
  }

  std::string pattern(argv[2]);
  std::string filepath(argv[1]);
  long long int matches;
  try {
    matches = std::stoll(argv[3]);
  } catch (std::invalid_argument) {
    std::cerr << "invalid argument for <max matches>\n";
    return 1;
  }

  BoyreMoore bm(BADCHARS, pattern);
  std::vector<size_t> res1, res2;
  res1.reserve(matches), res2.reserve(matches);

  auto strt = high_resolution_clock::now();
  bm.find(filepath, pattern, std::back_inserter(res1), matches);
  auto en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);
  std::cout << "classical search function find: " << en.count() << " ms.\n";
  std::cout << "found: " << res1.size() << '\n';

  bm.set_search_count(0);

  strt = high_resolution_clock::now();
  bm.pfind(filepath, pattern, std::back_inserter(res2), matches);
  en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);
  std::cout << "parallel search function pfind: " << en.count() << " ms.\n";
  std::cout << "found: " << res2.size() << '\n';

  return 0;
}
