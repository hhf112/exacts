raw. expect slow commits. currenty under porototyping under testing branch.

## Moore Search
A header only implementation of parallelized Boyre Moore exact string searching algorithm.

## Benchmark run 
for the benchmark:

1. clone the repo and `cd` into it
2. run `sh build`
3. run: `./search <filename> <pattern>`

### Sample output
```
hrsh $(LAPTOP-HK58DTQE):~/dev/moore$🌙 ./srch 100mb.txt example
classical search function find: 136 ms.
found: 1000000
parallel search function pfind: 107 ms.
found: 1000002
```

