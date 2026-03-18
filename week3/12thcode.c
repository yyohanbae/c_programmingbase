#include <stdio.h>
int main()
{
    int a = 2;
    if (a < 0) 
    {
        printf("a is negative.\n");
    } 
    else if (a < 10) 
    {
        printf("a is less than 10.\n");
    } 
    else if (a < 3) 
    {
        printf("a is less than 3.\n");
    } 
    else 
    {
        printf("a is other.\n");
    }
    return 0;
}