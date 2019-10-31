# MapReduce

Multithreaded MapReduce library using a POSIX threads ThreadPool implementation.

## Usage

### Library

- Include `mapreduce.h` in your program header.
- Provide your `Mapper` and `Reducer` functions. These should follow this definition:

  ```
  typedef void (*Mapper)(char *file_name);
  typedef void (*Reducer)(char *key, int partition_number);
  ```

  A Mapper uses _MR_Emit_ `void MR_Emit(char *key, char *value)` to Map a key to
  value while the Reducer uses `char *MR_GetNext(char *key, int partition_number)` to get values for
  a key from a specific partition. See `distwc.cpp` for sample implementation.

- Run MapReduce using _MR_Run_

  ```
    void MR_Run(int num_files, char \*filenames[],
    Mapper map, int num_mappers,
    Reducer concate, int num_reducers);
  ```

  Here `num_mappers` and `num_reducers` are the number of mapper and reducer threads to be initalized by
  MapReduce.

### Sample Worcount Program

- To generate wordcount program binary, run `make`.
- To generate object files, run `make compile`. You can generate the program binary by running `make wc` after.
- To remove the compiled files run `make clean`.
- To run the wordcount program after generating binary, run `./wordcount` followed by the files as arguments. For ex `./wordcount testcase/sample1.txt testcase/sample2.txt testcase/sample3...`

- To generate compressed submission run `make compress`.

## Design

The Map part of _MapReduce_ brings up _M_ worker threads managed by a ThreadPool. Then _N_ files are
processed by these threads in a longest job first priority policy and the mapped values are written to _R_
partitions (where _R_ is the number of Reducers) where each partition is a Map mapping a key to a vector of
values. Any calls to `MR_Emit` hash the key (first argument of the call) to a particular partition and add
the value to the map for the key's vector. This takes O(log(n)) time since the keys are required to be
sorted and C++ maps (ordered maps) are red-black trees internally.

After all the mapping jobs are done, _R_ reducer threads are brought up and work on a single partition each.
Each call to `MR_GetNext` with a key returns the next unprocessed value for a key or `NULL` if all values
have been returned. This is done by popping values off from a key's vector one by one. This too takes
O(log(n)) time since finding a key in an ordered map takes O(log(n)) time.

## Known Issues

- Using `valgrind` to profile this library can sometimes stall (albeit very rarely). This seems to be a
  common problem with older versions of valgrind with PThreads since it forces them to run on a single core
  (https://stackoverflow.com/questions/10134638/valgrind-hanging-to-profile-a-multi-threaded-program). In my
  case, running the program again on stall usually leads to clean run which profiles the program perfectly
  (with no detectable leaks). Redirecting output to a file using --log-file="valgrind.%p.txt" makes it work
  better too.
