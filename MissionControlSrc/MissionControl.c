#include <asm-generic/errno-base.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include "MissionControl.h"

FILE *fp;


void logMC(char *str) {
    fp = fopen("/tmp/log", "a");
    fwrite(str,strlen(str), 1, fp);;
    fclose(fp);
}

void sigHandler(int s)
{
    if (s == SIGPIPE)
    {
        printf("Something went wrong, restarting\n");
        execlp("Client", "Client", NULL);
        exit(1);
    }
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

    int userHex = 0;
    int login = 0;
    int userSize = 0, pwSize = 0;
    char *userName, *pw;
    ssize_t bytesRead;
    char buff[32];

    signal(SIGPIPE, sigHandler);
    //if(fork() == 0)
    //	execlp("pidstat", "pidstat","-p", "1446", NULL);

    initPipes(&inputPipeFD, &outputPipeFD);
    initAccountingPipes(&accountingInputPipe, &accoutingOutputPipe);

    while(1)
    {
        bytesRead = read(inputPipeFD, &userSize, sizeof userSize);
        sleep(1);

        //printf("asd");
        if(bytesRead > 0)
        {
            userName = malloc(userSize);
            do{
                bytesRead = read(inputPipeFD, userName, userSize);
            }while(bytesRead <= 0);
            logMC("Client Connected\n");

            do{
                bytesRead = read(inputPipeFD, &pwSize, sizeof pwSize);
            }while(bytesRead <= 0);
            if(pwSize)
            {
                pw = malloc(pwSize);
                do{
                    bytesRead = read(inputPipeFD, pw, pwSize);
                }while(bytesRead <= 0);
            }
            else{
                pw = malloc(1);
                pw = "";
            }
            if(fork() == 0)//Is Child
            {
                write(accountingInputPipe, &userSize, sizeof(int)); /*Warn Accounting of New Connection to Fork*/
                write(accountingInputPipe, userName, userSize); /*Warn Accounting of New Connection to Fork*/


                initUserPipes(&inputClientMC, &outputClientMC, &inputClientAcc, &outputClientAcc, &stdinClient, &stdoutClient,
                              userName);

                if(doLogin(userName, pw) != E_AUTH_SUCCSS)
                {
                    printf("Authentication Failed\n");
                    write(outputClientMC, &login, sizeof login);
                    exit(1);
                } else{
                    login = 1;
                    write(outputClientMC, &login, sizeof login);
                }
                clientHandler(inputClientMC, outputClientMC, inputClientAcc, outputClientAcc,accountingInputPipe, accoutingOutputPipe,
                              userName);
            }
            else{
                //exit(1);
            }

        }

    }
    return 0;
}


void clientHandler(int inputClientMC, int outputClientMC, int inputClientAcc, int outputClientAcc, int accountingInputPipe, int accoutingOutputPipe, char *userName)
{
    Intent it;
    int cnt;
    int shouldClose = 0;
    float bal;
    do{
        cnt = read(inputClientMC, &it.msgId, sizeof(it.msgId));
        if(cnt > 0)
        {
            read(inputClientMC, &it.dataSize, sizeof(it.dataSize));
            switch (it.msgId)
            {
                case MSG_EXEC_CMD:
                    execCommand(inputClientMC, outputClientMC, inputClientAcc,outputClientAcc,it.dataSize, userName);
                    break;
                case MSG_BAL_CHECK:
                    bal = balanceCheck(inputClientAcc, outputClientAcc,accountingInputPipe);
                    write(outputClientMC, &bal, sizeof bal);
                    break;
                case MSG_BAL_UPDATE:
                    read(inputClientMC, &bal, sizeof bal);
                    bal = balanceUpdate(inputClientAcc, outputClientAcc,bal);
                    write(outputClientMC, &bal, sizeof bal);
                    break;
                case MSG_MC_CLOSE:
                    logMC("Client Disconnected");
                    printf("User %s has logged out\n", userName);
                    it.msgId = MSG_ACC_DISC;
                    write(outputClientAcc, &it, sizeof it);
                    close(outputClientAcc);

                    close(inputClientAcc);
                    exit(1);/*
                    shouldClose = 1;
                    break;*/
                default:
                    printf("Intent Not Recognized\n");
                    //exit(1);
                    break;
            }
        }
    }
    while(!shouldClose);
}


float balanceCheck(int inputClientAcc, int outputClientAcc, int accountingInputPipe)
{
    /*Talk to Accounting*/
    Intent i;
    i.msgId = MSG_ACC_CHECK;
    float bal;

    write(outputClientAcc, &i, sizeof i);/*Write out Message to Accounting On the Exclusive Pipe*/

    read(inputClientAcc, &bal, sizeof(float));

    return bal;

}

float balanceUpdate(int inputClientAcc, int outputClientAcc, float bal)
{
    /*Talk to Accounting*/
    Intent i;
    i.msgId = MSG_ACC_UPDATE;

    write(outputClientAcc, &i, sizeof i);/*Write out Message to Accounting On the Exclusive Pipe*/

    write(outputClientAcc, &bal, sizeof bal);

    read(inputClientAcc, &bal, sizeof(float));

    return bal;

}

void execCommand(int inputClientMC, int outputClientMC, int inputClientAcc, int outputClientAcc, int dataSize, char *userName)
{
    int killFlag = 0;
    int pid;
    int fd[2];
    char *psRet = malloc(1024);
    char buff[64];
    char *cmdString = malloc(dataSize + 1);
    accIntent ai;
    ai.msgId = MSG_ACC_RUN;
    char *auxStrTok;
    char *ptr;
    char *ctrl;
    float usage;


    do{
        dataSize = read(inputClientMC, cmdString, dataSize);
    }while(dataSize <= 0);

    pid =  execStat(cmdString, userName);

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

void initUserPipes(int *inputClientMC, int *outputClientMC, int *inputClientAcc, int *outputClientAcc, int *stdinClient, int *stdoutClient, char *userName)
{
    char buff[32];
    sprintf(buff , "/tmp/%sInput.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    {
        if(errno != EEXIST){
            printf("Input Pipe Creation Failed\n");
            exit(1);
        }

    }

    *inputClientMC = open(buff , O_RDONLY);/*Will Block Until Client Catches Up*/
    //dup2(STDIN_FILENO, *inputFD);

    sprintf(buff , "/tmp/%sOutput.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *outputClientMC = open(buff , O_WRONLY);/*Will Block Until Client Catches Up*/
    //dup2(*outputClientMC, STDOUT_FILENO);

    sprintf(buff , "/tmp/%sAccountingInput.pipe", userName);


    if(mkfifo(buff, 0666) != 0)
    {
        if(errno != EEXIST){
            printf("Input Pipe Creation Failed\n");
            exit(1);
        }

    }

    *inputClientAcc = open(buff , O_RDONLY);
    //dup2(STDIN_FILENO, *inputFD);

    sprintf(buff , "/tmp/%sAccountingOutput.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *outputClientAcc = open(buff , O_WRONLY);

    sprintf(buff , "/tmp/%sStdin.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }

    *stdinClient = open(buff , O_RDONLY);

    sprintf(buff , "/tmp/%sStdout.pipe", userName);

    if(mkfifo(buff, 0666) != 0)
    if(errno != EEXIST)
    {
        printf("Output Pipe Creation Failed\n");
        exit(1);
    }


    *stdoutClient = open(buff , O_WRONLY);
    //dup2(STDOUT_FILENO, *stdoutClient);
    dup2(*stdoutClient , STDOUT_FILENO);
    dup2(*stdoutClient , STDERR_FILENO);
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



// Execs Given StringCommand via auxExec creating a process, returns that process's PID for stats, 0 if strCmd is not valid
pid_t execStat( char *strCmd, char *user )
{
    pid_t p;


    if( !strCmd || !strlen( strCmd ) )
        return 0;

    // Hooking for Stats
    if( ( p = fork() ) == 0 ) {	// Son
        auxExec(strCmd, user);
        exit( EXIT_SUCCESS );
    }

    return p;
}


// Auxiliary Functionary parsing strCmd and actually dealing with Cmd/Commands structures
// At this point strCmd has been validated.
void auxExec(char *input, char *user)
{
    char *auxStrTok;
    Commands cmds = NULL;
    char *ptr;
    char *ctrl;


//    strcpy( auxStrTok, input );
    auxStrTok = strdup( input );
    ptr = strtok_r( auxStrTok, "|", &ctrl );
    if( ptr != NULL )
        cmds = CmdsInit( ptr );

    ptr = strtok_r( NULL, "|", &ctrl);
    while( ptr != NULL ) {

        CmdsNext( cmds, ptr );

        ptr = strtok_r( NULL, "|", &ctrl);
    }

    free( auxStrTok );

    CmdsExec( cmds, user );

    freeCommands( cmds );
}


// Creates a Cmd(Command) structure with a Given String
Cmd createCmd( char *str )
{
    Cmd cmd;
    char *ptr, temp[100];
    char *ctrl, *ctrl2;
    int argCount = 1;

    cmd = malloc( sizeof( struct cmd ) );


    strcpy( temp, str );

    ptr = strtok_r( str, " ", &ctrl);
    cmd->op = trim( ptr );
    cmd->args = malloc( sizeof(char**));
    cmd->args[0] = malloc( strlen( cmd->op ) + 1 );
    strcpy( cmd->args[0], cmd->op );
    if( ctrl != NULL ) {
        ptr = strtok_r(ctrl, " ", &ctrl2);
        while (ptr != NULL) {
            cmd->args = realloc(cmd->args, (argCount++) * sizeof(char *));
            cmd->args[argCount - 1] = malloc(strlen(ptr) + 1);
            strcpy(cmd->args[argCount - 1], ptr);

            ptr = strtok_r(NULL, " ", &ctrl2);
        }
    }

    cmd->args[ argCount ] = NULL;
    cmd->argCount = argCount;

    return cmd;
}


// Trims whitespaces at the front AND at the end
// e.g. ' wc -r  ' -> 'wc -r'
char * trim( char *in )
{
    int i, len;
    int start, end;
    char *out;


    if( !in || !( len = strlen(in) ) )
        return NULL;


    for( i = 0; (i < len) && isspace( (unsigned char)in[i] ); i++ );
    if( (start = i) == len )
        return NULL;

    for( i = len -  1; (i >= start) && isspace( (unsigned char)in[i] ); i-- );
    if( (end = i) == start )
        return NULL;

    out = malloc( end - start + 1 );

    strncpy( out, in+start, end - start + 1);
    out[ end-start + 1] = '\0';

    return out;
}


// Initializes a Commands Struct with the First Command
Commands CmdsInit( char *first )
{
    Commands cmds;
    Cmd cmdTemp;

    cmdTemp = createCmd( first );
    cmdTemp->pipeOut = -1;
    cmdTemp->pipeIn = -1;

    cmds = malloc( sizeof( struct commands ) );

    cmds->arrCmds = malloc( sizeof( struct cmd ) );
    cmds->arrCmds[0] = cmdTemp;
    cmds->count = 1;

    return cmds;
}


// Inserts New Command into the Valid Given Commands structure
void CmdsNext( Commands cmds, char *str )
{
    Cmd cur = createCmd( str );


    if( !cmds || !(cmds->count) )
        return;

    cur->pipeIn = -1;
    cur->pipeOut = -1;

    cmds->arrCmds = realloc( cmds->arrCmds, sizeof( struct cmd ) * (cmds->count++) );
    cmds->arrCmds[ cmds->count - 1] = cur;


}


// Executes the Commands in a Valid Given Commands structure
// Features: Supports piping output, e.g. ' ls | wc -l'
void CmdsExec( Commands cmds, char *user )
{

    char  usrEnv[strlen(user) + sizeof("LOGNAME=")];
    sprintf(usrEnv, "LOGNAME=%s", user);
    char  usrEnv2[strlen(user) + sizeof("USER=")];
    sprintf(usrEnv2, "USER=%s", user);
    char *envp[] = {usrEnv, usrEnv2, NULL};

    int i;
    Cmd cur, prev, next;

    int pipeFd[2];
    int pid;
    int nPipes;


    if( !cmds || !(cmds->count) )
        return;


    // Building Pipes
    nPipes = cmds->count - 1;
    for( i = 0; i < nPipes; i++ ) {
        pipe( pipeFd );

        cur = cmds->arrCmds[ i ];
        next = cmds->arrCmds[ i+1 ];

        cur->pipeOut = pipeFd[0];
        next->pipeIn = pipeFd[1];

    }


    for( i = 0; i < cmds->count; i++ ) {
        cur = cmds->arrCmds[i];

        if( i )
            prev = cmds->arrCmds[ i - 1];

        if( (i+1) < cmds->count )
            next = cmds->arrCmds[ i + 1];


        if( ( pid = fork() ) == 0 ) {	// Son

            if( i ) {      // Not First
                prev = cmds->arrCmds[ i - 1];
                if (dup2( prev->pipeOut, STDIN_FILENO ) < 0 ) {
                    perror("Error Duplicating InputPipe");
                    exit(EXIT_FAILURE);
                }


            }


            if( (i+1) < cmds->count ) {    // Not Last
                next = cmds->arrCmds[ i + 1 ];
                if (dup2( next->pipeIn, STDOUT_FILENO ) < 0 ) {
                    perror("Error Duplicating OutputPipe");
                    exit(EXIT_FAILURE);
                }


            }

            if( execvpe( cur->op, cur->args, envp ) < 0 ) {
                perror( "Error Executing Command" );
                exit( EXIT_FAILURE );
            }

            exit(EXIT_SUCCESS);
        }
        else if( pid == -1 ) {	// Error Happened
            perror( "Error Forking" );
            exit( EXIT_FAILURE );
        }
        else {		// Parent, Closing used Pipes


            if( i )
                close( prev->pipeOut );

            if( (i+1) < cmds->count )
                close( next->pipeIn );

        }

    }

    // Parent Waits For Children
    for( i = 0; i < cmds->count; i++ )
        wait(NULL);

}


// Frees a Valid Given Commands structure
void freeCommands( Commands cmds )
{
    int i,argCount,j;
    Cmd cur;


    if( !cmds )
        return;

    for( i = 0; i < cmds->count; i++ ) {
        cur = cmds->arrCmds[i];

        argCount = cur->argCount;
        for( j = 0; j < argCount; j++ )
            free( cur->args[j] );

        free( cur->args );

        free( cur->op );

        free( cur );

    }

    free( cmds->arrCmds );
    free( cmds );

}

int doLogin( char *uName, char *pwd )
{
    int ret = 0;
    struct passwd *userPwd;
    struct spwd *saltPwd;
    //struct crypt_data data;
    char *secure;

    if( !uName || !strlen( uName ) )
        return E_AUTH_PARAMS;

    if( !pwd )
        return E_AUTH_PARAMS;


    userPwd = getpwnam( uName );
    if( userPwd == NULL ) {
        perror("Couldn't Access Password");
        return E_AUTH_ACCESS;
    }

    saltPwd = getspnam( uName );
    if( ( saltPwd == NULL ) && ( errno == EACCES ) ) {
        perror( "Permissions Error" );
        return E_AUTH_ACCESS;
    }

    userPwd->pw_passwd = saltPwd->sp_pwdp;

    //data.initialized = 0;

    //secure = crypt_r( pwd, userPwd->pw_passwd, &data );
    //secure = crypt( pwd, userPwd->pw_passwd );
    memset( pwd, '\0', strlen( pwd ) + 1);

    if( secure == NULL ) {
        perror( "Error Encrypting Password" );
        return E_AUTH_FAILED;
    }

    if( strcmp( secure, userPwd->pw_passwd ) == 0 ) {
        perror( "Incorrect Password" );
        return E_AUTH_INCORR;
    }


    return E_AUTH_SUCCSS;
}
