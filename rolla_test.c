#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "rolla.h"

int main() {
    printf("-- load --\n");
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("%u/%u\n", (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);
    rolla *db = rolla_create("db");
    gettimeofday(&tv, NULL);
    printf("%u/%u\n", (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);


    int i;
    char buf2[8] = {0};
    printf("-- write --\n");
    gettimeofday(&tv, NULL);
    printf("%u/%u\n", (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);
    for (i=0; i < 100000; i++) {
        snprintf(buf2, 8, "%d", i % 2 ? i : 4);
        rolla_set(db, buf2, (uint8_t *)buf2, 8);
    }
    gettimeofday(&tv, NULL);
    printf("%u/%u\n", (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);
    sleep(8);

    printf("-- read --\n");
    uint32_t sz;
    gettimeofday(&tv, NULL);
    printf("%u/%u\n", (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);
    for (i=0; i < 100000; i++) {
        snprintf(buf2, 8, "%d", i % 2 ? i : 4);
        char *p = (char *)rolla_get(db, buf2, &sz);
        assert(p && !strcmp(buf2, p));
        free(p);
        if (i % 100000 == 0)
            printf("%d\n", i);
    }
    gettimeofday(&tv, NULL);
    printf("%u/%u\n", (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);

    sleep(8);

    snprintf(buf2, 8, "%d", 4);
    char *p = (char *)rolla_get(db, buf2, &sz);
    assert(p);
    free(p);
    rolla_del(db, buf2);
    p = (char *)rolla_get(db, buf2, &sz);
    assert(!p);

    rolla_close(db, 1);

    return 0;
}
