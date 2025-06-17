#include <iostream>
#include <iterator>
#include <thread>
#include <vector>

#include "BoyreMoore.h"

using namespace std;

#define BADCHARS 256


using namespace std::chrono;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "wrong usage\n \t try: search <filename> <pattern> \n";
    return 1;
  }

  BoyreMoore bm(BADCHARS);
  std::string pattern(argv[2]);
  std::string filepath(argv[1]);

  std::vector<size_t> res1, res2;

  auto strt = high_resolution_clock::now();
  bm.find(filepath, pattern, std::back_inserter(res1), 1000000);
  auto en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);
  std::cout << "classical search function find: " << en.count() << " ms.\n";
  std::cout << "found: " << res1.size() << '\n';

  bm.set_search_count(0);

  strt = high_resolution_clock::now();
  bm.pfind(filepath, pattern, std::back_inserter(res2), 1000000);
  en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);
  std::cout << "parallel search function pfind: " << en.count() << " ms.\n";
  std::cout << "found: " << res2.size() << '\n';


  return 0;
}
