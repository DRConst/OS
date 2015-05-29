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
    int inputClientMC, outputClientMC, inputClientAcc, outputClientAcc, stdoutClient, stdinClient;
    //</editor-fold>

    int userName = 0;
    ssize_t bytesRead;
    char buff[32];

    initPipes(&inputPipeFD, &outputPipeFD);
    initAccountingPipes(&accountingInputPipe, &accoutingOutputPipe);

    while(1)
    {
        bytesRead = read(inputPipeFD, buff, 32);
        sleep(1);
        userName = atoi(buff);
        login(userName);

        //printf("asd");
        if(bytesRead > 0)
        {
            log("Client Connected\n");
            printf("Client ");
            printf(buff);
            printf(" Connected\n");
            if(fork() != 0)//Is Child
            {
                write(accountingInputPipe, &userName, sizeof(int)); /*Warn Accounting of New Connection to Fork*/
                initUserPipes(&inputClientMC, &outputClientMC, &inputClientAcc, &outputClientAcc, &stdinClient, &stdoutClient, userName);
                log("User Pipes Inited\n");
                dup2(STDOUT_FILENO, stdoutClient);
                dup2(stdinClient, STDIN_FILENO);
                clientHandler(inputClientMC, outputClientMC, inputClientAcc, outputClientAcc,accountingInputPipe, accoutingOutputPipe, userName);
            }
            else{
                exit(1);
            }

        }

    }
    return 0;
}


void clientHandler(int inputClientMC, int outputClientMC, int inputClientAcc, int outputClientAcc, int accountingInputPipe, int accoutingOutputPipe, int userName)
{
    Intent it;
    int cnt;
    int shouldClose = 0;
    float bal;
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
                    printf("User %04x has a Balance of %f\n", userName, (bal = balanceCheck(inputClientAcc, outputClientAcc,accountingInputPipe, userName)));
                    write(outputClientMC, &bal, sizeof bal);
                    break;
                case MSG_BAL_UPDATE:
                    read(inputClientMC, &bal, sizeof bal);
                    printf("User %04x has a Balance of %f\n", userName, (bal = balanceUpdate(inputClientAcc, outputClientAcc,bal, userName)));
                    write(outputClientMC, &bal, sizeof bal);
                    break;
                case MSG_MC_CLOSE:
                    log("Client Disconnected");
                    printf("User %04x has logged out\n", userName);
                    it.msgId = MSG_ACC_DISC;
                    write(outputClientAcc, &it, sizeof it);
                    close(outputClientAcc);
                    close(inputClientAcc);
                    exit(1);/*
                    shouldClose = 1;
                    break;*/
                default:
                    printf("Intent Not Recognized\n");
                    break;
            }
        }
    }
    while(!shouldClose);
}


float balanceCheck(int inputClientAcc, int outputClientAcc, int accountingInputPipe, int userName)
{
    /*Talk to Accounting*/
    Intent i;
    i.msgId = MSG_ACC_CHECK;
    char buff[64];
    float bal;
    sprintf(buff, "%d", userName);


    write(outputClientAcc, &i, sizeof i);/*Write out Message to Accounting On the Exclusive Pipe*/

    read(inputClientAcc, &bal, sizeof(float));

    return bal;

}

float balanceUpdate(int inputClientAcc, int outputClientAcc, float bal, int userName)
{
    /*Talk to Accounting*/
    Intent i;
    i.msgId = MSG_ACC_UPDATE;
    char buff[64];
    sprintf(buff, "%d", userName);


    write(outputClientAcc, &i, sizeof i);/*Write out Message to Accounting On the Exclusive Pipe*/


    write(outputClientAcc, &bal, sizeof bal);

    read(inputClientAcc, &bal, sizeof(float));

    return bal;

}

void execCommand(int inputClientMC, int outputClientMC, int inputClientAcc, int outputClientAcc, int userName, int dataSize)
{
    int killFlag = 0;
    int pid;
    int fd[2];
    char psRet[1024];
    char buff[64];
    char *cmdString = malloc(dataSize + 1);
    accIntent ai;
    ai.msgId = MSG_ACC_RUN;
    ai.amount = 1.0f;
    if((pid = fork()) != 0)/*TODO: Flip*/
    {
        while(!killFlag) {

            pipe(fd);

            if(fork() != 0)
            {
                close(fd[1]);
                read(fd[0], psRet, sizeof psRet);
            } else{

                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                sprintf(buff, "%d", pid);
                execlp("pidstat", "pidstat","-p", buff, NULL);
            }

            printf("%s\n", psRet);
           // char *asd = strtok(psRet, "\n");
             //= strtok(NULL, "\n");
            //printf("%s\n", asd);
            sleep(1);
            killFlag = 1;
            /*Calc Next Bill*/
        }
        kill(pid, SIGKILL);
        //exit(1);

    }else{
        do{
            dataSize = read(inputClientMC, cmdString, dataSize);
        }while(dataSize <= 0);
        system(cmdString);
        printf("Got intent with datasize of %d\n", dataSize);
        exit(1);
    }

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

void initUserPipes(int *inputClientMC, int *outputClientMC, int *inputClientAcc, int *outputClientAcc, int *stdinClient, int *stdoutClient, int userName)
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
    //dup2(*outputClientMC, STDOUT_FILENO);

    sprintf(buff , "/tmp/%04xAccountingInput.pipe", userName);


    if(mkfifo(buff, 0666) != 0)
    {
        if(errno != EEXIST){
            printf("Input Pipe Creation Failed\n");
            exit(1);
        }

    }

    *inputClientAcc = open(buff , O_RDONLY);
    //dup2(STDIN_FILENO, *inputFD);

    sprintf(buff , "/tmp/%04xAccountingOutput.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *outputClientAcc = open(buff , O_WRONLY);

    sprintf(buff , "/tmp/%04xStdin.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *stdinClient = open(buff , O_RDONLY);

    sprintf(buff , "/tmp/%04xStdout.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }


    *stdoutClient = open(buff , O_WRONLY);
    //dup2(STDOUT_FILENO, *stdoutClient);
    dup2(*stdoutClient , STDOUT_FILENO);
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

    *accountingInputPipe = open("/tmp/accountingInput.pipe", O_WRONLY );
    *accountingOutputPipe = open("/tmp/accountingOutput.pipe", O_RDONLY );
}