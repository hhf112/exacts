#pragma once
#include <atomic>      //  std::atomic
#include <climits>     // LLONG_MAX
#include <cstring>     //  std::memcpy
#include <filesystem>  //std::filesystem
#include <fstream>     //  std::fstream
#include <functional>  //  std::function
#include <iostream>    //  std::cerr
#include <string>      //  std::string
#include <thread>      //  std::thread
#include <vector>      //  std::vector

namespace hhf112 {
// GOLBAL typedefs
using arith_t = std::ptrdiff_t;

class Streamer {
 public:
  static constexpr int MB = 1048576;
  static constexpr int MAX_CHUNK_LIMIT = 200 * MB;
  static constexpr int END_STREAM = 1;
  static constexpr int CONT_STREAM = 0;

  inline void endStream() { file_.close(); }
  inline int get_chunk_size() { return chunk_size_; }
  inline void set_chunk_size(size_t n) { chunk_size_ = n; }

  /*
   * @brief set `file`, moderate passed chunk size
   */
  inline size_t startStream(const std::string &p) {
    path_ = p;
    chunk_size_ = std::min(chunk_size_, (size_t)MAX_CHUNK_LIMIT);
    file_ = std::fstream(path_, std::ios::in);

    if (!file_) {
      std::cerr << "startStream: unable to open file_\n";
      return -1;
    }
    return std::filesystem::file_size(path_);
  }

  /*
   * @brief run `{action}` for every chunk read. copy trailing `{overlap}`
   *length ofA each chunk to the front.
   *@detail first is read is chunk_size + overlap
   */
  inline int forStream(size_t overlap,
                       const std::function<int(const std::string &)> &action) {
    if (overlap > chunk_size_) return -1;
    try {
      buffer_.resize(chunk_size_ + overlap);
    } catch (std::bad_alloc &b) {
      std::cerr << "forStream: caught exception std::bad_alloc\n";
      return -1;
    } catch (std::length_error &l) {
      std::cerr << "forStream: caught exception std::length_error\n";
    }

    try {
      file_.read(buffer_.data(), chunk_size_ + overlap);
    } catch (std::ios_base::failure &f) {
      return -1;
    }

    while (file_.gcount()) {
      if (action(buffer_) == END_STREAM) break;
      std::memcpy(buffer_.data(), buffer_.data() + chunk_size_, overlap);
      try {
        file_.read(buffer_.data() + overlap, chunk_size_);
      } catch (std::ios_base::failure &f) {
        return -1;
      }
    }

    return 1;
  }

 private:
  std::fstream file_;
  std::string path_;
  std::string buffer_;
  size_t chunk_size_ = 2 * MB;
};

struct PreProced {
  std::vector<size_t> shift, bpos;
  std::vector<arith_t> badchars;
  int NCHARS;

  PreProced() = default;
  PreProced(int nchars, size_t patternlen) : NCHARS{nchars} {
    shift.resize(patternlen + 1);
    bpos.resize(patternlen + 1);
    badchars.resize(nchars, -1);
  }
};

class PreProcFactory {
 public:
  PreProcFactory() = default;

  /*@brief fetch from or create in preprocessed pattern cache*/
  inline const PreProced &getPreProced(int nchars, const std::string &str) {
    if (record_.count(str)) return record_[str];

    int n = str.length();
    PreProced data(nchars, str.length());
    badCharHeuristic(data.badchars, str, n);
    preprocess_strong_suffix(data.shift, data.bpos, str, n);
    preprocess_case2(data.shift, data.bpos, str, n);

    auto [newData, _] = record_.emplace(str, std::move(data));
    return newData->second;
  }

 private:
  /*@src
   * https://www.geeksforgeeks.org/dsa/boyer-moore-algorithm-for-pattern-searching*/
  inline void badCharHeuristic(std::vector<arith_t> &badhcars,
                               const std::string &str, size_t size) {
    size_t i;
    for (i = 0; i < size; i++) badhcars[(int)str[i]] = i;
  }

  /*@src
   * https://www.geeksforgeeks.org/dsa/boyer-moore-algorithm-good-suffix-heuristic*/
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

  /*@src
   * https://www.geeksforgeeks.org/dsa/boyer-moore-algorithm-good-suffix-heuristic*/
  inline void preprocess_case2(std::vector<size_t> &shift,
                               std::vector<size_t> &bpos,
                               const std::string &pat, size_t m) {
    arith_t i = static_cast<arith_t>(m), j = static_cast<arith_t>(m + 1);
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

  std::unordered_map<std::string, PreProced> record_;
};

class ExactS {
 public:
  static constexpr int MAX_MATCHES = 1000000;

  // CONSTRUCTOR
  ExactS() = default;

  // API

  /* @brief reset search count and flag to stop threads. */
  inline void reset_search() {
    search_count_.store(0);
    done_.store(false);
  }
  /* @brief by value */
  inline int get_search_count() { return search_count_.load(); }

  /*
   * @brief run `{action}` for every occurance in buffer
   * @params `{action(it, en)}`:
   *     @params `{it}` iterator to occurance
   *     @params `{en}` iterator to end of buffer
   * @note occurances may be random and repeated
   */
  inline int pfind(
      const std::string &path, const std::string &pattern,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, size_t matches = MAX_MATCHES) {
    if (pattern.length() > static_cast<size_t>(LLONG_MAX)) return -1;

    bool fail = 0;
    int status =
        streamer_.forStream(pattern.length() - 1, [&](const std::string &buf) {
          if (done_.load()) return Streamer::END_STREAM;
          if (parallelSearch(buf, pattern, action, nchars, matches) < 0) {
            fail = 1;
            return Streamer::END_STREAM;
          }
          return Streamer::CONT_STREAM;
        });

    if (fail || status == -1) return -1;
    return search_count_.load();
  }

  /*
   * @brief run `{action}` for every occurance in buffer
   * @params `{action(it, en)}`:
   *     @params `{it}` iterator to occurance
   *     @params `{en}` iterator to end of buffer
   * @note occurances may be repeated
   */
  inline int find(
      const std::string &path, const std::string &pattern,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, size_t matches = MAX_MATCHES) {
    if (pattern.length() > static_cast<size_t>(LLONG_MAX)) return -1;
    if (streamer_.startStream(path) == 1) return -1;

    int status =
        streamer_.forStream(pattern.length() - 1, [&](const std::string &buf) {
          if (done_.load()) return Streamer::END_STREAM;
          search(buf, pattern, 0, buf.length(), action, nchars, matches);
          return Streamer::CONT_STREAM;
        });

    if (!status) return -1;
    return search_count_.load();
  }

  /*
   * @brief run `{action}` for every occurance in `{text}`
   * @params `{action(it, en)}`:
   *     @params `{it}` iterator to occurance
   *     @params `{en}` iterator to end of `{text}`
   * @note occurances may be repeated
   */
  inline int parallelSearch(
      const std::string &text, const std::string &pattern,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, size_t matches = MAX_MATCHES) {
    if (pattern.length() > static_cast<size_t>(LLONG_MAX)) return -1;

    const int concurrency = std::thread::hardware_concurrency();
    if (!concurrency) return -1;
    const size_t ntext = text.length();
    const arith_t m = pattern.length();

    const int numThreads = concurrency + bool(ntext % concurrency);

    std::vector<std::thread> threads(numThreads);

    const size_t part = ntext / concurrency;
    const arith_t overlap = m - 1;
    int cur = 0;
    for (auto &thread : threads) {
      arith_t startPos = cur * part;
      arith_t endPos = std::min((cur + 1) * part + overlap, ntext);

      thread = std::thread([&, startPos, endPos]() {
        search(text, pattern, startPos, endPos, action, nchars, matches);
      });
      ++cur;
    }

    for (auto &thread : threads) thread.join();
    return search_count_.load();
  }

  /*
   * @brief run `{action}` for every occurance in `{text}`
   * @params `{action(it, en)}`:
   *     @params `{it}` iterator to occurance
   *     @params `{en}` iterator to end of `{text}`
   * @note occurances may be repeated
   */
  inline int search(
      const std::string &text, const std::string &pattern, size_t startPos,
      size_t endPos,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, size_t matches = MAX_MATCHES) {
    if (pattern.length() > static_cast<size_t>(LLONG_MAX)) return -1;

    const PreProced &data = registry_.getPreProced(nchars, pattern);
    const arith_t m = pattern.length();
    const size_t n = text.length();
    size_t s = startPos, en = endPos;
    arith_t j, shift_gsfx, shift_bchr;

    while (s <= en - m) {
      j = m - 1;
      while (j >= 0 && pattern[j] == text[s + j]) --j;

      if (j < 0) {
        action(text.begin() + s, text.end());
        if (search_count_.fetch_add(1, std::memory_order_relaxed) >= matches) {
          done_.store(true);
          return search_count_.load();
        }

        shift_bchr = (s + m < en) ? m - data.badchars[text[s + m]]
                                  : static_cast<arith_t>(1);
        shift_gsfx = data.shift[0];
      } else {
        shift_gsfx = data.shift[j + 1];
        shift_bchr =
            std::max(static_cast<arith_t>(1), j - data.badchars[text[s + j]]);
      }

      s += std::max(shift_gsfx, shift_bchr);
    }
    return search_count_.load();
  }

 private:
  PreProcFactory registry_;
  Streamer streamer_;
  std::atomic<size_t> search_count_{0};
  std::atomic<bool> done_{false};
};
}  // namespace hhf112
