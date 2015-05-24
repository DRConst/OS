
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


int main();
void initPipes(int *inputFD, int *outputFD);
void initUserPipes(int *inputFD, int *outputFD, int userName);
void login(int username){};
void clientHandler(int inputClientFD, int outputClientFD, int userName);
void execCommand(int userName, int dataSize);
float balanceCheck(int userName);
void balanceUpdate(int userName);

typedef struct intent{
    int msgId, dataSize;
}Intent;
#endif //MISSIONCONTROL_MAIN_H
