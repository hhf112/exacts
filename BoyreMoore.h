#pragma once
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#define MAX_MATCHES 1000000

using index_t = std::ptrdiff_t;

class BoyreMoore {
 private:
  //  No of characters to be considerd for bad character heuristic.
  const size_t nchars;

  // size per chunk of filestream in bytes.
  size_t chunkSize = 1 * 1024 * 1024;

  // filepath.
  std::string path;
  std::string buffer;
  std::fstream file;

  // No of shifts for every index preproccessed for Good suffix heuristic
  std::vector<size_t> shift;
  // border positions preprocessed for Good suffix heuristic
  std::vector<size_t> bpos;
  std::vector<index_t> badchar;

  // references for classical Boyre Moore search preprocessing:
  //   https://www.geeksforgeeks.org/boyer-moore-algorithm-good-suffix-heuristic/
  //   https://www.geeksforgeeks.org/boyer-moore-algorithm-for-pattern-searching/
  void badCharHeuristic(const std::string &str, size_t size);
  void preprocess_strong_suffix(const std::string &pat, size_t m);
  void preprocess_case2(const std::string &pat, size_t m);

  // > initializes file path.
  // > and sets chunk size to be fetched =  min (passed chunk size, 50 Mb(s)).
  // > opens file.
  int startStream(const std::string &p);

  //> reads chunks overlapping by patternlength to avoid search misses.
  //> runs action for each chunk.
  void forStream(size_t patternlen,
                 const std::function<int(const std::string &)> &action);

  const std::string &getBuf() { return buffer; }
  std::string getPath() { return path; }

 public:
  BoyreMoore(size_t nchars) : nchars{nchars} {
    try {
      badchar.resize(nchars, -1);
    } catch (std::length_error) {
      std::cerr << "BoyreMoore: unable to allcoate badhcars\n";
    };
  };

  // #find
  template <typename OutputItStart>
  std::optional<OutputItStart> find(const std::string &path,
                                    const std::string &pattern,
                                    OutputItStart beg,
                                    int matches = MAX_MATCHES) {
    if (startStream(path) == 1) return {};

    if (!badchar.size()) {
      std::cerr << "find: badchars not initialized\n";
      return {};
    }

    size_t startIndex = 0;
    int found = 0;
    forStream(pattern.length(), [&](const std::string &buf) {
      found += search(buf, pattern, 0, buf.length(), startIndex, beg,
                      matches - found);
      beg += found;
      if (found == matches) return 1;

      startIndex += chunkSize - pattern.length() + 1;
      return 0;
    });

    return beg;
  }

  // #pfind
  template <typename OutputItStart>
  std::optional<OutputItStart> pfind(const std::string &path,
                                     const std::string &pattern,
                                     OutputItStart beg,
                                     int matches = MAX_MATCHES) {
    if (!badchar.size()) {
      std::cerr << "pfind: badchars not initialized\n";
      return {};
    }
    if (startStream(path) == 1) return {};

    size_t startIndex = 0;
    int found = 0;
    forStream(pattern.length(), [&](const std::string &buf) {
      std::optional<OutputItStart> check =
          parallelSearch(buf, pattern, startIndex, beg, matches - found);
      if (check.has_value()) {
        found += std::distance(check.value(), beg);
        beg = check.value();
        if (found == matches) {
          return 1;
        }
        return 0;
      } else
        return 1;
    });

    return beg;
  }

  // #pfind_unique
  template <typename OutputItStart>
  std::optional<OutputItStart> pfind_unique(const std::string &path,
                                            const std::string &pattern,
                                            OutputItStart beg,
                                            int matches = MAX_MATCHES) {
    OutputItStart start = beg;
    std::optional<OutputItStart> check = pfind(path, pattern, beg, matches);
    if (!check.has_value())
      return {};
    else
      beg = check.value();

    std::sort(start, beg);
    auto en = std::unique(start, beg);
    return en;
  }

  // #search
  template <typename OutputItStart>
  int search(const std::string &text, const std::string &pat, size_t startPos,
             size_t endPos, size_t startIndex, OutputItStart beg,
             int matches = MAX_MATCHES) {
    const size_t patlen = pat.length();
    const size_t textlen = text.length();

    bpos.resize(patlen + 1), shift.resize(patlen + 1);

    badCharHeuristic(pat, patlen);
    preprocess_strong_suffix(pat, patlen);
    preprocess_case2(pat, patlen);

    index_t plen = static_cast<index_t>(patlen);
    index_t s = static_cast<index_t>(startPos);
    index_t j = static_cast<index_t>(j);
    index_t startIdx = static_cast<index_t>(startIndex);
    index_t en = static_cast<index_t>(endPos);

    int found = 0;
    while (s <= en - plen) {
      j = plen - 1;
      while (j >= 0 && pat[j] == text[s + j]) --j;

      if (j < 0) {
        *beg = static_cast<size_t>(s + startIndex);
        beg++;

        if (++found == matches) return found;

        s += std::max(static_cast<index_t>(shift[0]),
                      (s + plen < en ? plen - badchar[text[s + patlen]]
                                     : static_cast<index_t>(1)));
      } else
        s += std::max(
            static_cast<index_t>(shift[j + 1]),
            std::max(static_cast<index_t>(1), j - badchar[text[s + j]]));
    }
    return found;
  }

  // #parallelSearch
  template <typename OutputItStart>
  std::optional<OutputItStart> parallelSearch(const std::string &text,
                                              const std::string &pattern,
                                              size_t startIndex,
                                              OutputItStart beg,
                                              int matches = MAX_MATCHES) {
    const int concurrency = std::thread::hardware_concurrency();
    if (!concurrency) {
      std::cerr << "parallelSearch: No threads available on system.\n";
      return {};
    }

    const index_t txtlen = static_cast<index_t>(text.length());
    const index_t patlen = static_cast<index_t>(pattern.length());

    const int numThreads = concurrency + (txtlen % concurrency);

    std::vector<std::thread> threads(numThreads);
    std::vector<std::vector<size_t>> results(numThreads);

    const size_t part = txtlen / concurrency;
    int found = 0;

    for (int i = 0; i < numThreads; i++) {
      index_t startPos = i * part;
      index_t endPos = std::min((i + 1) * static_cast<index_t>(part) + patlen - 1, txtlen);

      threads[i] = std::thread([&, i]() {
        found += search(text,
                        pattern, 
                        startPos,
                        endPos,
                        startIndex,
                        beg,
                        matches - found);
      });
    }

    for (int i = 0; i < numThreads; i++) {
      threads[i].join();
      beg = std::copy(results[i].begin(), results[i].end(), beg);
    }

    return beg;
  }
};
