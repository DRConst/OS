#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>

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


int main()
{
    printf("pid: %d\n", execStat( "ls | wc -l" ) );
    return 0;
}


// Execs Given StringCommand via auxExec creating a process, returns that process's PID for stats, 0 if strCmd is not valid
pid_t execStat( char *strCmd )
{
    pid_t p;


    if( !strCmd || !strlen( strCmd ) )
        return 0;

    // Hooking for Stats
    if( ( p = fork() ) == 0 ) {	// Son
        auxExec( strCmd );
        exit( EXIT_SUCCESS );
    }else {
        if (p == -1) { // Error
            perror("Error Forking");
            exit(EXIT_FAILURE);

        } else // Parent
            wait(NULL);
    }

    return p;
}


// Auxiliary Functionary parsing strCmd and actually dealing with Cmd/Commands structures
// At this point strCmd has been validated.
void auxExec( char *input )
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

    CmdsExec( cmds );

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
void CmdsExec( Commands cmds )
{
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


            if( execvp( cur->op, cur->args ) < 0 ) {
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