#include <string.h>
#include <stdio_ext.h>
#include "Client.h"

void checkBalance(int pipe, int outputPipe, int name);

void sigHandler(int s)
{
  if (s == SIGPIPE)
  {
      printf("Something went wrong, restarting\n");
      execlp("Client", "Client", NULL);
      exit(1);
  }
}


int main()
{
    /**/
    char userName[32], pw[32];
    int userSize;
    char buff[32];
    int login;
    int registerPipe, inputPipe, outputPipe;/*Input and Output Refer to MissionControl*/
    int stdinPipe, stdoutPipe;
    int answer = -1;
    int sF = 0;

    signal(SIGPIPE, sigHandler);

    /*Get User Credentials*/

    printf("Welcome, Please Enter Your Username (31 Char Limit): ");
    do{
        //fflush(stdout);
        memset(userName, '\0', sizeof userName);
        fgets(userName, sizeof userName, stdin);
    }while(strcmp(buff, " \n") <= 0);
    strtok(userName, "\n");
    userSize = strlen(userName) + 1;
    printf("\n");

    /*END*/

    /*Warn the Server of Client*/
    registerPipe = open("/tmp/missionControlInput.pipe", O_WRONLY);
    write(registerPipe, &userSize, sizeof userSize);
    write(registerPipe, userName, userSize);

    printf("Welcome, Please Enter Your Password (31 Char Limit | Insert | if N/A): ");
    do{
        //fflush(stdout);
        memset(pw, '\0', sizeof pw);
        fgets(pw, sizeof pw, stdin);
    }while(strcmp(buff, " \n") <= 0);
    strtok(pw, "\n");
    userSize = strlen(pw) + 1;
    if(userSize == 1)
    {
        userSize = 0;
        write(registerPipe, &userSize, sizeof userSize);
    }
    else{
        write(registerPipe, &userSize, sizeof userSize);
        write(registerPipe, pw, userSize);
    }


    /*Get the pipes to the server*/
    sprintf(buff, "/tmp/%sInput.pipe", userName);
    do{
        inputPipe = open( buff , O_WRONLY);
    }
    while(inputPipe <= 0);/*Hold Until the Server Creates the Pipe*/
    sprintf(buff, "/tmp/%sOutput.pipe", userName);
    do{
        outputPipe = open( buff , O_RDONLY);
    }
    while(outputPipe <= 0);/*Hold Until the Server Creates the Pipe*/

    sprintf(buff , "/tmp/%sStdin.pipe", userName);
    do {
    stdinPipe = open(buff, O_WRONLY);
    }while(stdinPipe <= 0);/*Hold Until the Server Creates the Pipe*/
    //dup2(STDIN_FILENO, stdinPipe);
    sprintf(buff , "/tmp/%sStdout.pipe", userName);
    do {
        stdoutPipe = open(buff, O_RDONLY);
    }while(stdoutPipe <= 0);/*Hold Until the Server Creates the Pipe*/

    if(fork() == 0)/*Replace with propper STDOUT routing*/
    {
        char b[2048];
        while(1)
        {
            memset(b, '\0', sizeof b);
            if(read(stdoutPipe, b, sizeof b) > 0)
            	printf("%s\n", b);
            else
            	usleep(100);
            if(getppid() == 1)
                exit(0);
        }

    }

    read(outputPipe, &login, sizeof login);
    if(!login)
       exit(1);

    fflush(stdin);
    while(answer)
    {

        //fflush(stdin);
        //__fpurge(stdin);
        //fflush(stdout);
        printf("What do you wish to do?\n1 - Input Commands\n2 - Check Balance\n3 - Update Balance\n0 - Exit\n");
        sF = scanf("%d", &answer);
        if(sF)
            switch (answer)
            {
                case 1:
                    commandDialog(inputPipe, outputPipe, userName);
                    break;
                case 2:
                    checkBalance(inputPipe, outputPipe, userName);
                    break;
                case 3:
                    updateBalance(inputPipe, outputPipe, userName);
                    break;
                default:
                    answer = 0;
            }
    }

    Intent quit;
    quit.msgId = MSG_MC_CLOSE;
    write(inputPipe, &quit, sizeof quit);
    return 0;
}

void checkBalance(int inputPipe, int outputPipe, int name)
{
    Intent i;
    i.msgId = MSG_BAL_CHECK;
    i.dataSize = 0;
    float bal;

    write(inputPipe, &i, sizeof i); /*Write the Intent*/

    read(outputPipe, &bal, sizeof(float));
    printf("Your Current Balance is %f\n", bal);
}

void updateBalance(int inputPipe, int outputPipe, int name)
{
    Intent i;
    i.msgId = MSG_BAL_UPDATE;
    i.dataSize = 0;
    float bal = 5.0f;
    char buff[64];

    printf("Please enter the ammount to deposit: ");
    do{
        //fflush(stdout);
        memset(buff, '\0', sizeof buff);
        fgets(buff, 64, stdin);
    }while(strcmp(buff, " \n") <= 0);

    bal = atof(buff);

    i.dataSize = bal;

    write(inputPipe, &i, sizeof i); /*Write the Intent*/

    write(inputPipe, &bal, sizeof bal);

    read(outputPipe, &bal, sizeof(float));

    printf("Your Current Balance is %f\n", bal);
}

void commandDialog(int inputPipe, int outputPipe, int userName)
{

    char stB[64];
    printf("You are now connected to the server, Ctlr+D disconnects\n");

    //memcpy(stB, &i, sizeof i);
    //write(inputPipe, stB, sizeof i)
    Intent i;
    while(1)
    {

        memset(&i,0,sizeof i);
        i.msgId = MSG_EXEC_CMD;
        char buff[1024];
        char null[1];
        null[0] = '\0';
        int asd;
        //scanf("%s", buff);
        do{
            //fflush(stdout);
        	memset(buff, '\0', sizeof buff);
            fgets(buff, 64, stdin);
        }while((asd = strcmp(buff, " \n")) <= 0);


        strtok(buff, "\n");


        if(!strcmp(buff, "MC_QUIT"))
        {
            i.msgId = MSG_MC_CLOSE;
            i.dataSize = -1;
            //memcpy(stB, &i, sizeof i);
            //write(inputPipe, &i, sizeof i);
            return;
        } else{

            i.dataSize = (int) (strlen(buff) + 1);
            //memcpy(stB, &i, sizeof i);

            if(i.msgId != MSG_EXEC_CMD)
                printf("how");
            write(inputPipe, &i.msgId, sizeof i.msgId);
            write(inputPipe, &i.dataSize, sizeof i.dataSize);

            write(inputPipe, buff, i.dataSize);
        }

    }
}
