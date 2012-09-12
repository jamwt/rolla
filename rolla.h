#ifndef ROLLA_H
#define ROLLA_H

#include <inttypes.h>

#define NUMBUCKETS 8192

/* XXX locking? */
typedef struct rolla {
    uint32_t offsets[NUMBUCKETS];
    int mapfd;
    char *path;
    uint8_t *map;
    int mmap_alloc;
    int eof;

} rolla;

rolla * rolla_create(char *path);
void rolla_close(rolla *r, int compress);

void rolla_set(rolla *r, char *key, uint8_t *val, uint32_t vlen);
uint8_t * rolla_get(rolla *r, char *key, uint32_t *len);
void rolla_del(rolla *r, char *key);

typedef void(*rolla_iter_cb) (rolla *r, char *key, uint8_t *val, uint32_t length, void *passthrough);
void rolla_iter(rolla *r, rolla_iter_cb cb, void *passthrough);

#endif /* ROLLA_H */
