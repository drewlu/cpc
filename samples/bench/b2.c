#include <stdio.h>

extern int f(int);

int
main()
{
    int i, j;
    volatile int s;
    for(i = 0; i < 100000; i++) {
        s = 0;
        for(j = 0; j < 1000; j++)
            s += f(j);
    }
    printf("%d\n", s);
    return 0;
}
