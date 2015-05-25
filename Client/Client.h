//
// Created by nooblevler on 24/05/15.
//

#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H


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

typedef struct intent{
    int msgId, dataSize;
}Intent;

int main();
void commandDialog(int inputPipe, int outputPipe, int userName);

#endif //CLIENT_CLIENT_H
