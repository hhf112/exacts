#include <iostream>
#include <vector>

#include "Moore.h"
using namespace std;

#define BADCHARS 256

using namespace std::chrono;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr
        << "wrong usage\n \t try: search <filename> <pattern>  \n";
    return 1;
  }

  std::string pattern(argv[2]), filepath(argv[1]);
  const long long int matches = 10000000;

  hhf112::Bm bm;
  std::vector<size_t> res1, res2;
  res1.reserve(matches), res2.reserve(matches);

  auto strt = high_resolution_clock::now();
  bm.find(
      filepath, pattern, [&](auto it, auto en) { res1.push_back(1); },
      matches);
  auto en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);
  std::cout << "classical search function find: " << en.count() << " ms.\n";
  std::cout << "found: " << res1.size() << '\n';

  bm.set_search_count_(0);
  strt = high_resolution_clock::now();
  int status = bm.pfind(
      filepath, pattern, [&](auto it, auto en) { res2.push_back(1); },
      matches);
  en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);
  std::cout << "parallel search function pfind: " << en.count() << " ms.\n";
  std::cout << "found: " << res2.size() << '\n';
  std::cout << "status: " << status << '\n';

  return 0;
}
