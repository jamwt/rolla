export CFLAGS="-g -Wall -Werror -pedantic -std=gnu99 -O2 -fno-strict-aliasing"
gcc $CFLAGS -o rolla_test rolla.c rolla_test.c 
