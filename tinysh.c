#include <sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

int ts_cd(char **args);

int ts_help(char **args);

int ts_exit(char **args);

int num_natives();

int tsCmdSplit(char *inputLine, char *tokens[]);

int ts_execute(char **args);

#define TOK_DELIM " \t\r\n\a" //delimiters for parsing, whitespace only for now
#define MAX_TOKENS 100

/*void ts_loop(void)
{
	char *line;
	char **args;
	int status;
	//print '$@@>' command line indicator
	//read in command lines, split off arguments and execute command

	do{
		printf("$@@> ");
		line = ts_cmdread();
		args = ts_cmdsplit(line);
		status = ts_execute(args);

		free(line);
		free(args);

	} while(status);
}*/

void ts_loop()
{
	int exitFlag, index = 0; 
	size_t nBytes = MAX_TOKENS;
	char *buffer;
	char *tokens[nBytes];
	
	buffer = (char *)malloc(nBytes * sizeof(char));

	do{
		printf("ict374@@=>");//prompt line

		exitFlag = getline(&buffer, &nBytes, stdin);
		
		buffer[exitFlag - 1] = '\0';
		
		if(strcmp(buffer,"exit") == 0)
			break;
		else if(exitFlag == -1)
			printf("Error reading in");
		
		tsCmdSplit(buffer, tokens);
		
		exitFlag = ts_execute(tokens);
		
		exitFlag = 0;
	}while(exitFlag >= 0);
}

/*char *ts_cmdread(void)
{
	//use getline to read in comand line

	char *line = NULL;
	ssize_t bufsize = 0;//buffer allocated by getline
	getline(&line, &bufsize, stdin);
	return line;
}*/

int tsCmdSplit(char *inputLine, char *tokens[])
{
	char *tokPtr;
	int element = 0;

	tokPtr = strtok(inputLine, TOK_DELIM);

	while(tokPtr != NULL)
	{	
		//break loop if array too small
		if(element > MAX_TOKENS){
			element = -1;
			break;
		}

		tokens[element] = tokPtr;
		element++;
		//to next token
		tokPtr = strtok(NULL, TOK_DELIM);
	}
	//null term token array
	tokens[element] = '\0';

	return element;
}

/*char **ts_cmdsplit(char *line)
{
	//break command line into tokens, using whitespace as delimiter
	int bufsize = TS_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;
	//if memory allocation fails, exit
	if(!tokens){
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}
	//break input into tokens
	token = strtok(line, TS_TOK_DELIM);
	while(token != NULL){
		tokens[position] = token;
		position++;

		//increase buffer size
		if(position >= bufsize){
			bufsize += TS_TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			//error message if no tokenstring
			if(!tokens){
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
				}
		}
		//set token to NULL
		token = strtok(NULL, TS_TOK_DELIM);
	}
	//end cstring with nullptr
	tokens[position] = NULL;
	return tokens;
}*/

//launch processes

int ts_launcher(char **args)
{
	pid_t pid, wpid;
	int status;
	//init new process
	pid = fork();
	if(pid == 0){
		//child process executes
		if(execvp(args[0], args) == -1){
			perror("lsh");
		}
		exit(EXIT_FAILURE);
	}else if(pid < 0){
		//forking error
		perror("lsh");
	}else {
		do{//parent process
			wpid = waitpid(pid, &status, WUNTRACED);
		}while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

char *cmd_str[] = {
	//command names
	"cd",
	"help",
	"exit"
};

int(*native_cmd[])(char**)={
	//pointers to native command functions
	&ts_cd,
	&ts_help,
	&ts_exit
};

int num_natives()
{
	return sizeof(cmd_str)/sizeof(char*);
}

int ts_cd(char **args)
{
	//execute 'cd' built in command
	//does not go back to home directory on 'cd' command, can only use 'cd ..'
	if(args[1] == NULL){
		fprintf(stderr, "tsh: expected arg to \"cd\"\n");
	}else{
		if(chdir(args[1]) != 0){
				perror("tsh");
				
		}
	}
	return 1;
}

int ts_help(char **args)
{
	//prints out built in commands

	int i;
	printf("TinyShell program - ICT374 Assignment 2\n");
	printf("Type args, and hit enter\n");
	printf("Following commands are included in the shell: \n");

	for(i = 0; i < num_natives(); i++){
		printf(" %s\n", cmd_str[i]);
	}

	return 1;
}

int ts_exit(char **args){
	return 0;
}

int ts_execute(char **args)
{
	int i;

	if(args[0] == NULL){
		return 1;
	}
	//if command is a built in command, execute this
	for(i = 0; i < num_natives(); i++){
		if(strcmp(args[0], cmd_str[i]) == 0){
				return(*native_cmd[i])(args);
				
		}
	}
	//else, launch the program
	return ts_launcher(args);
}

int main(int argc, char **argv)
{
	ts_loop();

	return EXIT_SUCCESS;
}
