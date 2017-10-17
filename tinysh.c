#include <sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<signal.h>
#include "command.h"

int changeDir(char **args);

int displayHelp(char **args);

int changePrompt(char **args);

int displayCurDir(char **args);

int numOfBuiltIns();

int (*builtInsArr[])(char*);

char *builtIns[];

int executeCommand(int numCommands, Command commands[]);

int launchProgram(char* args);

void catcher(int signo);

void setSignals();

char *prompt = "$@@";
    //delimiters for parsing,
#define TS_TOK_DELIM " \t\r\n\a"
    //buffer size
#define TS_TOK_BUFSIZE 256

#define MAX_NUM_COMMANDS 1000

void shellLoop(void)
{
	
	int exitFlag;
	int numcommands = 0;	
	size_t nBytes = TS_TOK_BUFSIZE;
	char *buffer;
	char *tokens[nBytes];
	Command commands[MAX_NUM_COMMANDS];
	
	
	buffer = (char *)malloc(nBytes * sizeof(char));

	do{
		printf("%s", prompt);//prompt line

		exitFlag = getline(&buffer, &nBytes, stdin);
		
		buffer[exitFlag - 1] = '\0';
		
		if(strcmp(buffer,"exit") == 0)
			break;
		else if(exitFlag == -1)
			printf("Error reading in");
		
		numcommands = separateCommands(tokens, commands);
		
		//commands[numcommands] = '\0';
		
		exitFlag = executeCommand(numcommands, commands);//this has to change
		
		exitFlag = 0;
	}while(exitFlag >= 0);
	
}

int executeCommand(int numCommands, Command commands[])
{
	int i;
	Command *com = commands;

	if(com->argv[0] == NULL)
    	{
		return 1;
	}
        //if command is a built in command, execute this
	for(i = 0; i < numCommands; i++)
	{
		if(strcmp(com->argv[0], builtIns[i]) == 0)
		{
				return(*builtInsArr[i])(com->argv[0]);

		}
	}
        //else, launch the program
	return launchProgram(com->argv[0]);
}

    //launch processes
int launchProgram(char *args)
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
		exit(EXIT_FAILURE);
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
	"prompt",
	"pwd"
};

int(*builtInsArr[])(char*)={
        //pointers to native command functions
	&changeDir,
	&displayHelp,
	&changePrompt,
	&displayCurDir
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

int changePrompt(char **args)
{
    if(!(args[1] == NULL))
    {
        prompt = args[1];

    }

    return 1;
}

int displayCurDir(char **args)
{
   char pwd[TS_TOK_BUFSIZE];
   if (getcwd(pwd, sizeof(pwd)) != NULL)
   {
       printf("Current working dir: %s\n", pwd);
   }

   else
   {
       perror("getcwd() error");
   }


    return 1;

}

void catcher(int signo)
{
	printf("Signal no %d is caught.\n", signo);


}


void setSignals()
{
    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, SIGINT);
    sigaddset(&s, SIGQUIT);
    sigaddset(&s, SIGTSTP);

    sigprocmask(SIG_SETMASK, &s, NULL);


}
int main(int argc, char **argv)
{
    setSignals();
	shellLoop();

	return EXIT_SUCCESS;
}
