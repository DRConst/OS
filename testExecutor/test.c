#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

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


Cmd createCmd( char *str );
Commands CmdsInit( char *first, int pipeIn, int pipeOut );
void CmdsNext( Commands cmds, char *str );
void CmdsExec( Commands cmds );
void freeCommands( Commands cmds );

char *trim( char *in );
int main()
{
	char input[100] = "ls -l | wc -l | cat";
	char auxStrTok[100] = "";
	Commands cmds;
	char *ptr;
	char *ctrl;


	strcpy( auxStrTok, input );
	ptr = strtok_r( input, "|", &ctrl );
	if( ptr != NULL )
		cmds = CmdsInit( ptr, STDIN_FILENO, STDOUT_FILENO );

	ptr = strtok_r( NULL, "|", &ctrl);
	while( ptr != NULL ) {

		CmdsNext( cmds, ptr );

		ptr = strtok_r( NULL, "|", &ctrl);
	}


	CmdsExec( cmds );

//	freeCommands( cmds );

	return 0;
}

Commands CmdsInit( char *first, int pipeIn, int pipeOut )
{
	Commands cmds;
	Cmd cmdTemp = createCmd( first );

	cmdTemp->pipeIn = pipeIn;
	cmdTemp->pipeOut = pipeOut;

	cmds = malloc( sizeof( struct commands ) );

	cmds->arrCmds = malloc( sizeof( struct cmd ) );
	cmds->arrCmds[0] = cmdTemp;
	cmds->count = 1;

	return cmds;
}


void CmdsNext( Commands cmds, char *str )
{
	Cmd cur = createCmd( str );
	Cmd prev;
	int pipeFd[2];


	if( !cmds || !(cmds->count) )
		return;

	prev = cmds->arrCmds[ cmds->count - 1];

	cur->pipeOut = prev->pipeOut;

	pipe( pipeFd );

	prev->pipeOut = pipeFd[1];
	cur->pipeIn = pipeFd[0];


	cmds->arrCmds = realloc( cmds->arrCmds, sizeof( Cmd ) * (cmds->count++) );
	cmds->arrCmds[ cmds->count - 1] = cur;


}


void CmdsExec( Commands cmds )
{
	int i;
	Cmd cur, prev;
	int pipeIn, pipeOut;
	int pipeFd[2];

	if( !cmds || !(cmds->count) )
		return;

	for( i = 0; i < cmds->count; i++ ) {
		cur = cmds->arrCmds[i];

		if( fork() == 0 ) {	// Son

			if( i ) {        // Not First
				dup2(prev->pipeIn, 0);
				close(prev->pipeIn);
				close(prev->pipeOut);
			}

			if( (i+1) < cmds->count ) {    // not Last
				close( cur->pipeIn );
				dup2( cur->pipeOut, 1 );
				close( cur->pipeOut );
			}

			execvp( cur->op, cur->args );
			exit(EXIT_FAILURE);
		}
		else {	// Parent

//			close( cur->pipeOut );
			if( i ) {
				close(prev->pipeOut);
				close(prev->pipeIn);
			}

			if( i+1 < cmds->count)
				prev = cur;
			wait( NULL );

		}

	}

}


void freeCommands( Commands cmds )
{
	int i;

	for( i = cmds->count - 1; i >= 0 ; i-- ) {

		free( cmds->arrCmds[i]->op );
		free( cmds->arrCmds[i]->args );
		free( cmds->arrCmds[i] );

	}

	free( cmds->arrCmds );
	free( cmds );

}

Cmd createCmd( char *str )
{
	Cmd cmd;
	char *ptr, temp[100];
	char *ctrl, *ctrl2;
	int argCount = 0;

	cmd = malloc( sizeof( Cmd ) );


	strcpy( temp, str );

	ptr = strtok_r( str, " ", &ctrl);
	cmd->op = trim( ptr );

//	ptr = strchr( temp, ' ');
	if( ctrl != NULL ) {
		ptr = strtok_r(ctrl, " ", &ctrl2);
		while (ptr != NULL) {
			cmd->args = realloc(cmd->args, (argCount++) * sizeof(char *));
			cmd->args[argCount - 1] = malloc(strlen(ptr) + 1);
			strcpy(cmd->args[argCount - 1], ptr);

			ptr = strtok_r(NULL, " ", &ctrl2);
		}
	}

	cmd->argCount = argCount;

	return cmd;
}

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

/*
int main( int argc, char **argv )
{
	int pipefd[2];
	Cmd *cmds;
	Cmd temp;
	int nCmds, sizeCmds;

	int i;


	if( argc == 1 )
		return 0;

	nCmds = 1;
	printf("hai");
	sizeCmds = sizeof( struct cmd );

	cmds = malloc( sizeCmds );

	printf("hai");
	for( i = 0; i < argc; i++ )
	{
		printf("hai");
		if( strcmp( argv[i], "|" ) == 0 ) {

			nCmds++;
			sizeCmds += sizeof( struct cmd );
			cmds = realloc( cmds, sizeCmds );

			temp = cmds[ nCmds - 1 ];
			temp->op = malloc( strlen( argv[i+1] ) );
			strcpy( temp->op, argv[i+1] );

		}
		else {

			temp = cmds[ nCmds - 1 ];
			temp->args = realloc( temp->args,
								  strlen( temp->args ) - 1 + strlen( argv[i] ) );
			strcat( temp->args, argv[i] );

		}

	}

	printf( "nCmds: %d", nCmds );

	return 0;
}*/