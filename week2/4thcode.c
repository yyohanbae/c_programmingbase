#include<stdio.h>
int main()
{
    unsigned char b = 254;
    printf("b %%d: %d\n", b);
    printf("b %%i: %i\n", b);
    printf("b %%c: %c\n", b);
    printf("b %%f: %f\n", (float)b);
    printf("b %%e: %e\n", b);
    printf("-----\n");
    return 0;
}