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
 * Database files can compact on close, so amortized disk use is O(keys)
 * A key enumeration interface is supported
 * On-disk representation has only 10 bytes of overhead for each
   key/value pair
 * Already in use on millions of iOS and Android devices (armv7)

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
   records, "repair" is never necessary).  Applications can
   call rolla_sync() if they want to force msync more often.
 * A database created with a certain bucket count must always be
   used with that bucket count.  The bucket count cannot change.

Design
------

Rolla has a very simple design.

When a rolla database is created, an array of buckets is allocated
(by default 8192).  Database keys are hashed onto these buckets.
The bucket values contain the offset to the _last_ entry in the
file on disk that contains a key that was hashed into that bucket.

On a read, this last value is checked.  If the key matches, the
value is returned; otherwise, the record at this offset in the
file contains a link back to the *previous* offset in this bucket,
and so on.  Traversal continues until a record is encountered
with a sentinel value that indicates there are no more records
in this bucket.  This is why misses (key not found) are
expensive in rolla.

A write is simply a matter of overwriting the in-memory bucket
value to point to the current EOF marker in the on-disk file,
then taking the old value from this bucket and writing it out
to the disk as the previous link in the chain, along with the
new key and value.  In this way, writes are always appends.

Mutating the same key over and over again will grow the
database without bound.  `rolla_close()` takes a `compress`
flag that will walk the database and write a new version
with only the current values for each key.  It is important
to close and compress active rolla databases regularly to
both preserve disk space and prevent them from exceeding
their 4GB limit.  Compaction is done in a crash-safe manner
using a tempfile and `rename()`.

When a database is reloaded, the file is walked and the
bucket array is re-populated with the final offsets.  Any
partial records are truncated in case of a crash.

Rolla uses the `mmap()` syscall to memory-map this file as
an array of records (C structs).

Moving the NUMBUCKETS constant up and down can tune rolla
for different expected key counts.  Clearly, larger bucket
counts result in fewer seeks on disk because only a
very few keys will map to that bucket, and link walking is
minimal or absent.

Rolla was designed to run on 32-bit systems, so offset
values are typically unsigned 32-bit integers.  This limits
total database size to ~4GB.

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
