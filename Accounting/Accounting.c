#include "Accounting.h"

int main()
{
    int inputPipeFD, outputPipeFD;
    int inputMCFD, outputMCFD;
    int bytesRead, username;

    initPipes(&inputPipeFD, &outputPipeFD);

    while(1)
    {
        bytesRead = read(inputPipeFD, &username, sizeof(int));

        if(bytesRead > 0 && fork() != 0)//Is Child
        {
            initMCPipes(&inputMCFD, &outputMCFD, username);

            mCHandler(inputMCFD, outputMCFD, username);
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

void initMCPipes(int *inputFD, int *outputFD, int username)
{

    char buff[32];

    sprintf(buff , "/tmp/%04xAccountingInput.pipe", username);


    if(mkfifo(buff, 0666) != 0)
    {
        if(errno != EEXIST){
            printf("Input Pipe Creation Failed\n");
            exit(1);
        }

    }

    *inputFD = open(buff , O_WRONLY);/*Read and Write Direction Are Dictated Towards the MC*/
    //dup2(STDIN_FILENO, *inputFD);

    sprintf(buff , "/tmp/%04xAccountingOutput.pipe", username);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *outputFD = open(buff , O_RDONLY);
    //dup2(STDOUT_FILENO, *outputFD);
}

void mCHandler(int inputMCFD, int outputMCFD, int username)
{
    accIntent acIt;
    int bytesRead;
    float balance;
    while(1)
    {
        bytesRead = read(outputMCFD, &acIt, sizeof(acIt));
        if(bytesRead >= 0)
        switch (acIt.msgId)
        {
            case MSG_ACC_CHECK:
                printf("User %04x has a Balance of %f\n", username, (balance = balanceCheck(username)));
                write(inputMCFD, &balance, sizeof(float));
                break;
            case MSG_ACC_UPDATE:
                balanceUpdate(username, acIt.amount);
                break;
            case MSG_ACC_RUN:
                sendReply(outputMCFD, balanceRun(username, acIt.amount));
                break;
            default:
                printf("Accounting Intent Not Recognized");
                break;
        }
    }
}

float balanceCheck(int username)
{
    char buff[32];
    float balance;
    FILE *fp;

    sprintf(buff , "%04x.bal", username);

    if((fp = fopen(buff,"r")) != NULL)
    {
        fscanf(fp,"%f", &balance);

        fclose(fp);
    }
    else
        balance = 0;

    return balance;
}

void balanceUpdate(int username, float amount)
{
    float oldBalance = balanceCheck(username);
    char buff[32];
    float balance;
    FILE *fp;

    balance = amount + oldBalance;

    sprintf(buff, "%04x.bal", username);

    if ((fp = fopen(buff, "w")) != NULL) {
        fwrite(&balance, 1, sizeof(balance), fp);

        fclose(fp);
    }
    else
        printf("Oh Darn! Something went wrong trying to write to file!\n");
}

int balanceRun (int username, float amount)
{
    float oldBalance = balanceCheck(username);
    float balance;

    balance = amount + oldBalance;

    if(balance <= 0)
        return 0;
    else
        balanceUpdate(username, amount);

    return 1;
}

void sendReply(int outputMCFD, int reply)
{
    char ch[2];

    sprintf(ch, "%d", reply);

    write(outputMCFD, ch, 2);
}
