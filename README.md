
# Parallelized Boyre Moore Search
A library for parallelized exact string searching using the classical Boyre Moore Algorithm.


# Installation and benchmark build
a simple bash build script (build) is attached to avoid the hassle of object files. 

1. clone the repo and `cd` into it
2. run `sh build`

# Benchmark run 
for the benchmark run: <br>
`./search <filename> <pattern> <chunksize in mbs preferably >= 10>`

#### Sample Usage:

```
hrsh $(LAPTOP-HK58DTQE):/mnt/d/dev/poozle/exact-string-search$🌙 ./search 50mb.txt example 20
chunksize: 20971520 byte(s).
classical search function find: 818 ms.
found: 2516583
parallel search function pfind: 409 ms.
found: 2516583
parallel search function pfind_unique: 747 ms.
found: 2516583
```

# Documentation 
check BoyreMoore.h for concise comments  on every functionality.

## Constructor
```
BoyreMoore(size_t nchars) : nchars{nchars} { badchar.resize(nchars, -1); };
```
Initializes badchar with the possible bad chars in the search. refer  https://www.geeksforgeeks.org/boyer-moore-algorithm-for-pattern-searching/
## Vector returning search
all functions return an empty vector on being unable to open the inputted file.
## Non parallel search
### find:
```
std::vector<size_t> find(size_t chunkSize, const std::string &path,
                        const std::string &pattern);
```    
starts a filestream using `void startStream(size_t chunkSize, const std::string &path)` with specified chunk size. 
calls the forStream function 

`void forStream(size_t patternlen,
                 const std::function<void(const std::string &)> &action);`

and passes the classical Boyre Moore Search as a function pointer
```
void search(const std::string &text, const std::string &pat,
            const std::function<void(size_t)> &foreach, int l, int r,
            size_t startIndex);
```
for the complete range of the chunk on each chunk. Stores results in a temporary vector.  Concatenates all results in the result vector.

## Parallel find alternatives
```
  std::vector<size_t> pfind(size_t chunkSize, const std::string &path,
                         const std::string &pattern);

  //runs pfind but sorts and deduplicates the result with added overhead.
  std::vector<size_t> pfind_unique(size_t chunkSize, const std::string &path,
                                const std::string &pattern);

```

### pfind
```
  std::vector<size_t> pfind(size_t chunkSize, const std::string &path,
                         const std::string &pattern);
```
starts a file stream using 
`
void startStream(size_t chunkSize, const std::string &path)
`. 
 Calls forStream
 `
 void forStream(size_t patternlen,
                 const std::function<void(const std::string &)> &action)
`
and passes the following breakdown of a function pointer as action:<br>
A temporary vector initialized with the  parallel search function 
`
  std::vector<size_t> parallelSearch(const std::string &text,
                                  const std::string &pattern,
                                  size_t startIndex);
`then concatenates the temp vector into the result vector after processing every chunk.

### pfind_unique
```
std::vector<size_t> pfind_unique(size_t chunkSize, const std::string &path,
                                const std::string &pattern);
```
first does the same task as 
`
std::vector<size_t> pfind(size_t chunkSize, const std::string &path,
                         const std::string &pattern);
`
after concatenating all temp vectors into result vector, calls `std::sort` and `std::unique` followed by `std::erase` on the final result vector.

## Search Functions
### search
```
void search(const std::string &text, const std::string &pat,
              const std::function<void(size_t)> &foreach, int l, int r,
              size_t startIndex);

```
performs classical Boyre Moore Search. Shifts the pattern by the maxium of the preprocessed lengths from Bad Character Heuristic  `std::vector<int> badchar` & the Good Suffix Heuristic. runs foreach for each pattern found.
```
// No of shifts for every index preproccessed for Good suffix heuristic
  std::vector<int> shift;
  // border positions preprocessed for Good suffix heuristic
std::vector<int> bpos;
```
 references for classical Boyre Moore search preprocessing: 
    https://www.geeksforgeeks.org/boyer-moore-algorithm-good-suffix-heuristic/
   https://www.geeksforgeeks.org/boyer-moore-algorithm-for-pattern-searching/


### parallelSearch
```
std::vector<size_t> parallelSearch(const std::string &text,
                                  const std::string &pattern,
                                  size_t startIndex);
```
divides text length into forward overlapped partitions based on `std::thread::hardware_concurrency()`, dispatches a thread stored in ` std::vector<std::thread> threads(numThreads);` for each partition.
Corresponding threads push results to `std::vector<std::vector<int>> results(numThreads);`. Concatenates all results int returned vector. Returned vector may contain repetitions and may be unordered.

## I/O
### startStream
```
void startStream(size_t chunkSize, const std::string &path);
```
sets `int chunkSize` by the minimum of the passed value and `MAX_CHUNK_LIMIT` which is initially set to 128 MB(s). returns 1 on unsuccesful file stream. Resizes buffer to chunk size. 

### forStream 
```
void BoyreMoore::forStream(
    size_t patternlen, const std::function<void(const std::string &)> &action) 
```
A wrapper to run the `action` functor on every chunk processed. Chunks are backwards overlapped using `std::memcpy` with pattern length to avoid search misses. 
