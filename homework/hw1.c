#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main()
{
    srand(time(NULL)); //needed for making choices random everytime
    char playername[50];
    int playerchoice;
    int opponentchoice; 
    char *moves[] = {"", "Rock", "Paper", "Scissor"};
    char *semichoices[2] = {"Alex", "Steve"};
    char *finalschoices[4] = {"Jun", "David", "Oliver", "Sofia"};
    int semiopponent = rand() % 2;
    int finalsopponent = rand() % 4;

    printf("\nWelcome to the 8-Player Rock-Paper-Scissor Tournament!\nWhat's your name?\n");
    scanf("%s", playername);
    printf("--------------------------------------\n");
    printf("[BRACKET]\n\n[1] %s VS [2] John\n[3] Alex VS [4] Steve\n[5] Jun VS [6] David\n[7] Oliver VS [8] Sofia\n", playername);
    printf("--------------------------------------\n");
    printf("QUARTERFINALS:\nYour first opponent is [2] John\n");
    printf("--------------------------------------\n");
    while (1)
    {
        printf("1: Rock   2: Paper   3: Scissor\nYour choice? > ");
        scanf("%d", &playerchoice);
        if (playerchoice < 1 || playerchoice > 3) 
        {
            printf("\nInvalid choice, try again!\n");
            continue;
        }
        opponentchoice = rand() % 3 + 1; //% 3 gives 0, 1, 2, so we add 1 to make it 1, 2, 3
        printf("\nYou chose: %s\n", moves[playerchoice]);
        printf("John chose: %s\n", moves[opponentchoice]);
        break;
    }
    




    printf("Your next opponent is: %s\n", semichoices[semiopponent]); //use later
    return 0;
}