
# ExactS (formerly Moore Search) <img src = "https://img.shields.io/github/actions/workflow/status/hhf112/moore-search/c-cpp.yml" alt="build status">
A **superfast** exact string searcher available for both sequential and parallel workflows. Go ahead and give it a try! <br>
### What is it?: 
- A header only exact string searching API based on Boyre Moore's exact string searching algorithm, parallelized and sequential. compatible with C++17. <br>
### Why did I make this?:
- I hate parsing JS*N.
- There are many more cases like the above.

# Perf
| Metric          | `find` (Single-Threaded) | `pfind` (Parallel) | Speedup               |
|-----------------|-------------------------|--------------------|-----------------------|
| **Avg. Time**   | 407 ms                  | 180 ms             | **55.8% faster**      |
| **Best Case**   | 395 ms                  | 116 ms             | **70.6% faster**      |
| **Worst Case**  | 473 ms                  | 244 ms             | **48.4% faster**      |
```bash
hrsh $(LAPTOP-HK58DTQE):~/dev/exacts$🌙 ./srch 100mb.txt example
classical search function find: 2742430 in 473 ms.
parallel search function pfind: 2742430 in 152 ms.
hrsh $(LAPTOP-HK58DTQE):~/dev/exacts$🌙 ./srch 100mb.txt example
classical search function find: 2742430 in 410 ms.
parallel search function pfind: 2742430 in 157 ms.
hrsh $(LAPTOP-HK58DTQE):~/dev/exacts$🌙 ./srch 100mb.txt example
classical search function find: 2742430 in 411 ms.
parallel search function pfind: 2742430 in 242 ms.
hrsh $(LAPTOP-HK58DTQE):~/dev/exacts$🌙 ./srch 100mb.txt example
classical search function find: 2742430 in 395 ms.
parallel search function pfind: 2742430 in 116 ms.
hrsh $(LAPTOP-HK58DTQE):~/dev/exacts$🌙 ./srch 100mb.txt example
classical search function find: 2742430 in 401 ms.
parallel search function pfind: 2742430 in 240 ms.
hrsh $(LAPTOP-HK58DTQE):~/dev/exacts$🌙 ./srch 100mb.txt example
classical search function find: 2742430 in 412 ms.
parallel search function pfind: 2742430 in 153 ms.
hrsh $(LAPTOP-HK58DTQE):~/dev/exacts$🌙 ./srch 100mb.txt example
classical search function find: 2742430 in 397 ms.
parallel search function pfind: 2742430 in 152 ms.
hrsh $(LAPTOP-HK58DTQE):~/dev/exacts$🌙 ./srch 100mb.txt example
classical search function find: 2742430 in 403 ms.
parallel search function pfind: 2742430 in 148 ms.
hrsh $(LAPTOP-HK58DTQE):~/dev/exacts$🌙 ./srch 100mb.txt example
classical search function find: 2742430 in 409 ms.
parallel search function pfind: 2742430 in 244 ms.
```
# Docs
Recently made breaking changes, will update docs soon. 
- Doxygen comments are added for docs.
