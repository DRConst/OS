#include <asm-generic/errno-base.h>
#include "main.h"

/*TODO: Signal Handling*/
typedef void (*sighandler_t)(int);
void test(int a)
{
    printf("Signal");
}

int main()
{
    int inputPipeFD, outputPipeFD;
    int inputClientFD, outputClientFD;
    int userName;
    int bytesRead;


    initPipes(&inputPipeFD, &outputPipeFD);

    signal(SIGINT, test);
    while(1)
    {
        bytesRead = read(inputPipeFD, &userName, sizeof(int));

        login(userName);

        //printf("asd");

        if(bytesRead > 0 && fork() == 0)//Is Child
        {

            initUserPipes(&inputClientFD, &outputClientFD, userName);

            clientHandler(inputClientFD, outputClientFD, userName);
        }
    }
    return 0;
}


void clientHandler(int inputClientFD, int outputClientFD, int userName)
{
    Intent it;
    while(read(inputClientFD, &it, sizeof(it)) >= 0)
    {
        switch (it.msgId)
        {
            case MSG_EXEC_CMD:
                break;
            case MSG_BAL_CHECK:
                printf("User %04x has a Balance of %f\n", userName, balanceCheck(userName));
                break;
            case MSG_BAL_UPDATE:
                balanceUpdate(userName);
                break;
            default:
                printf("Intent Not Recognized");
                break;
        }
    }
}


float balanceCheck(int userName)
{
    /*Talk to Accounting*/

    return 0.0f;

}

void balanceUpdate(int userName)
{
    /*Talk to Accounting*/

    printf("%04x has a Balance of %f", userName, balanceCheck(userName));

}

void execCommand(int userName, int dataSize)
{
    int killFlag = 0;
    int pid;
    char *cmdString = malloc(dataSize);

    if((pid = fork()) != 0)
    {
        killFlag = 0; /*Talk to accounting for kill command*/
        if(killFlag)
            kill(pid, SIGKILL);

        /*Relay Stats*/
    }else{

        while(read(STDIN_FILENO, cmdString, dataSize) == 0);/*STDIN has been overwritten by ClientPipe*/
        {/*Waiting on Data*/};

        system(cmdString);

    }

}



void initPipes(int *inputFD, int *outputFD)
{
    if(mkfifo("/tmp/missionControlInput.pipe", 0777) != 0)
    {
        if(errno != EEXIST){
            printf("Input Pipe Creation Failed\n");
            exit(1);
        }

    }

    if(mkfifo("/tmp/missionControlOutput.pipe", 0777) != 0)
        if(errno != EEXIST)
        {
            printf("Output Pipe Creation Failed\n");
            exit(1);
        }

    *inputFD = open("/tmp/missionControlInput.pipe", O_RDONLY | O_NONBLOCK);
    *outputFD = open("/tmp/missionControlOutput.pipe", O_WRONLY | O_NONBLOCK);

}

void initUserPipes(int *inputFD, int *outputFD, int userName)
{
    char buff[32];


    sprintf(buff , "/tmp/%04xInput.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    {
        if(errno != EEXIST){
            printf("Input Pipe Creation Failed\n");
            exit(1);
        }

    }

    *inputFD = open(buff , O_RDONLY);
    dup2(STDIN_FILENO, *inputFD);

    sprintf(buff , "/tmp/%04xOutput.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *outputFD = open(buff , O_WRONLY);
    dup2(STDOUT_FILENO, *outputFD);
}

