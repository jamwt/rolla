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

 * `rolla` is **NOT THREAD SAFE** .  If you want to access `rolla`
   databases in a multithreaded environment, you'll have to serialize
   access in a higher layer.
 * Keys are null-terminated C-Strings, not bytestrings
 * Keys must be <=255 bytes
 * An entire database cannot exceed sizeof(uint32_t) 4GB
 * Misses are expensive (they must read approx 1/256th of the database
   off disk to invalidate the key)
 * Compaction only happens when the database is destroyed and reloaded,
   it cannot be compacted while online
 * Files are assumed little endian and are not portable across
   architectures of different byte orderings
 * `rolla` does not call `fsync()`, but instead relies on the operating
   system's schedule.  Some data that was not totally committed to
   the backing hardware could be lost if system failure happened
   (though the database will come up cleanly with a subset of
   records, "repair" is never necessary)

Design
------

TBD

Author
------

Jamie Turner <jamie@bu.mp>
