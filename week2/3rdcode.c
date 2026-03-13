#include<stdio.h>

int main()
{
    int a=65;
    printf("a %%d: %d\n", a);
    printf("a %%i: %i\n", a);
    printf("a %%c: %c\n", a+32);
    printf("a %%f: %f\n", (float)a);
    printf("-----\n");
    return 0;
}