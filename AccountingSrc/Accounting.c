#include <string.h>
#include "Accounting.h"

int main()
{
    int inputPipeFD, outputPipeFD;
    int inputMCFD, outputMCFD;
    int bytesRead, userSize;
    char *userName;

    initPipes(&inputPipeFD, &outputPipeFD);

    while(1)
    {
        bytesRead = read(inputPipeFD, &userSize, sizeof(int));
        userName = malloc(userSize);
        read(inputPipeFD, userName, userSize);
        sleep(1);
        if(bytesRead > 0 && fork() == 0)//Is Child
        {
            initMCPipes(&inputMCFD, &outputMCFD, userName);

            mCHandler(inputMCFD, outputMCFD, userName);
        }
    }

    return 0;
}
void initPipes(int *inputFD, int *outputFD)
{
    if(mkfifo("/tmp/accountingInput.pipe", 0777) != 0)
    if(errno != EEXIST)
    {
        printf("Input Pipe Creation Failed\n");
        exit(1);
    }

    if(mkfifo("/tmp/accountingOutput.pipe", 0777) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *inputFD = open("/tmp/accountingInput.pipe", O_RDONLY);/*Blocks Until MC is Ready to Start*/
    *outputFD = open("/tmp/accountingOutput.pipe", O_WRONLY);

}

void initMCPipes(int *inputFD, int *outputFD, char *userName)
{

    char buff[32];

    sprintf(buff , "/tmp/%sAccountingInput.pipe", userName);


    if(mkfifo(buff, 0666) != 0)
    {
        if(errno != EEXIST){
            printf("Input Pipe Creation Failed\n");
            exit(1);
        }

    }

    *inputFD = open(buff , O_WRONLY);/*Read and Write Direction Are Dictated Towards the MC*/
    //dup2(STDIN_FILENO, *inputFD);

    sprintf(buff , "/tmp/%sAccountingOutput.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *outputFD = open(buff , O_RDONLY);
    //dup2(STDOUT_FILENO, *outputFD);
}

void mCHandler(int inputMCFD, int outputMCFD, char *userName)
{
    accIntent acIt;
    int bytesRead;
    float balance, delta;
    int kill = 0;
    while(1)
    {
        bytesRead = read(outputMCFD, &acIt, sizeof(acIt));
        usleep(100);
        if(bytesRead == sizeof acIt)
            switch (acIt.msgId)
            {
                case MSG_ACC_CHECK:
                    printf("User %s has a Balance of %f\n", userName, (balance = balanceCheck(userName)));
                    write(inputMCFD, &balance, sizeof(float));
                    break;
                case MSG_ACC_UPDATE:
                    read(outputMCFD, &balance, sizeof balance);
                    printf("User %s has a Balance of %f\n", userName, (balance = balanceUpdate(userName, balance)));
                    write(inputMCFD, &balance, sizeof(float));
                    //balanceUpdate(userName, acIt.amount);
                    break;
                case MSG_ACC_RUN:
                    printf("%f CPU", acIt.amount);
                    (kill = (balanceUpdate(userName, acIt.amount) <= 0));
                    write(inputMCFD, &kill, sizeof kill);
                    break;
                case MSG_ACC_DISC:
                    printf("User %s has Disconnected", userName);
                    close(inputMCFD);
                    close(outputMCFD);
                    exit(0);
                    return;
                default:
                    printf("Accounting Intent Not Recognized");
                    exit(0);
                    break;
            }
    }
}

float balanceCheck(char *userName)
{
    char buff[32];
    float balance;
    FILE *fp;

    sprintf(buff , "/tmp/%04x.bal", userName);

    if((fp = fopen(buff,"r")) != NULL)
    {
        fscanf(fp,"%s", buff);
        balance = atof(buff);
        fclose(fp);
    }
    else
        balance = 0;

    return balance;
}

float balanceUpdate(char *userName, float amount)
{
    float oldBalance = balanceCheck(userName);
    char buff[32];
    float balance = -1;
    FILE *fp;

    balance = amount + oldBalance;

    sprintf(buff, "/tmp/%04x.bal", userName);

    if ((fp = fopen(buff, "w")) != NULL) {
        sprintf(buff, "%f\0", balance);
        fwrite(buff, strlen(buff) + 1, 1, fp);

        fclose(fp);

        return balance;
    }
    else
        printf("Oh Darn! Something went wrong trying to write to file!\n");
    return balance;
}

int balanceRun (char *userName, float amount)
{
    float oldBalance = balanceCheck(userName);
    float balance;

    balance = amount + oldBalance;

    if(balance <= 0)
        return 0;
    else
        balanceUpdate(userName, amount);

    return 1;
}
