#include "rolla.h"

#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define NO_BACKTRACE ((uint32_t)0xffffffff)
#define CACHE_SIZE (32 * 1024)
#define DEBUG_PRINT_TRUNCATE 1

struct rolla {
    uint32_t offsets[NUMBUCKETS];
    int mapfd;
    char *path;
    uint8_t *map;
    uint32_t mmap_alloc;
    uint32_t eof;
    uint8_t cache[CACHE_SIZE];
    int cache_used;
};

static void rolla_remap(rolla *r) {
    if (r->map) {
        msync(r->map, r->mmap_alloc, MS_SYNC);
        int s = munmap(r->map, r->mmap_alloc);
        assert(!s);
    }
    r->mmap_alloc = r->eof;
    if (r->mmap_alloc) {
        r->map = (uint8_t *)mmap(
            NULL, r->mmap_alloc, PROT_READ, MAP_SHARED, r->mapfd, 0);
    } else {
        r->map = NULL;
    }
    r->cache_used = 0;
}

static uint32_t jenkins_one_at_a_time_hash(char *key, size_t len);

#define hash jenkins_one_at_a_time_hash

static uint32_t rolla_index_lookup(rolla *r, char *key) {
    uint32_t fh = hash(key, strlen(key)) % NUMBUCKETS;
    return r->offsets[fh];
}

static uint32_t rolla_index_keyval(rolla *r, char *key, uint32_t off) {
    uint32_t fh = hash(key, strlen(key)) % NUMBUCKETS;

    uint32_t last = r->offsets[fh];
    r->offsets[fh] = off;

    return last;
}

static void rolla_load(rolla *r) {

    struct stat st;

    r->mapfd = open(r->path,
            O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    assert(r->mapfd > -1);

    int s = fstat(r->mapfd, &st);
    if (!s) {
        r->eof = st.st_size;
    }
    else {
        r->eof = 0;
    }

    rolla_remap(r);

    if (r->map) {
        int save_eof = r->eof;

        uint8_t *p = r->map;
        uint32_t off = 0;
        while (1) {
            if (off == r->eof) {
                break; /* no recovery, clean shutdown */
            }
            if (off + 9 > r->eof) {
#if(DEBUG_PRINT_TRUNCATE)
                fprintf(stderr, "rolla: recovering truncated db!\n");
#endif
                r->eof = off;
                break;
            }
            unsigned char klen = *(uint8_t *)p;
            uint32_t vlen = *(uint32_t *)(p + 1);
            uint32_t last = *(uint32_t *)(p + 5);
            uint32_t jump = 9 + klen + vlen;

            if (klen == 0 || (off + jump > r->eof)) {
#if(DEBUG_PRINT_TRUNCATE)
                fprintf(stderr, "rolla: recovering truncated db!\n");
#endif
                r->eof = off;
                break;
            }

            char *key = (char *)(p + 9);

            uint32_t prev = rolla_index_keyval(r, key, off);
            assert(prev == last);

            off += jump;
            p += jump;
        }

        if (save_eof != r->eof) {
            s = ftruncate(r->mapfd, (off_t)r->eof);
            assert(!s);
            rolla_remap(r);
        }

        lseek(r->mapfd, r->eof, SEEK_SET);
    }
}

rolla * rolla_create(char *path) {
    rolla *r = realloc(NULL, sizeof(rolla));
    r->path = malloc(strlen(path) + 1);
    strcpy(r->path, path);
    r->mmap_alloc = 0;
    r->cache_used = 0;
    memset(r->offsets, 0xff, NUMBUCKETS * sizeof(uint32_t));

    rolla_load(r);

    return r;
}

uint8_t * rolla_get(rolla *r, char *key, uint32_t *len) {
    uint32_t off = rolla_index_lookup(r, key);

    while (off != NO_BACKTRACE) {
        uint8_t *p;
        if (off >= r->mmap_alloc) {
            p = &r->cache[off - r->mmap_alloc];
        } else {
            p = &r->map[off];
        }
        uint8_t klen = *(uint8_t *)p;
        if (!strncmp((char *)(p + 9), key, klen)) {
            uint32_t vlen = *(uint32_t *)(p + 1);
            if (vlen == 0) {
                return NULL;
            }
            uint8_t *res = malloc(vlen);
            *len = vlen;
            memmove(res, p + 9 + klen, vlen);
            return res;
        }
        off = *(uint32_t *)(p + 5);
    }

    return NULL;
}

void rolla_sync(rolla *r) {
    int s = fsync(r->mapfd);
    assert(!s);
}

void rolla_set(rolla *r, char *key, uint8_t *val, uint32_t vlen) {
    uint8_t klen = strlen(key) + 1;
    uint32_t step = 9 + klen + vlen;
    /* write to the mapfd */
    uint32_t last = rolla_index_keyval(r, key, r->eof);
    struct iovec iov[] = {
        {&klen, sizeof(uint8_t)},
        {&vlen, sizeof(uint32_t)},
        {&last, sizeof(uint32_t)},
        {key, klen},
        {val, vlen}};

    int bwrite = writev(r->mapfd, iov, 5);
    assert(bwrite == step);
    r->eof += step;

    if (r->cache_used + step > CACHE_SIZE) {
        rolla_remap(r);
    }
    else {
        /* write to trailing cache */
        uint8_t *p = &r->cache[r->cache_used];

        *((uint8_t *)p) = klen;
        *((uint32_t *)(p + 1)) = vlen;
        *((uint32_t *)(p + 5)) = last;
        memmove(p + 9, key, klen);
        if (vlen)
            memmove(p + 9 + klen, val, vlen);
        r->cache_used += step;
    }
}

void rolla_del(rolla *r, char *key) {
    rolla_set(r, key, NULL, 0);
}

void rolla_iter(rolla *r, rolla_iter_cb cb, void *passthrough) {
    int i;

    int sl = 10 * 1024;
    char *s = realloc(NULL, sl);

    rolla_remap(r); /* since we do not use the write-ahead cache */

    for (i = 0; i < NUMBUCKETS; i++) {
        uint32_t search_off = 0;
        s[0] = 0;
        uint32_t off = r->offsets[i];
        char buf[258];
        buf[0] = 1;
        while (off != NO_BACKTRACE) {
            uint8_t klen = *(uint8_t *)(r->map + off);
            char *key = (char *)(r->map + off + 9);
            strcpy(buf + 1, key);
            buf[klen] = 1;
            buf[klen + 1] = 0;
            uint32_t skey_len = klen + 2;
            if (!strstr(s, buf)) {
                /* not found */
                uint32_t vlen = *(uint32_t *)(r->map + off + 1);
                if (vlen) {
                    uint8_t *val = (uint8_t *)(r->map + off + klen + 9);
                    cb(r, key, val, vlen, passthrough);
                }
                if (search_off + skey_len >= sl) {
                    sl *= 2;
                    s = realloc(s, sl);
                }
                strcpy(s + search_off, buf);
                search_off += skey_len - 1; /* trailing \0 */
            }

            off = *(uint32_t *)(r->map + off + 5);
        }
    }

    free(s);
}

static void rolla_rewrite_cb(rolla *r, char *key, uint8_t *data, uint32_t length, void *pass) {
    rolla *new = (rolla *)pass;
    rolla_set(new, key, data, length);
}

void rolla_close(rolla *r, int compress) {
    char path[1200] = {0};
    if (compress) {
        assert(strlen(r->path) < 1100);
        strcat(path, r->path);
        strcat(path, ".rolla_rewrite");

        rolla *tmp = rolla_create(path);
        rolla_iter(r, rolla_rewrite_cb, (void *)tmp);
        rolla_close(tmp, 0);
    }

    munmap(r->map, r->mmap_alloc);
    close(r->mapfd);

    if (compress) {
        rename(path, r->path);
    }

    free(r->path);
    free(r);
}

/* From Bob Jenkins/Dr. Dobbs */
static uint32_t jenkins_one_at_a_time_hash(char *key, size_t len)
{
    uint32_t hash, i;
    for(hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}
