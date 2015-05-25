#include <string.h>
#include "Client.h"

int main()
{
    /**/
    int userName  = 123;
    char buff[32];
    int registerPipe, inputPipe, outputPipe;/*Input and Output Refer to MissionControl*/
    int answer;

    /*Get User Credentials*/

    printf("Welcome, Please Enter Your Username (31 Char Limit): ");
    //scanf("%d", &userName);
    printf("\n");

    /*END*/

    /*Warn the Server of Client*/
    sprintf(buff, "%d", userName);
    registerPipe = open("/tmp/missionControlInput.pipe", O_WRONLY);
    write(registerPipe, buff, 4);

    /*Get the pipes to the server*/
    sprintf(buff, "/tmp/%04xInput.pipe", userName);
    do{
        inputPipe = open( buff , O_WRONLY);
    }
    while(inputPipe <= 0);/*Hold Until the Server Creates the Pipe*/
    sprintf(buff, "/tmp/%04xOutput.pipe", userName);
    do{
        outputPipe = open( buff , O_RDONLY);
    }
    while(outputPipe <= 0);/*Hold Until the Server Creates the Pipe*/

    printf("PIPED");
    //write(inputPipe, "\0", 1);
    printf("What do you wish to do?\n1 - Input Commands\n2 - Check Balance\n3 - Update Balance\n");
    scanf("%d", &answer);

    switch (answer)
    {
        case 1:
            commandDialog(inputPipe, outputPipe, userName);
            break;
        case 2:
            break;
        case 3:
            break;
        default:
            return -1;
    }
    read(outputPipe, buff, sizeof buff);
    printf(buff);
    printf("\n");
    return 0;
}


void commandDialog(int inputPipe, int outputPipe, int userName)
{

    char stB[64];
    printf("You are now connected to the server, Ctlr+D disconnects\n");
    Intent i;
    i.msgId = MSG_EXEC_CMD;

    //memcpy(stB, &i, sizeof i);
    //write(inputPipe, stB, sizeof i)
    while(1)
    {
        char buff[64];
        char null[1];
        null[0] = '\0';
        int asd;
        //scanf("%s", buff);
        do{
            fflush(stdout);
            fgets(buff, 64, stdin);
        }while((asd = strcmp(buff, " \n")) <= 0);


        i.dataSize = (int) (strlen(buff));
        strtok(buff, "\n");
        memcpy(stB, &i, sizeof i);

        if(!strcmp(buff, "MC_QUIT"))
        {

        } else{

            write(inputPipe, stB, sizeof i);

            write(inputPipe, buff, i.dataSize);
        }

    }
}