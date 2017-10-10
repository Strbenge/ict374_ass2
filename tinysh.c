#include <sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

int changeDir(char **args);

int displayHelp(char **args);

int shellExit(char **args);

int numOfBuiltIns();

char *readFromCMD(void);

char **ts_cmdsplit(char *line);

int executeCommand(char **args);

    //delimiters for parsing, whitespace only for now
#define TS_TOK_DELIM " \t\r\n\a"
    //buffer size
#define TS_TOK_BUFSIZE 256



void shellLoop(void)
{
	char *line;
	char **args;
	int status;
        //print '$@@>' command line indicator
        //read in command lines, split off arguments and execute command

	do
    {
		printf("$@@> ");
		line = readFromCMD();
		args = ts_cmdsplit(line);
		status = executeCommand(args);

		free(line);
		free(args);

	} while(status);
}

char *readFromCMD(void)
{
        //use getline to read in comand line

	char *line = NULL;
        //buffer allocated by getline
	ssize_t bufsize = 0;

	getline(&line, &bufsize, stdin);

	return line;
}

char **ts_cmdsplit(char *line)
{
        //break command line into tokens, using whitespace as delimiter
	int bufsize = TS_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;
        //if memory allocation fails, exit
	if(!tokens)
    {
		fprintf(stderr, "lsh: allocation error\n");
		shellExit(EXIT_FAILURE);
	}
        //break input into tokens
	token = strtok(line, TS_TOK_DELIM);
	while(token != NULL)
	{
		tokens[position] = token;
		position++;

            //increase buffer size
		if(position >= bufsize)
        {
			bufsize += TS_TOK_BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));
                //error message if no tokenstring
			if(!tokens)
			{
				fprintf(stderr, "lsh: allocation error\n");
				shellExit(EXIT_FAILURE);
            }
		}
            //set token to NULL
		token = strtok(NULL, TS_TOK_DELIM);
	}

        //end cstring with nullptr
	tokens[position] = NULL;
	return tokens;
}

    //launch processes

int launchProgram(char **args)
{
	pid_t pid, wpid;
	int status;
        //init new process
	pid = fork();
	if(pid == 0)
    {
            //child process executes
		if(execvp(args[0], args) == -1)
		{
			perror("lsh");
		}
		shellExit(EXIT_FAILURE);
	}
	else
        if(pid < 0)
        {
                //forking error
            perror("lsh");

        }
        else

        {
            do
            {
                //parent process
                wpid = waitpid(pid, &status, WUNTRACED);

            }while (!WIFEXITED(status) && !WIFSIGNALED(status));

	}
	return 1;
}

char *builtIns[] = {
            //command names
	"cd",
	"help",
	"exit"
};

int(*builtInsArr[])(char**)={
        //pointers to native command functions
	&changeDir,
	&displayHelp,
	&exit
};

int numOfBuiltIns()
{
	return sizeof(builtIns)/sizeof(char*);
}

int changeDir(char **args)
{
        //execute 'cd' built in command
        //does not go back to home directory on 'cd' command, can only use 'cd ..'
	if(args[1] == NULL)
    {
		fprintf(stderr, "tsh: expected arg to \"cd\"\n");
	}
	else
    {
		if(chdir(args[1]) != 0)
		{
				perror("tsh");

		}
	}
	return 1;
}

int displayHelp(char **args)
{
        //prints out built in commands

	int i;
	printf("TinyShell program - ICT374 Assignment 2\n");
	printf("Type args, and hit enter\n");
	printf("Following commands are included in the shell: \n");

	for(i = 0; i < numOfBuiltIns(); i++)
    {
		printf(" %s\n", builtIns[i]);
	}

	return 1;
}

int shellExit(char **args)
{
	return 0;
}

int executeCommand(char **args)
{
	int i;

	if(args[0] == NULL)
    {
		return 1;
	}
        //if command is a built in command, execute this
	for(i = 0; i < numOfBuiltIns(); i++)
	{
		if(strcmp(args[0], builtIns[i]) == 0)
		{
				return(*builtInsArr[i])(args);

		}
	}
        //else, launch the program
	return launchProgram(args);
}

int main(int argc, char **argv)
{
	shellLoop();

	return EXIT_SUCCESS;
}
