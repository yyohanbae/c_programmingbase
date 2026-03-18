#include<stdio.h>
int main()
{
    if (0) printf("Print 0 run!!\n");
    if (1) printf("Print 1 run!!\n");
    if (-5) printf("Print -5 run!!\n");
    return 0;
    //when there are 2 operations like (-5 <= a < 4), it'll only run "-5 <= a" so make it ((-5 <= a) && (a < 4))
}