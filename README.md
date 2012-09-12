rolla
===

`rolla` is a simple key/value storage engine.  It aims to achieve a good balance
of fast query performance, low disk space, and data safety while being stingy
with memory.  It was designed primarily for embedding in mobile applications.

Features
--------

 * Database is pure C and relies only on the C standard library
 * Database is crash-only design; database files cannot go corrupt
 * Inserts are O(1)
 * Queries are O(1) .. O(n), though on typical workloads with hot
   spots the performance is closer to O(1) due to scanning hash buckets
   in write-recency order
 * An idle (but fully loaded) database uses < 40kB of memory with
   default tuning parameters
 * Database files compact on close, so amortized disk use is O(keys)
 * A key enumeration interface is supported
 * On-disk representation has only 10 bytes of overhead for each
   key/value pair

Caveats
-------

 * `rolla` databases are **NOT THREAD SAFE**.  If you want to access a
  `rolla` database from multiple threads concurrently, you'll have to
   serialize access in a higher layer.  The library is fully
   re-entrant, however--so you can use *different* databases
   in multiple threads concurrently.
 * Keys are null-terminated C-Strings, not bytestrings
 * Keys must be <=255 bytes
 * An entire database cannot exceed 4GB
 * Misses are expensive (they must read approx 1/8192 of the database
   off disk to invalidate the key)
 * Compaction only happens when the database is closed and reloaded,
   it cannot be done while online
 * Files are assumed little endian and are not portable across
   architectures of different byte orderings
 * `rolla` does not call `fsync()` or `msync()`, but instead relies on
   the operating system's schedule.  Some data that was not totally
   committed to the backing hardware could be lost if system failure
   happened (though the database will come up cleanly with a subset of
   records, "repair" is never necessary)

Design
------

TBD

Benchmarks
----------

**Hardware**

    VMWare Fusion
    Arch Linux 3.3.7-1-ARCH #1 SMP PREEMPT x64
    Intel(R) Core(TM) i7-2677M CPU @ 1.80GHz
    Crucial 256GB SSD
    1.4GB RAM (likely on-disk file fully in page cache)

**Data**

    Keys and values are 1-6 bytes [1..1000000].

**Test 1**

    NUMBUCKETS=8192
    100,000 writes, and 100,000 reads
    writes 3609742.327/s
    reads  4034846.852/s
    (note: this is the sweet spot for this library)

**Test 2**

    NUMBUCKETS=8192
    1,000,000 writes, and 1,000,000 reads
    writes 5558771.063/s
    reads   621547.615/s
    (note: too few buckets, too much chaining)

**Test 3**

    NUMBUCKETS=262144
    1,000,000 writes, and 1,000,000 reads
    writes 4818579.943/s
    reads  3342098.868/s
    (note: less chaining and more req'd ram.. ~1MB)

Author
------

Jamie Turner <jamie@bu.mp>
