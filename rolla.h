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

typedef struct rolla {
    uint32_t offsets[NUMBUCKETS];
    int mapfd;
    char *path;
    uint8_t *map;
    int mmap_alloc;
    int eof;

} rolla;

rolla * rolla_create(char *path);
void rolla_sync(rolla *r);
void rolla_close(rolla *r, int compress);

void rolla_set(rolla *r, char *key, uint8_t *val, uint32_t vlen);
uint8_t * rolla_get(rolla *r, char *key, uint32_t *len);
void rolla_del(rolla *r, char *key);

typedef void(*rolla_iter_cb) (rolla *r, char *key, uint8_t *val, uint32_t length, void *passthrough);
void rolla_iter(rolla *r, rolla_iter_cb cb, void *passthrough);

#endif /* ROLLA_H */
