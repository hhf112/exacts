#pragma once
#include <atomic>      // for std::atomic
#include <cstring>     // for std::memcpy
#include <fstream>     // for std::fstream
#include <functional>  // for std::function
#include <iostream>    // for std::cerr
#include <iterator>    // for std::back_inserter
#include <optional>    // for std::optional
#include <string>      // for std::string
#include <thread>      // for std::thread
#include <vector>      // for std::vector

// HARD LIMITS
#define MAX_MATCHES 1000000
#define MAX_CHUNK_LIMIT (200 * MB)
#define CHECK_TICKER 10000

// standard sizes
#define MB 1048576

// FILE STREAM SIGNALS
#define END_STREAM 1
#define CONT_STREAM 0

// typedefs
using index_t = std::ptrdiff_t;

class BoyreMoore {
 public:
  /* members */
  struct PatternData {
    std::vector<size_t> shift;
    std::vector<size_t> bpos;
    std::vector<index_t> badchars;

    PatternData() = default;
    PatternData(int nchars, size_t patternlen) {
      shift.resize(patternlen + 1);
      bpos.resize(patternlen + 1);
      badchars.resize(nchars, -1);
    }
  };

  // A better implementation possibly a round robin hashmap
  std::unordered_map<std::string, PatternData> patternCache;

  BoyreMoore(int nchars) : m_nchars{nchars} {}
  BoyreMoore(int nchars, const std::string &pattern) : m_nchars{nchars} {
    preprocess_pattern(nchars, pattern);
  }

  /* getters and setters */
  inline void set_search_count(size_t n) { m_search_count = n; }
  inline int get_search_count() { return m_search_count; }
  inline void set_chunk_size(size_t n) { m_chunk_size = n; }

  inline std::string &getBuf() { return m_buffer; }
  inline const std::string &getBufconst() { return m_buffer; }
  inline std::string getPath() { return m_path; }

  /* functions */
  template <typename OutputItStart>
  inline std::optional<OutputItStart> find(const std::string &path,
                                    const std::string &pattern,
                                    OutputItStart beg,
                                    int matches = MAX_MATCHES) {
    if (startStream(path) == 1) return {};

    size_t startIndex = 0;
    forStream(pattern.length(), [&](const std::string &buf) {
      search(buf, pattern, 0, buf.length(), startIndex, beg, matches);
      if (m_search_count >= matches) return END_STREAM;
      startIndex += m_buffer.length() - pattern.length() + 1;
      return CONT_STREAM;
    });

    m_file.close();
    return beg;
  }

  template <typename OutputItStart>
  inline std::optional<OutputItStart> pfind(const std::string &path,
                                            const std::string &pattern,
                                            OutputItStart beg,
                                            int matches = MAX_MATCHES) {
    if (startStream(path) == 1) return {};

    size_t startIndex = 0;
    bool fail = 0;
    forStream(pattern.length(), [&](const std::string &buf) {
      std::optional<OutputItStart> check =
          parallelSearch(buf, pattern, startIndex, beg, matches);
      if (!check.has_value()) {
        std::cerr << "parallelSearch: failed to search/insert\n";
        fail = 1;
        return END_STREAM;
      }

      beg = check.value();
      if (m_search_count >= matches) {
        return END_STREAM;
      }
      startIndex += m_chunk_size - pattern.length() + 1;
      return CONT_STREAM;
    });

    if (fail) std::cerr << "search failed and was halted intermediately.\n";
    return beg;
  }

  template <typename OutputItStart>
  inline int search(const std::string &text, const std::string &pat,
                    size_t startPos, size_t endPos, size_t startIndex,
                    OutputItStart beg, int matches = MAX_MATCHES);

  template <typename OutputItStart>
  inline std::optional<OutputItStart> parallelSearch(const std::string &text,
                                                     const std::string &pattern,
                                                     size_t startIndex,
                                                     OutputItStart beg,
                                                     int matches = MAX_MATCHES);

  inline void preprocess_pattern(int nchars, const std::string &pattern) {
    if (patternCache.count(pattern)) return;

    int n = pattern.length();
    PatternData data(nchars, pattern.length());
    badCharHeuristic(data.badchars, pattern, n);
    preprocess_strong_suffix(data.shift, data.bpos, pattern, n);
    preprocess_case2(data.shift, data.bpos, pattern, n);

    patternCache.insert(make_pair(pattern, std::move(data)));
  }

 private:
  std::atomic<size_t> m_search_count = 0;

  const int m_nchars = 256;

  size_t m_chunk_size = 2 * MB;

  std::string m_path;
  std::string m_buffer;
  std::fstream m_file;

  // references for classical Boyre Moore search preprocessing:
  //   https://www.geeksforgeeks.org/boyer-moore-algorithm-good-suffix-heuristic/
  //   https://www.geeksforgeeks.org/boyer-moore-algorithm-for-pattern-searching/
  inline void badCharHeuristic(std::vector<index_t> badhcars,
                               const std::string &str, size_t size);
  inline void preprocess_strong_suffix(std::vector<size_t> shift,
                                       std::vector<size_t> bpos,
                                       const std::string &pat, size_t m);
  inline void preprocess_case2(std::vector<size_t> shift,
                               std::vector<size_t> bpos, const std::string &pat,
                               size_t m);

  int startStream(const std::string &p) {
    m_path = p;
    m_chunk_size = std::min(m_chunk_size, (size_t)MAX_CHUNK_LIMIT);
    m_file = std::fstream(m_path, std::ios::in);

    if (!m_file) {
      std::cerr << "startStream: unable to open file\n";
      return 1;
    }
    try {
      m_buffer.resize(m_chunk_size, 'a');
    } catch (std::length_error) {
      std::cerr << "startStream: unable to allocate buffer.\n";
      return 1;
    }
    return 0;
  }

  inline void forStream(size_t patternlen,
                 const std::function<int(const std::string &)> &action) {
    if (patternlen == 0) return;
    m_buffer.resize(m_chunk_size + patternlen - 1, 'a');

    if (!m_file.read(m_buffer.data(), m_chunk_size + patternlen - 1)) {
      std::cerr << "forStream: failed to read from m_file\n";
      return;
    }

    while (m_file.gcount()) {
      if (action(m_buffer) == 1) break;
      std::memcpy(m_buffer.data(), m_buffer.data() + m_chunk_size,
                  patternlen - 1);
      m_file.read(m_buffer.data() + patternlen - 1, m_chunk_size);
    }
  }
};

// Implementation details
//
// SEARCH
template <typename OutputItStart>
int BoyreMoore::search(const std::string &text, const std::string &pat,
                       size_t startPos, size_t endPos, size_t startIndex,
                       OutputItStart beg, int matches) {
  const size_t patlen = pat.length();
  const size_t textlen = text.length();

  preprocess_pattern(m_nchars, pat);
  const PatternData &data = patternCache[pat];

  const index_t plen = static_cast<index_t>(patlen);
  const index_t startIdx = static_cast<index_t>(startIndex);

  index_t s = static_cast<index_t>(startPos);
  index_t en = static_cast<index_t>(endPos);
  index_t j, shift_gsfx, shift_bchr;

  int local_iter_count = 0;
  while (s <= en - plen) {
    if (local_iter_count % CHECK_TICKER) {
      if (m_search_count >= matches) {
        return m_search_count;
      }
    }

    j = plen - 1;
    while (j >= 0 && pat[j] == text[s + j]) --j;

    if (j < 0) {
      *beg = static_cast<size_t>(s) + startIndex;
      ++m_search_count;

      shift_bchr = s + plen < en ? plen - data.badchars[text[s + patlen]]
                                 : static_cast<index_t>(1);
      shift_gsfx = static_cast<index_t>(data.shift[0]);
    } else {
      shift_gsfx = static_cast<index_t>(data.shift[j + 1]);
      shift_bchr =
          std::max(static_cast<index_t>(1), j - data.badchars[text[s + j]]);
    }

    s += std::max(shift_gsfx, shift_bchr);
    ++local_iter_count;
  }
  return m_search_count;
}

// PARALLEL_SEARCH
template <typename OutputItStart>
std::optional<OutputItStart> BoyreMoore::parallelSearch(
    const std::string &text, const std::string &pattern, size_t startIndex,
    OutputItStart beg, int matches) {
  const int concurrency = std::thread::hardware_concurrency();
  if (!concurrency) {
    std::cerr << "parallelSearch: No threads available.\n";
    return {};
  }

  const index_t txtlen = static_cast<index_t>(text.length());
  const index_t patlen = static_cast<index_t>(pattern.length());

  const int numThreads = concurrency + bool(txtlen % concurrency);

  std::vector<std::thread> threads(numThreads);
  std::vector<std::vector<size_t>> results(numThreads);

  const size_t part = txtlen / concurrency;
  const index_t overlap = patlen - 1;

  for (int i = 0; i < numThreads; i++) {
    index_t startPos = i * part;
    index_t endPos =
        std::min((i + 1) * static_cast<index_t>(part) + overlap, txtlen);

    threads[i] = std::thread([&, i]() {
      search(text, pattern, startPos, endPos, startIndex,
             std::back_inserter(results[i]), matches);
    });
  }

  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
    beg = std::copy(results[i].begin(), results[i].end(), beg);
  }

  return beg;
}

// PREPROCESSING
void BoyreMoore::preprocess_strong_suffix(std::vector<size_t> shift,
                                          std::vector<size_t> bpos,
                                          const std::string &pat, size_t m) {
  size_t i, j;
  j = bpos[0];
  for (i = 0; i <= m; i++) {
    if (shift[i] == m) shift[i] = j;
    if (i == j) j = bpos[j];
  }
}

void BoyreMoore::badCharHeuristic(std::vector<index_t> badhcars,
                                  const std::string &str, size_t size) {
  size_t i;
  for (i = 0; i < size; i++) badhcars[(int)str[i]] = i;
}

void BoyreMoore::preprocess_case2(std::vector<size_t> shift,
                                  std::vector<size_t> bpos,
                                  const std::string &pat, size_t m) {
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
