#ifndef ACCOUNTING_MAIN_H
#define ACCOUNTING_MAIN_H

#include <sys/stat.h>
#include <errno.h>
#include "stdio.h"
#include "stdlib.h"
#include <fcntl.h>
#include <unistd.h>

#define MSG_ACC_CHECK 4
#define MSG_ACC_UPDATE 8
#define MSG_ACC_RUN 16
#define MSG_ACC_DISC 32

void initMCPipes(int *inputFD, int *outputFD, int username);
void mCHandler(int inputMCFD, int outputMCFD, int username);
float balanceCheck(int username);
float balanceUpdate(int username, float amount);
int balanceRun(int username, float amount);
void sendReply(int outputMCFD, int reply);

typedef struct accIntent{
    int msgId;
    float amount;
}accIntent;

#endif