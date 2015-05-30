
#ifndef MISSIONCONTROL_MAIN_H
#define MISSIONCONTROL_MAIN_H

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "stdio.h"
#include "stdlib.h"

#define MSG_EXEC_CMD 0
#define MSG_BAL_CHECK 1
#define MSG_BAL_UPDATE 2
#define MSG_MC_CLOSE 4

#define MSG_ACC_CHECK 4
#define MSG_ACC_UPDATE 8
#define MSG_ACC_RUN 16
#define MSG_ACC_DISC 32



int main();
void initPipes(int *inputFD, int *outputFD);
void initUserPipes(int *inputMC, int *outputMC, int *inputAcc, int *outputAcc,int *stdinClient, int *stdoutClient, int userName);
void login(int username){};
void clientHandler(int inputClientMC, int outputClientMC, int inputClientAcc, int outputClientAcc, int accountingInputPipe, int accoutingOutputPipe, int userName);
void execCommand(int inputClientMC, int outputClientMC, int inputClientAcc, int outputClientAcc, int userName, int dataSize);
float balanceCheck(int inputClientAcc, int outputClientAcc, int accountingInputPipe, int userName);
float balanceUpdate(int inputClientAcc, int outputClientAcc, float bal, int userName);
void initAccountingPipes(int *accountingInputPipe, int *accountingOutputPipe);

typedef struct intent{
    int msgId, dataSize;
}Intent;
typedef struct accIntent{
    int msgId;
    float amount;
}accIntent;

typedef struct cmd {

    char *op;
    char **args;
    int pipeIn, pipeOut;
    int argCount;
} *Cmd;

typedef struct commands {
    Cmd *arrCmds;
    int count;
} *Commands;


void auxExec( char *input );
pid_t execStat( char *strCmd );
Cmd createCmd( char *str );
Commands CmdsInit( char *first );
void CmdsNext( Commands cmds, char *str );
void CmdsExec( Commands cmds );
void freeCommands( Commands cmds );

char *trim( char *in );



#endif //MISSIONCONTROL_MAIN_H


