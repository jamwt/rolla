#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "rolla.h"

double doublenow() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (double)tv.tv_sec 
        + (((double)tv.tv_usec) / 1000000.0);
}

int main() {
    double start;
    printf("-- load --\n");
    start = doublenow();
    rolla *db = rolla_create("db");
    printf("load took %.3f\n", 
    doublenow() - start);

    int i;
    char buf2[8] = {0};
    printf("-- write --\n");
    
    start = doublenow();
    for (i=0; i < 10000000; i++) {
        snprintf(buf2, 8, "%d", i % 2 ? i : 4);
        rolla_set(db, buf2, (uint8_t *)buf2, 8);
    }
    printf("write took %.3f\n", 
    doublenow() - start);
    sleep(8);

    printf("-- read --\n");
    uint32_t sz;
    start = doublenow();
    for (i=0; i < 10000000; i++) {
        snprintf(buf2, 8, "%d", i % 2 ? i : 4);
        char *p = (char *)rolla_get(db, buf2, &sz);
        assert(p && !strcmp(buf2, p));
        free(p);
        if (i % 100000 == 0)
            printf("%d\n", i);
    }
    printf("read took %.3f\n", 
    doublenow() - start);

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
