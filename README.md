under active development. tests to be added.


## Moore Search
A header only implementation of parallelized Boyre Moore exact string searching algorithm.

## Benchmark run 
Mote: build system to be added.

1. clone the repo and `cd` into it
2. run `sh build`
3. run: `./search <filename> <pattern> <max search count>`

### Sample output
```
hrsh $(LAPTOP-HK58DTQE):~/dev/moore$🌙 ./srch 800mb.txt example 10000000
classical search function find: 1446 ms.
found: 10000000
parallel search function pfind: 1119 ms.
found: 10000004
```
# Documentation 
under work.

# Public 

## nested types

### PatternData struct
```
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

## members

### patternCache
```
std::unordered_map<std::string, PatternData> patternCache;
```
### Note
 A better implementation possibly a round robin hashmap to be added.


## Functions

## find
```
std::optional<OutputItStart> 
find(const std::string &m_path,
                const std::string &pattern,
                OutputItStart beg,
                int matches = MAX_MATCHES);

```
appends all matches found into container iterated by beg until specified matches are found or eof encountered
### Parameters
`path`: file path to search in
`pattern`: pattern to search for
`beg`: input iterator of the container matches are appended to
`matches`: (optional) maximum number of matches to look for

### Return values
1. beg translated by number of matches appended on success
2. {} on fail

### Notes
2. May return repeated indexes due to 
    1. overlapped chunks to avoid misse

## pfind: threaded find

```
inline std::optional<OutputItStart>
pfind(const std::string &path,
            const std::string &pattern,
            OutputItStart beg,
            int matches = MAX_MATCHES);

```
appends all matches found into container iterated by beg until specified matches are found or eof encountered
### Parameters
`path`: file path to search in
`pattern`: pattern to search for
`beg`: input iterator of the container matches are appended to
`matches`: (optional) maximum number of matches to be specified

### Return values
1. beg translated by number of matches appended on success
2. {} on fail

### Notes
1. May return unordered indexes as total matches are counted by threads running all over the chunk
2. May return repeated indexes due to 
    1. local search space overlapping of each thread to avoid misses
    2. overlapped chunks to avoid misse

## search

```
  template <typename OutputItStart>
inline int search(const std::string &text,
                const std::string &pat,
                size_t startPos,
                size_t endPos,
                size_t startIndex,
                OutputItStart beg,
                int matches = MAX_MATCHES);
```
appends all matches found into container iterated by beg until specified matches are found or eof encountered
### Parameters
`text`: text to search on
`pat`: pattern to search for
`startPos`: search start index inclusive
`endPos`: search end index exclusive
`startIndex`: (optional) index to appened on every find
`beg`: input iterator of the container matches are appended to
`matches`: (optional) maximum number of matches to look for

### Return values
1. number of matches

## parallelSearch: threaded search

```
template <typename OutputItStart>
inline std::optional<OutputItStart> 
parallelSearch(const std::string &text,
                const std::string &pattern,
                size_t startIndex,
                OutputItStart beg,
                int matches = MAX_MATCHES);
```
appends all matches found into container iterated by beg until specified matches are found or eof encountered
### Parameters
`text`: text to search on
`pat`: pattern to search for
`startPos`: search start index inclusive
`endPos`: search end index exclusive
`startIndex`: (optional) index to appened on every find
`beg`: input iterator of the container matches are appended to
`matches`: (optional) maximum number of matches to look for

### Return values
1. translated beg by number of matches on success
2. {} on fail

### Notes
2. May return repeated and unordered indexes due to 
    1. overlapped local search space to avoid misses


## preprocess_pattern
```
inline void preprocess_pattern(int nchars, const std::string &pattern);
```
adds preprocessed tables to cache if not existing.

### Parameters
`nchars`: number of badchars
`pattern`: pattern to search for

### Return values
1. void


## getters and setters 
```
  inline void set_search_count(size_t n) { m_search_count = n; }
  inline int get_search_count() { return m_search_count; }
  inline void set_chunk_size(size_t n) { m_chunk_size = n; }

  inline std::string &getBuf() { return m_buffer; }
  inline const std::string &getBufconst() { return m_buffer; }
  inline std::string getPath() { return m_path; }

```









