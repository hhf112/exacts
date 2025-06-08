#include <iterator>
#include <optional>
#include <vector>
#include <iostream>


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


    std::vector<size_t> res1;
    auto strt = high_resolution_clock::now();
    bm.find(filepath,
            pattern,
            res1.begin());
    auto en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);
    std::cout << "classical search function find: " << en.count() << " ms.\n";
    std::cout << "found: " << res1.size() << '\n';

    std::vector<size_t> res2;
    strt = high_resolution_clock::now();
    bm.pfind(filepath,
             pattern,
             res2.begin());
    en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);

    std::cout << "parallel search function pfind: " << en.count() << " ms.\n";
    std::cout << "found: " << res2.size() << '\n';

    std::vector<size_t> res3;
    strt = high_resolution_clock::now();
    bm.pfind_unique( filepath,
                    pattern,
                    res3.begin());
    en = duration_cast<milliseconds>(high_resolution_clock::now() - strt);

    std::cout << "parallel search function pfind_unique: " << en.count()
        << " ms.\n";
    std::cout << "found: " << res3.size() << '\n';

    return 0;
}
