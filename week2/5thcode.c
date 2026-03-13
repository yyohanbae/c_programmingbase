#include<stdio.h>
int main()
{
    char strc[5] = "abcd";
    printf("strc %%d: %d\n", strc);
    printf("strc %%i: %i\n", strc);
    printf("strc %%c: %c\n", strc);
    printf("strc %%s: %s\n", strc);
    printf("-----\n");
    return 0;
}