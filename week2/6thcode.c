#include <stdio.h>
int main()
{
    int num1, num2;
    printf("first number : ");
    scanf("%d", &num1);
    printf("second number : ");
    scanf("%d", &num2);
    printf("Inputted numbers: %d. %d\n", num1, num2);
    printf("--------\n");
    printf("Sum:%d\n", num1 +num2);
    printf("Difference:%d\n", num1-num2);
    printf("Product: %d\n", num1*num2);
    return 0;
} 
