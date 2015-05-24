#ifndef ACCOUNTING_MAIN_H
#define ACCOUNTING_MAIN_H

#include <sys/stat.h>
#include <errno.h>
#include "stdio.h"
#include "stdlib.h"
#include <fcntl.h>
#include <unistd.h>

#define MSG_BAL_CHECK 0
#define MSG_BAL_UPDATE 1
#define MSG_BAL_RUN 2

void initMCPipes(int *inputFD, int *outputFD, int username);
void mCHandler(int inputMCFD, int outputMCFD, int username);
float balanceCheck(int username);
void balanceUpdate(int username, float amount);
int balanceRUN(int username, float amount);
void sendReply(int outputMCFD, int reply);

typedef struct accIntent{
    int msgId;
    float amount;
}accIntent;

#endif