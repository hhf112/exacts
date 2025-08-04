#include <iostream>

#include "./single include/ExactS.h"
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

  hhf112::ExactS bm;

  auto strt = high_resolution_clock::now();
  int cnt1 = bm.find(
      filepath, pattern, [&](auto it, auto en) {},
      matches);
  auto en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);
  std::cout << "classical search function find: " << cnt1 << " in "<< en.count() << " ms.\n";

  bm.reset_search();
  strt = high_resolution_clock::now();
  int cnt2 = bm.pfind(
      filepath, pattern, [&](auto it, auto en) {},
      matches);
  en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);
  std::cout << "parallel search function pfind: " << cnt2 << " in " << en.count() << " ms.\n";

  return 0;
}
