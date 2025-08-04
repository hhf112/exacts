
# ExactS (formerly Moore Search) <img src = "https://img.shields.io/github/actions/workflow/status/hhf112/moore-search/c-cpp.yml" alt="build status">
a *superfast* exact string searcher available for both sequential and parallel workflows. Go ahead and give it a try!
 
Details: A header only implementation of parallelized Boyre Moore exact string searching algorithm. compatible with C++17.
Why did I make this?:
- I refuse to parse the WHOLE JSON-like data types (JSON INCLUDED) with a HUGE header/even LARGER libary when I need a single key just because.
- There are many more cases like the above.

# Benchmarks
to be added.
~38% faster than single threaded Boyre Moore for now.included
^ older benchmark before last patch. I think it is definately faster now. 

recently made breaking changes. will update docs soon. Doxygen comments have been added.
