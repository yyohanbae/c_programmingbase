#include<stdio.h>
int main()
{
    int a = 2;
    float a2 = 2.0;
    int b = 3;
    float c = 3.0;
    printf("a %% b: %d\n", a % b);
    printf("a %% c: %d\n", a % (int)c);
    //float and int dont work tgt so you gotta change one of the two by attaching either (int) or (float) in front of the variable
    printf("a2 %% b: %d\n", (int)a2 % b);
    return 0;
}