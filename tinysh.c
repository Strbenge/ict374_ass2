#include <sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<signal.h>
#include "command.h"

int changeDir(Command commands[]);

int displayHelp(Command commands[]);

int changePrompt(Command commands[]);

int displayCurDir(Command commands[]);

int numOfBuiltIns();

int (*builtInsArr[])(Command commands[]);

char *builtIns[];

int executeCommand(int numCommands, Command commands[]);

int launchProgram(Command commands[]);

void catcher(int signo);

void setSignals();

void sigHandler();

int tsCmdSplit(char *inputLine, char *tokens[]);

int pipelineExec(Command commands[]);

void writeFork(Command commands[], int pipefd[]);

void readFork(Command commands[], int pipefd[]);

    //delimiters for parsing,
#define TS_TOK_DELIM " \t\r\n\a"
    //buffer size
#define TS_TOK_BUFSIZE 256

#define MAX_NUM_COMMANDS 1000

char *prompt = "$@@";

static Command emptyCom;

void shellLoop(void)
{
	
	int exitFlag, index;
	int numcommands = 0;	
	size_t nBytes = TS_TOK_BUFSIZE;
	char *buffer;
	char *tokens[nBytes];
	Command commands[MAX_NUM_COMMANDS];
		
	buffer = (char *)malloc(nBytes * sizeof(char));

	do{
		//clean up zombies
		signal(SIGCHLD, sigHandler);

		printf("%s", prompt);//prompt line

		exitFlag = getline(&buffer, &nBytes, stdin);
		
		buffer[exitFlag - 1] = '\0';

		if(strcmp(buffer,"exit") == 0)
			break;
		else if(exitFlag == -1)
			printf("Error reading in");

		tsCmdSplit(buffer, tokens);
		
		numcommands = separateCommands(tokens, commands);

		//printf("First command is: %s\n", commands[0].argv[0]);
		//printf("Separator is %s\n", commands[0].sep);
		//printf("Second command is %s\n", commands[1].argv[0]);

		//set NULL term for array
		commands[numcommands] = emptyCom;
		
		exitFlag = executeCommand(numcommands, commands);

		//empty the command structs for next run
		while(index < numcommands)
		{
			commands[index] = emptyCom;
			index++;
		}


		
		exitFlag = 0;
	}while(exitFlag >= 0);
	
}

int tsCmdSplit(char *inputLine, char *tokens[])
{
	char *tokPtr;
	int element = 0;

	tokPtr = strtok(inputLine, TS_TOK_DELIM);

	while(tokPtr != NULL)
	{
		if(element > TS_TOK_BUFSIZE)
		{
			element = -1;
			break;
		}

		tokens[element] = tokPtr;
		element++;
		//to next token
		tokPtr = strtok(NULL, TS_TOK_DELIM);
	}
	//null term tok array
	tokens[element] = '\0';

	return element;
}

					
int executeCommand(int numCommands, Command commands[])
{
	int i;

	if(commands[0].argv[0]== NULL)
    	{
		return 1;
	}

	//if pipeline is present, use pipeline module
	if(strcmp(commands[0].sep, "|") == 0)
	{
		return pipelineExec(commands);
	}

        //if command is a built in command, execute this
	for(i = 0; i < numOfBuiltIns(); i++)
	{
		if(strcmp(commands[0].argv[0], builtIns[i]) == 0)
		{
			return(*builtInsArr[i])(commands);
		}
	}
        //else, launch the program
	return launchProgram(commands);
}

    //launch processes
int launchProgram(Command commands[])
{
	printf("launchProgram entered\n");

	pid_t pid, wpid;
	int status;
        //init new process
	pid = fork();
	if(pid == 0)
    	{
            //child process executes
		if(execvp(commands[0].argv[0], commands[0].argv) == -1)
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
	//parent process
        {	//no wait for child
	    if(strcmp(commands[0].sep, "&") == 0)
	    {
		    wpid = waitpid(pid, &status, WNOHANG);
	    }
	    else
	    {
		//wait for child
            	do
            	{
                
                	wpid = waitpid(pid, &status, WUNTRACED);

            	}while (!WIFEXITED(status) && !WIFSIGNALED(status));
	    }

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

int(*builtInsArr[])(Command commands[])={
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

int changeDir(Command commands[])
{
        //execute 'cd' built in command
        //does not go back to home directory on 'cd' command, can only use 'cd ..'
	if(commands[0].argv[1] == NULL)
    	{
		fprintf(stderr, "tsh: expected arg to \"cd\"\n");
	}
	else
    	{
		if(chdir(commands[0].argv[1]) != 0)
		{
				perror("tsh");

		}
	}
	return 1;
}

int displayHelp(Command commands[])
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

int changePrompt(Command commands[])
{
    if(!(commands[0].argv[1] == NULL))
    {
        prompt = commands[0].argv[1];

    }

    return 1;
}

int displayCurDir(Command commands[])
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

void sigHandler()
{
	int more = 1;
	pid_t pid;
	int status;

	while(more){
		pid = waitpid(-1, NULL, WNOHANG);
		if(pid <= 0)
			more = 0;
	}

	return;
}

//when pipelining, splits off 2 child processes for write/read ends
int pipelineExec(Command commands[])
{
	pid_t write, read;
	pid_t parent;
	int status;
	int pipefd[2];

	//create pipe
	if(pipe(pipefd) <0)
	{
		exit(1);
	}

	//fork children
	write = fork();
	read = fork();

	if(write == 0)
	{
		writeFork(commands, pipefd);
	}
	if(read == 0)
	{
		readFork(commands, pipefd);
	}
	//close parent copy of pipe
	close(pipefd[0]);
	close(pipefd[1]);
	
	while((parent = wait(& status)) > 0)
	{
		if(parent == write)
			printf("Write terminated\n");
		else if(parent == read)
			printf("Read terminated\n");
	}
	return 0;
}

void writeFork(Command commands[], int pipefd[])
{
	close(pipefd[0]);
	dup(pipefd[1]);
	close(pipefd[1]);

	execvp(commands[0].argv[0], commands[0].argv);

	exit(0);
}

void readFork(Command commands[], int pipefd[])
{
	close(pipefd[1]);
	dup(pipefd[0]);
	close(pipefd[0]);

	execvp(commands[1].argv[0], commands[0].argv);

	exit(0);
}

int main(int argc, char **argv)
{
    setSignals();
	shellLoop();

	return EXIT_SUCCESS;
}
