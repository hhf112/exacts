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

class Streamer {
 public:
  static constexpr int MB = 1048576;
  static constexpr int MAX_CHUNK_LIMIT = 200 * MB;
  static constexpr int END_STREAM = 1;
  static constexpr int CONT_STREAM = 0;

  inline void endStream() { file_.close(); }
  inline int getChunkSize() { return chunk_size_; }
  inline int startStream(const std::string &p) {
    path_ = p;
    chunk_size_ = std::min(chunk_size_, (size_t)MAX_CHUNK_LIMIT);
    file_ = std::fstream(path_, std::ios::in);

    if (!file_) {
      std::cerr << "startStream: unable to open file_\n";
      return 1;
    }
    try {
      buffer_.resize(chunk_size_);
    } catch (std::length_error) {
      std::cerr << "startStream: unable to allocate buffer_.\n";
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

    buffer_.resize(chunk_size_ + patternlen - 1);

    if (!file_.read(buffer_.data(), chunk_size_ + patternlen - 1)) {
      std::cerr << "forStream: failed to read " << chunk_size_ + patternlen - 1
                << " bytes from file_\n";
      return;
    }

    while (file_.gcount()) {
      if (action(buffer_) == 1) break;
      std::memcpy(buffer_.data(), buffer_.data() + chunk_size_, patternlen - 1);
      file_.read(buffer_.data() + patternlen - 1, chunk_size_);
    }
  }

 private:
  std::fstream file_;
  std::string path_;
  std::string buffer_;
  size_t chunk_size_ = 2 * MB;
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

class PreProcFactory {
 public:
  PreProcFactory() = default;

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
  std::unordered_map<std::string, PreProced> record_;
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
  static constexpr int MAX_MATCHES = 1000000;

  // CONSTRUCTOR
  Bm() = default;

  // API
  /* @brief search_count_ is atomic */
  inline void set_search_count_(size_t n) { search_count_.store(n); }
  /* @brief by value */
  inline int get_search_count() { return search_count_.load(); }

  /*
   * @brief run {action} for every occurance in buffer
   * @params {action(it, en)}:
   *     @params {it} iterator to occurance
   *     @params {en} iterator to end of buffer
   *
   * @note occurances may be random and repeated
   */
  inline int pfind(
      const std::string &path, const std::string &pattern,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, int matches = MAX_MATCHES) {
    if (streamer_.startStream(path) == 1) return -1;

    bool fail = 0;
    streamer_.forStream(pattern.length(), [&](const std::string &buf) {
      if (done_.load()) return Streamer::END_STREAM;
      if (parallelSearch(buf, pattern, action, nchars, matches) < 0) {
        fail = 1;
        return Streamer::END_STREAM;
      }
      return Streamer::CONT_STREAM;
    });

    if (fail) return -1;
    return search_count_.load();
  }

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
    if (streamer_.startStream(path) == 1) return -1;

    streamer_.forStream(pattern.length(), [&](const std::string &buf) {
      if (done_.load()) return Streamer::END_STREAM;
      search(buf, pattern, 0, buf.length(), action, nchars, matches);
      return Streamer::CONT_STREAM;
    });

    return search_count_;
  }

  /*
   * @brief run {action} for every occurance in {text}
   * @params {action(it, en)}:
   *     @params {it} iterator to occurance
   *     @params {en} iterator to end of {text}
   *
   * @note occurances may be random and repeated
   */
  inline int parallelSearch(
      const std::string &text, const std::string &pattern,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, int startIndex = 0, int matches = MAX_MATCHES) {
    const int concurrency = std::thread::hardware_concurrency();
    if (!concurrency) return -1;
    const index_t txtlen = static_cast<index_t>(text.length());
    const index_t patlen = static_cast<index_t>(pattern.length());

    const int numThreads = concurrency + bool(txtlen % concurrency);

    std::vector<std::thread> threads(numThreads);
    std::vector<std::vector<size_t>> results(numThreads);

    const size_t part = txtlen / concurrency;
    const index_t overlap = patlen - 1;

    int cur = 0;
    for (auto &thread : threads) {
      index_t startPos = cur * part;
      index_t endPos =
          std::min((cur + 1) * static_cast<index_t>(part) + overlap, txtlen);

      thread = std::thread([&, startPos, endPos]() {
        search(text, pattern, startPos, endPos, action, nchars, matches);
      });
      ++cur;
    }

    for (auto& thread: threads) thread.join();
    return search_count_;
  }

  /*
   * @brief run {action} for every occurance in {text}
   * @params {action(it, en)}:
   *     @params {it} iterator to occurance
   *     @params {en} iterator to end of {text}
   */
  inline int search(
      const std::string &text, const std::string &pat, size_t startPos,
      size_t endPos,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, int matches = MAX_MATCHES) {
    const PreProced &data = registry_.getPreProced(nchars, pat);

    const size_t patlen = pat.length();
    const size_t textlen = text.length();
    const index_t plen = static_cast<index_t>(patlen);
    index_t s = static_cast<index_t>(startPos);
    index_t en = static_cast<index_t>(endPos);
    index_t j, shift_gsfx, shift_bchr;

    while (s <= en - plen) {
      j = plen - 1;
      while (j >= 0 && pat[j] == text[s + j]) --j;

      if (j < 0) {
        action(text.begin() + s, text.end());
        if (search_count_.fetch_add(1, std::memory_order_relaxed) >= matches) {
          done_.store(true);
          return -1;
        }

        shift_bchr = (s + plen < en) ? plen - data.badchars[text[s + patlen]]
                                     : static_cast<index_t>(1);
        shift_gsfx = static_cast<index_t>(data.shift[0]);
      } else {
        shift_gsfx = static_cast<index_t>(data.shift[j + 1]);
        shift_bchr =
            std::max(static_cast<index_t>(1), j - data.badchars[text[s + j]]);
      }

      s += std::max(shift_gsfx, shift_bchr);
    }
    return search_count_;
  }

 private:
  PreProcFactory registry_;
  Streamer streamer_;
  std::atomic<size_t> search_count_{0};
  std::atomic<bool> done_{false};
};
}  // namespace hhf112
