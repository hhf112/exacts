
# ExactS (formerly Moore Search) <img src = "https://img.shields.io/github/actions/workflow/status/hhf112/moore-search/c-cpp.yml" alt="build status">
A **superfast** exact string searcher available for both sequential and parallel workflows. Go ahead and give it a try! <br>
### What is it?: 
- A header only exact string searching API based on Boyre Moore's exact string searching algorithm, parallelized and sequential. compatible with C++17. <br>
### Why did I make this?:
- I hate parsing JS*N.
- There are many more cases like the above.

# Perf (100MB, 2.74 Million finds)
| Metric          | `find` (Single-Threaded) | `pfind` (Parallel) | Speedup               |
|-----------------|-------------------------|--------------------|-----------------------|
| **Avg. Time**   | 407 ms                  | 180 ms             | **55.8% faster**      |
| **Best Case**   | 395 ms                  | 116 ms             | **70.6% faster**      |
| **Worst Case**  | 473 ms                  | 244 ms             | **48.4% faster**      |
- tested on Intel i5-12500H, Windows-11 (No GPU optimizations included)
## test.cpp
```bash
git clone git@github.com:hhf112/exacts.git
```
```bash
cd exacts && sh build
```
```
./srch <file_name> <pattern>
```
# API
```cpp 
namespace hhf112 {
// GOLBAL typedefs
using arith_t = std::ptrdiff_t;

class ExactS {
 public:
  static constexpr int MAX_MATCHES = 1000000;

  // CONSTRUCTOR
  ExactS() = default;

  // API

  /* @brief reset search count and flag to stop threads. */
  inline void reset_search();
  /* @brief search_count is atomic */
  inline int get_search_count();

  /*
   * @brief run {action} for every occurance in buffer
   * @params {action(it, en)}:
   *     @params {it} iterator to occurance
   *     @params {en} iterator to end of buffer
   * @note occurances may be random and repeated
   */
  inline int pfind(
      const std::string &path, const std::string &pattern,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, size_t matches = MAX_MATCHES);

  /*
   * @brief run {action} for every occurance in each buffer
   * @params {action(it, en)}:
   *     @params {it} iterator to occurance
   *     @params {en} iterator to end of buffer
   * @note occurances may be repeated
   */
  inline int find(
      const std::string &path, const std::string &pattern,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, size_t matches = MAX_MATCHES);

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
      int nchars = 256, size_t matches = MAX_MATCHES);

  /*
   * @brief run {action} for every occurance in {text}
   * @params {action(it, en)}:
   *     @params {it} iterator to occurance
   *     @params {en} iterator to end of {text}
   */
  inline int search(
      const std::string &text, const std::string &pattern, size_t startPos,
      size_t endPos,
      const std::function<void(std::string::const_iterator it,
                               std::string::const_iterator en)> &action,
      int nchars = 256, size_t matches = MAX_MATCHES);

 private:
  PreProcFactory registry_;
  Streamer streamer_;
  std::atomic<size_t> search_count_{0};
  std::atomic<bool> done_{false};
};

struct PreProced {
  std::vector<size_t> shift, bpos;
  std::vector<arith_t> badchars;
  int NCHARS;

  PreProced() = default;
  PreProced(int nchars, size_t patternlen) : NCHARS{nchars};
};

class PreProcFactory {
 public:
  PreProcFactory() = default;

  /*@brief fetch from or create in preprocessed pattern cache*/
  inline const PreProced &getPreProced(int nchars, const std::string &str);

 private:
  /*@src
   * https://www.geeksforgeeks.org/dsa/boyer-moore-algorithm-for-pattern-searching/*/
  inline void badCharHeuristic(std::vector<arith_t> &badhcars,
                               const std::string &str, size_t size);

  /*@src
   * https://www.geeksforgeeks.org/dsa/boyer-moore-algorithm-good-suffix-heuristic/*/
  inline void preprocess_strong_suffix(std::vector<size_t> &shift,
                                       std::vector<size_t> &bpos,
                                       const std::string &pat, size_t m);

  /*@src
   * https://www.geeksforgeeks.org/dsa/boyer-moore-algorithm-good-suffix-heuristic/*/
  inline void preprocess_case2(std::vector<size_t> &shift,
                               std::vector<size_t> &bpos,
                               const std::string &pat, size_t m);

  std::unordered_map<std::string, PreProced> record_;
};

class Streamer {
 public:
  static constexpr int MB = 1048576;
  static constexpr int MAX_CHUNK_LIMIT = 200 * MB;
  static constexpr int END_STREAM = 1;
  static constexpr int CONT_STREAM = 0;

  inline void endStream();
  inline int get_chunk_size();
  inline void set_chunk_size(size_t n);

  /*
   * @brief set file, moderate passed chunk size
   */
  inline int startStream(const std::string &p);

  /*
   * @brief run action for every chunk read. copy trailing {overlap} length of
   *each chunk to the front.
   *@detail first is read is chunk_size + overlap
   */
  inline int forStream(size_t overlap,
                       const std::function<int(const std::string &)> &action);
 private:
  std::fstream file_;
  std::string path_;
  std::string buffer_;
  size_t chunk_size_ = 2 * MB;
}

} //namespace hhf112

```
