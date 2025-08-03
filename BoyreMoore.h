#pragma once
#include <atomic>      //  std::atomic
#include <cstring>     //  std::memcpy
#include <fstream>     //  std::fstream
#include <functional>  //  std::function
#include <iostream>    //  std::cerr
#include <iterator>    //  std::back_inserter
#include <optional>    //  std::optional
#include <string>      //  std::string
#include <thread>      //  std::thread
#include <vector>      //  std::vector

namespace hhf112 {
// GOLBAL typedefs
using index_t = std::ptrdiff_t;

class PatStreamer {
 public:
  static constexpr int MB = 1048576;
  static constexpr int MAX_CHUNK_LIMIT = 200;
  static constexpr int END_STREAM = 1;
  static constexpr int CONT_STREAM = 0;

  inline void endStream() { m_file.close(); }
  inline int getChunkSize() { return m_chunk_size; }
  inline int startStream(const std::string &p) {
    m_path = p;
    m_chunk_size = std::min(m_chunk_size, (size_t)MAX_CHUNK_LIMIT);
    m_file = std::fstream(m_path, std::ios::in);

    if (!m_file) {
      std::cerr << "startStream: unable to open file\n";
      return 1;
    }
    try {
      m_buffer.resize(m_chunk_size);
    } catch (std::length_error) {
      std::cerr << "startStream: unable to allocate buffer.\n";
      return 1;
    }
    return 0;
  }

  inline void forStream(size_t patternlen,
                        const std::function<int(const std::string &)> &action) {
    if (patternlen == 0) {
      std::cerr << "forStream: null pattern length.";
      return;
    }

    m_buffer.resize(m_chunk_size + patternlen - 1);

    if (!m_file.read(m_buffer.data(), m_chunk_size + patternlen - 1)) {
      std::cerr << "forStream: failed to read " << m_chunk_size + patternlen - 1
                << " bytes from m_file\n";
      return;
    }

    while (m_file.gcount()) {
      if (action(m_buffer) == 1) break;
      std::memcpy(m_buffer.data(), m_buffer.data() + m_chunk_size,
                  patternlen - 1);
      m_file.read(m_buffer.data() + patternlen - 1, m_chunk_size);
    }
  }

 private:
  std::fstream m_file;
  std::string m_path;
  std::string m_buffer;
  size_t m_chunk_size = 2 * MB;
};

struct PreProced {
  std::vector<size_t> shift, bpos;
  std::vector<index_t> badchars;
  int NCHARS;

  PreProced() = default;
  PreProced(int nchars, size_t patternlen) : NCHARS{nchars} {
    shift.resize(patternlen + 1);
    bpos.resize(patternlen + 1);
    badchars.resize(nchars, -1);
  }
};

class PreProcFact {
 public:
  PreProcFact() = default;

  inline const PreProced &getPreProced(int nchars, const std::string &str) {
    if (m_record.count(str)) return m_record[str];

    int n = str.length();
    PreProced data(nchars, str.length());
    badCharHeuristic(data.badchars, str, n);
    preprocess_strong_suffix(data.shift, data.bpos, str, n);
    preprocess_case2(data.shift, data.bpos, str, n);

    auto [newData, _] = m_record.emplace(str, std::move(data));
    return newData->second;
  }

 private:
  std::unordered_map<std::string, PreProced> m_record;
  inline void preprocess_strong_suffix(std::vector<size_t> &shift,
                                       std::vector<size_t> &bpos,
                                       const std::string &pat, size_t m) {
    size_t i, j;
    j = bpos[0];
    for (i = 0; i <= m; i++) {
      if (shift[i] == m) shift[i] = j;
      if (i == j) j = bpos[j];
    }
  }

  inline void badCharHeuristic(std::vector<index_t> &badhcars,
                               const std::string &str, size_t size) {
    size_t i;
    for (i = 0; i < size; i++) badhcars[(int)str[i]] = i;
  }

  inline void preprocess_case2(std::vector<size_t> &shift,
                               std::vector<size_t> &bpos,
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
};

class Bm {
 public:
  static constexpr int MAX_MATCHES = 10000000;
  static constexpr int CHECK_TICKER = 10000;

  // CONSTRUCTOR
  Bm() = default;

  // API
  /* @brief search_count is atomic */
  inline void set_search_count(size_t n) { m_search_count = n; }
  /* @brief by value */
  inline int get_search_count() { return m_search_count; }

  /* UNDER TESTING
  inline int pfind(
      const std::string &path, const std::string &pattern,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, int matches = MAX_MATCHES) {
    if (m_streamer.startStream(path) == 1) return {};

    size_t startIndex = 0;
    bool fail = 0;
    m_streamer.forStream(pattern.length(), [&](const std::string &buf) {
      int chk = parallelSearch(buf, pattern, startIndex, action, matches);
      if (chk < 0) {
        std::cerr << "pfind: parallelSearch failed.\n";
        return PatStreamer::END_STREAM;
      }

      if (m_search_count >= matches) {
        return PatStreamer::END_STREAM;
      }
      startIndex += m_streamer.getChunkSize() - pattern.length() + 1;
      return PatStreamer::CONT_STREAM;
    });

    if (fail) std::cerr << "search failed and was halted intermediately.\n";
    return m_search_count;
  }
  */

  /*
   * @brief run {action} for every occurance in each buffer
   * @params {action(it, en)}:
   *     @params {it} iterator to occurance
   *     @params {en} iterator to end of buffer
   */
  inline int find(
      const std::string &path, const std::string &pattern,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, int matches = MAX_MATCHES) {
    if (m_streamer.startStream(path) == 1) return {};

    size_t startIndex = 0;
    m_streamer.forStream(pattern.length(), [&](const std::string &buf) {
      search(buf, pattern, 0, buf.length(), action, nchars, startIndex,
             matches);
      if (m_search_count >= matches) return PatStreamer::END_STREAM;
      startIndex += m_streamer.getChunkSize() - pattern.length() + 1;

      return PatStreamer::CONT_STREAM;
    });

    m_streamer.endStream();
    return m_search_count;
  }

  /*
   * @brief run {action} for every occurance int {text}
   * @params {action(it, en)}:
   *     @params {it} iterator to occurance
   *     @params {en} iterator to end of {text}
   */
  inline int search(
      const std::string &text, const std::string &pat, size_t startPos,
      size_t endPos,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, size_t startIndex = 0, int matches = MAX_MATCHES) {
    const size_t patlen = pat.length();
    const size_t textlen = text.length();

    const PreProced &data = m_registry.getPreProced(nchars, pat);

    const index_t plen = static_cast<index_t>(patlen);
    const index_t startIdx = static_cast<index_t>(startIndex);

    index_t s = static_cast<index_t>(startPos);
    index_t en = static_cast<index_t>(endPos);
    index_t j, shift_gsfx, shift_bchr;

    int local_iter_count = 0;
    while (s <= en - plen) {
      if (local_iter_count % CHECK_TICKER == 0) {
        if (m_search_count >= matches) {
          return m_search_count;
        }
      }

      j = plen - 1;
      while (j >= 0 && pat[j] == text[s + j]) --j;

      if (j < 0) {
        action(text.begin() + s, text.end());
        ++m_search_count;

        shift_bchr = (s + plen < en) ? plen - data.badchars[text[s + patlen]]
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

  /* UNDER TESTING
  inline int parallelSearch(
      const std::string &text, const std::string &pattern, size_t startIndex,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, int matches = MAX_MATCHES) {
    const int concurrency = std::thread::hardware_concurrency();
    if (!concurrency) {
      std::cerr << "parallelSearch: No threads available.\n";
      return -1;
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
        search(text, pattern, startPos, endPos, action, nchars, matches);
      });
    }

    for (int i = 0; i<numThreads; i++) threads[i].join();
    return m_search_count;
  }
  */

 private:
  PreProcFact m_registry;
  PatStreamer m_streamer;
  std::atomic<size_t> m_search_count = 0;
};
}  // namespace hhf112
