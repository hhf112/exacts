prototyping under testing branch.
Documentation to be added soon.

## Moore Search
A header only implementation of parallelized Boyre Moore exact string searching algorithm.

## Benchmark run 
for the benchmark:

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

