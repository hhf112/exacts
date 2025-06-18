under development. tests to be added.

# Moore Search <img src = "https://img.shields.io/github/actions/workflow/status/hhf112/moore-search/c-cpp.yml" alt="build status">
A header only implementation of parallelized Boyre Moore exact string searching algorithm. compatible with C++17.

# Benchmarks
to be added.
~38% faster than single threaded Boyre Moore for now.

# Test run 
1. clone the repo and `cd` into it
2. run `sh build`
3. run: `./search <filename> <pattern> <max search count>`

#### Note
build system to be added.

### Sample output
```console
hrsh $(LAPTOP-HK58DTQE):~/dev/moore$🌙 ./srch 800mb.txt example 10000000
classical search function find: 1446 ms.
found: 10000000
parallel search function pfind: 1119 ms.
found: 10000004
```
# Public API
## getters and setters 
```cpp
  inline void set_search_count(size_t n) { m_search_count = n; }
  inline int get_search_count() { return m_search_count; }
  inline void set_chunk_size(size_t n) { m_chunk_size = n; }

  inline std::string &getBuf() { return m_buffer; }
  inline const std::string &getBufconst() { return m_buffer; }
  inline std::string getPath() { return m_path; }
```
## Nested types
### PatternData struct
```cpp
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
```
#### References
https://www.geeksforgeeks.org/boyer-moore-algorithm-good-suffix-heuristic/
https://www.geeksforgeeks.org/boyer-moore-algorithm-for-pattern-searching/


## Members

### patternCache
```cpp
std::unordered_map<std::string, PatternData> patternCache;
```
#### Note
 A better implementation possibly a round robin hashmap to be added.
## Functions
### find
```cpp
template <typename OutputItStart>
inline std::optional<OutputItStart> find(const std::string &path,
                                  const std::string &pattern,
                                  OutputItStart beg,
                                  int matches = MAX_MATCHES);

```
```yaml
Params
path       file path to search in
pattern    pattern to search for
beg        input iterator of the container matches are appended to
matches     maximum number of matches to look for [optional]

Return    
success    beg translated by number of matches appended on success
fail       {}
```
Runs `search` on every chunk. Appends all matches found into container iterated by beg until specified matches are found or eof encountered
#### Notes
May return repeated indexes due to overlapped chunks to avoid search misses

### pfind: threaded find

```cpp
template <typename OutputItStart>
inline std::optional<OutputItStart> pfind(const std::string &path,
                                        const std::string &pattern,
                                        OutputItStart beg,
                                        int matches = MAX_MATCHES);


```
```yaml
Params
path       file path to search in
pattern    pattern to search for
beg        input iterator of the container matches are appended to
matches    maximum number of matches to be specified [optional]

Return     
success   beg translated by number of matches appended on success
fail      {}
```
Runs `parallelSearch` on every chunk. Appends all matches found into container iterated by beg until specified matches are found or eof encountered
#### Notes
1. May return unordered indexes as total matches are counted by threads running all over the chunk
2. May return repeated indexes due to local search space overlapping of each thread and overlapped chunks to avoid misses

### search

```cpp
template <typename OutputItStart>  inline int search(const std::string &text,
                                                    const std::string &pat,
                                                    size_t startPos,
                                                    size_t endPos,
                                                    size_t startIndex,
                                                    OutputItStart beg,
                                                    int matches = MAX_MATCHES);
```
```yaml
Params
text           text to search on
pat            pattern to search for
startPos       search start index inclusive
endPos         search end index exclusive
startIndex     index to appened on every find
beg            input iterator of the container matches are appended to
matches        maximum number of matches to look for [optional]

Return          number of matches
```
Performs classical boyre moore seach and determines shifts by the maximum of good suffix heuristic 
and bad character heuristic. Appends all matches found into container iterated by beg until specified matches are found or eof encountered. An atomic counter
is polled between iterations for checking search count.

### parallelSearch: threaded search
```cpp
template <typename OutputItStart> inline std::optional<OutputItStart> 
parallelSearch(const std::string &text,
                const std::string &pattern,
                size_t startIndex,
                OutputItStart beg,
                int matches = MAX_MATCHES);
```
```yaml
Params
text           text to search on
pat            pattern to search for
startPos       search start index inclusive
endPos         search end index exclusive
startIndex     index to appened on every find
beg            input iterator of the container matches are appended to
matches        maximum number of matches to look for [optional]

Return          
success         beg translated by number of matches found
fail            {}
```
Allocates partitions of the `text` to `search` threads. Appends all matches found first into local containers for threads then
ensembles into container iterated by beg until specified matches are found or eof encountered. Search counting is atomic.

### preprocess_pattern
```cpp
inline void preprocess_pattern(int nchars, const std::string &pattern);
```
```yaml
Params
nchars    number of badcharacters
pattern   pattern to search for
```
adds preprocessed tables to cache if not existing.












