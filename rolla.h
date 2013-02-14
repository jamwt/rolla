#ifndef ROLLA_H
#define ROLLA_H

#include <inttypes.h>

/* The whole system swings on this parameter...

Set it to something large (128k to 256k buckets)
for something with tens of millions of keys.
256k will need at least ~1MB per database.

Keep it small (8k) for very good performance with
low memory overhead (<40kB) on embedded systems
with less than 1M keys.
*/
#define NUMBUCKETS (1024 * 8)

/* This is your database, friend. */
typedef struct rolla rolla;

/* Create a database (in a single file) at `path`.
   If `path` does not exist, it will be created;
   otherwise, it will be loaded. */
rolla * rolla_create(char *path);

/* Force the database to be sync'd to disk (msync) */
void rolla_sync(rolla *r);

/* Close the database; compress=1 means rewrite the database
   to eliminate redundant values for each single key */
void rolla_close(rolla *r, int compress);

/* Set `key` to byte array `val` of `vlen` bytes; you still
   own key and val, they are not retained */
void rolla_set(rolla *r, char *key, uint8_t *val, uint32_t vlen);

/* Get the value for `key`, which will be `*len` bytes long.
   NULL will be returned if the key is not found.  Otherwise,
   a value will be returned to you that is allocated on the heap.
   You own it, you must free() it eventually. */
uint8_t * rolla_get(rolla *r, char *key, uint32_t *len);

/* Remove the value `key` from the database.  Harmless NOOP
   if `key` does not exist */
void rolla_del(rolla *r, char *key);

/* Iterate over all keys in the database.  See the note in the
   README.md about caveats associated with iteration and mutation */
typedef void(*rolla_iter_cb) (rolla *r, char *key, uint8_t *val, uint32_t length, void *passthrough);
void rolla_iter(rolla *r, rolla_iter_cb cb, void *passthrough);

#endif /* ROLLA_H */
