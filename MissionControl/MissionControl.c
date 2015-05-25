#include <asm-generic/errno-base.h>
#include <string.h>
#include "MissionControl.h"

FILE *fp;
void log(char *str)
{
    fp = fopen("/tmp/log", "a");
    fwrite(str,strlen(str), 1, fp);;
    fclose(fp);
}

/*TODO: Signal Handling*/
typedef void (*sighandler_t)(int);
int main()
{
    //<editor-fold desc="General Immutable Pipes">
    int inputPipeFD, outputPipeFD;
    int accountingInputPipe, accoutingOutputPipe;
    //</editor-fold>


    //<editor-fold desc="Fork Specific Pipes">
    int inputClientMC, outputClientMC, inputClientAcc, outputClientAcc;
    //</editor-fold>

    int userName = 0;
    ssize_t bytesRead;
    char buff[32];

    initPipes(&inputPipeFD, &outputPipeFD);
    initAccountingPipes(&accountingInputPipe, &accoutingOutputPipe);

    while(1)
    {
        bytesRead = read(inputPipeFD, buff, 32);
        userName = atoi(buff);
        login(userName);

        //printf("asd");
        if(bytesRead > 0)
        {
            log("Client Connected\n");
            printf("Client ");
            printf(buff);
            printf(" Connected\n");
            //if(fork() != 0)//Is Child
            //{

                initUserPipes(&inputClientMC, &outputClientMC, &inputClientAcc, &outputClientAcc, userName);
                log("User Pipes Inited\n");

                clientHandler(inputClientMC, outputClientMC, inputClientAcc, outputClientAcc, userName);
           // }
        }

    }
    return 0;
}


void clientHandler(int inputClientMC, int outputClientMC, int inputClientAcc, int outputClientAcc, int userName)
{
    Intent it;
    int cnt;
    do{
        cnt = read(inputClientMC, &it, sizeof(it));
        if(cnt > 0)
        {
            switch (it.msgId)
            {
                case MSG_EXEC_CMD:
                    execCommand(inputClientMC, outputClientMC, inputClientAcc,outputClientAcc,userName,it.dataSize);
                    break;
                case MSG_BAL_CHECK:
                    printf("User %04x has a Balance of %f\n", userName, balanceCheck(userName));
                    break;
                case MSG_BAL_UPDATE:
                    balanceUpdate(userName);
                    break;
                default:
                    printf("Intent Not Recognized\n");
                    break;
            }
        }
    }
    while(1);
    //while(read(inputClientMC, &it, sizeof(it)) <= 0)
    //{

    //}
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

void execCommand(int inputClientMC, int outputClientMC, int inputClientAcc, int outputClientAcc, int userName, int dataSize)
{
    int killFlag = 0;
    int pid;
    char *cmdString = malloc(dataSize + 1);
    accIntent ai;
    ai.msgId = MSG_ACC_RUN;
    ai.amount = 1.0f;
    /*if((pid = fork()) == 0)
    {/*
        while(!killFlag) {
            char buff[1024];
            memcpy(buff, &ai, sizeof ai);
            write(inputClientAcc, buff, sizeof ai);
            read(outputClientAcc, buff, 1024);

            killFlag = atoi(read);
            sleep(1000);
            /*Calc Next Bill
        }
        kill(pid, SIGKILL);

    }else{*/
        int i = 0;
        //while(read(inputClientMC, cmdString, dataSize) <= 0)/*STDIN has been overwritten by ClientPipe*/
        //{i++;};
        //while(strlen(cmdString) != dataSize)
        do{
            dataSize = read(inputClientMC, cmdString, dataSize);
        }while(dataSize <= 0);
        system(cmdString);
        printf("Got intent with datasize of %d\n", dataSize);

    //}

}



void initPipes(int *inputFD, int *outputFD)
{
    if(mkfifo("/tmp/missionControlInput.pipe", 0666) != 0)
    {
        if(errno != EEXIST){
            printf("Input Pipe Creation Failed\n");
            exit(1);
        }

    }

    if(mkfifo("/tmp/missionControlOutput.pipe", 0666) != 0)
        if(errno != EEXIST)
        {
            printf("Output Pipe Creation Failed\n");
            exit(1);
        }

    *inputFD = open("/tmp/missionControlInput.pipe", O_RDONLY | O_NONBLOCK);
    *outputFD = open("/tmp/missionControlOutput.pipe", O_WRONLY | O_NONBLOCK);

}

void initUserPipes(int *inputClientMC, int *outputClientMC, int *inputClientAcc, int *outputClientAcc, int userName)
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

    *inputClientMC = open(buff , O_RDONLY);/*Will Block Until Client Catches Up*/
    //dup2(STDIN_FILENO, *inputFD);

    sprintf(buff , "/tmp/%04xOutput.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *outputClientMC = open(buff , O_WRONLY);/*Will Block Until Client Catches Up*/
    //dup2(*outputFD, STDOUT_FILENO);

    sprintf(buff , "/tmp/%04xAccountingInput.pipe", userName);

    /*
    if(mkfifo(buff, 0666) != 0)
    {
        if(errno != EEXIST){
            printf("Input Pipe Creation Failed\n");
            exit(1);
        }

    }
    */
    *inputClientAcc = open(buff , O_RDONLY);
    //dup2(STDIN_FILENO, *inputFD);

    sprintf(buff , "/tmp/%04xAccountingOutput.pipe", userName);
    /*
    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }
    */
    *outputClientAcc = open(buff , O_WRONLY);
}

void initAccountingPipes(int *accountingInputPipe, int *accountingOutputPipe)
{
    if(mkfifo("/tmp/accountingInput.pipe", 0666) != 0)
    {
        if(errno != EEXIST){
            printf("Input Pipe Creation Failed\n");
            exit(1);
        }

    }

    if(mkfifo("/tmp/accountingOutput.pipe", 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *accountingInputPipe = open("/tmp/accountingInput.pipe", O_RDONLY | O_NONBLOCK);
    *accountingOutputPipe = open("/tmp/accountingOutput.pipe", O_WRONLY | O_NONBLOCK);
}