#include <sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<glob.h>
#include <errno.h>
#include "command.h"

int changeDir(int cursor, Command commands[]);

int displayHelp(int cursor, Command commands[]);

int changePrompt(int cursor, Command commands[]);

int displayCurDir(int cursor, Command commands[]);

int numOfBuiltIns();

int (*builtInsArr[])(int cursor, Command commands[]);

int executeCommand(int numCommands, Command commands[]);

int launchProg(char* file, char** argv, char* stdOutFile, char* stdInFile, char* sep);

void setSignals();

void sigHandler();

int tsCmdSplit(char *inputLine, char *tokens[]);

int pipelineExec(int cursor, Command commands[]);

int waitExec(int cursor, Command commands[]);

int tsCmdSplit(char *inputLine, char *tokens[]);

void shellExit(int code);

//delimiters for parsing,
#define TS_TOK_DELIM " \t\r\n\a"
//buffer size
#define TS_TOK_BUFSIZE 256
//maximum number of commands
#define MAX_NUM_COMMANDS 1000

char *prompt = "$@@> ";

static Command emptyCom;

const char *builtIns[] = {
	"cd",
	"help",
	"prompt",
	"pwd"
};

void shellLoop(void)
{


	int exitFlag;
	int index = 0;
	int numcommands = 0;
	size_t nBytes = TS_TOK_BUFSIZE;
	char *buffer;
	char *tokens[nBytes];
	Command commands[MAX_NUM_COMMANDS];

	buffer = (char *)malloc(nBytes * sizeof(char));

	do{
        index = 0;
		//clean up zombies
		signal(SIGCHLD, sigHandler);

		//prompt line
		printf("%s", prompt);

		//get input
		exitFlag = getline(&buffer, &nBytes, stdin);

		buffer[exitFlag - 1] = '\0';

		if(strcmp(buffer,"exit") == 0)
			break;
		else if(exitFlag == -1)
			printf("Error reading in");

		//split into tokens
		tsCmdSplit(buffer, tokens);

		//fill command struct
		numcommands = separateCommands(tokens, commands);

		//set NULL term for array
		commands[numcommands] = emptyCom;
		//execute commands
		exitFlag = executeCommand(numcommands, commands);

		//empty the separators, redirects for next run
		while(index < numcommands)
		{

			commands[index].sep = NULL;
			commands[index].stdin_file = NULL;
			commands[index].stdout_file = NULL;
			index++;
		}


		exitFlag = 0;
	}while(exitFlag >= 0);

	shellExit(exitFlag);

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
	int i, cursor, returnNum;

	//iterate through all commands
	for(cursor = 0; cursor < numCommands; cursor++)
	{
		//sanity test
		if(commands[cursor].argv[0]== NULL)
    		{
			return 1;
		}
		//if pipeline is present, use pipeline module
		if(strcmp(commands[cursor].sep, "|") == 0)
		{
			return pipelineExec(cursor, commands);
		}
        	//if command is a built in command, execute this
		for(i = 0; i < numOfBuiltIns(); i++)
		{
			if(strcmp(commands[cursor].argv[0], builtIns[i]) == 0)
			{
				return(*builtInsArr[i])(cursor, commands);
			}
		}
		//if >1 command, execute sequential
		if((strcmp(commands[cursor].sep, ";") == 0) && numCommands > 1)
		{
			returnNum = waitExec(cursor, commands);
			cursor++;
		}
		//execute single command, or redirection
		else
		{
			returnNum = launchProg(commands[cursor].argv[0], commands[cursor].argv, commands[cursor].stdout_file, commands[cursor].stdin_file, commands[cursor].sep);
            commands[cursor].stdout_file = NULL;
            commands[cursor].stdin_file = NULL;

		}
	}

	return returnNum;
}


int launchProg(char* file, char** argv, char* stdOutFile, char* stdInFile, char* sep)
{

	pid_t pid, wpid;
	int status, save_in, save_out;
        //init new process
	pid = fork();
	if(pid == 0)
    	{
		    //child process executes
		if(stdOutFile != NULL)
		{
			//get fd for output redirection file
		    int fd = open(stdOutFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);


		    dup2(fd, fileno(stdout));

		    close(fd);
		}

		if(stdInFile != NULL)
		{
		    //get fd for input redirection file
		    int fd = open(stdInFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		    //save stout fd for resetting later

		    //redirect stdout to file

		    dup2(fd, fileno(stdin));
		    close(fd);

		}


            //command is executed
		if(execvp(file, argv) == -1)
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
	    if(strcmp(sep, "&") == 0)
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


int(*builtInsArr[])(int cursor, Command commands[])={
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

int changeDir(int cursor, Command commands[])
{
        //execute 'cd' built in command
	if(commands[cursor].argv[1] == NULL)
    	{
		chdir(getenv("HOME"));
	}
	else
    	{
		if(chdir(commands[cursor].argv[1]) != 0)
		{
				perror("tsh");

		}
	}
	return 1;
}

int displayHelp(int cursor, Command commands[])
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

int changePrompt(int cursor, Command commands[])
{
    if(!(commands[cursor].argv[1] == NULL))
    {
        prompt = commands[cursor].argv[1];

    }

    return 1;
}

int displayCurDir(int cursor, Command commands[])
{
   char pwd[TS_TOK_BUFSIZE];
   if (getcwd(pwd, sizeof(pwd)) != NULL)
   {
       printf("%s\n", pwd);
   }

   else
   {
       perror("getcwd() error");
   }


    return 1;

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

	while(more)
       {
		pid = waitpid(-1, NULL, WNOHANG);
		if(pid <= 0)
			more = 0;
	}

	return;
}

//when pipelining, splits off 2 child processes for write/read ends
int pipelineExec(int cursor, Command commands[])
{
	pid_t write, read;
	pid_t parent;
	int pipefd[2];
	int status;

	//create pipe
	if(pipe(pipefd) <0)
	{
		exit(1);
	}

	if((write = fork()) == 0)
	{
		close(fileno(stdout));
		close(pipefd[0]);
		dup2(pipefd[1], fileno(stdout));
		execvp(commands[cursor].argv[0], commands[cursor].argv);
	}
	if((read = fork()) == 0)
	{
		close(fileno(stdin));
		close(pipefd[1]);
		dup2(pipefd[0], fileno(stdin));
		execvp(commands[cursor+1].argv[0], commands[cursor+1].argv);
		
	}
	else
	{
		close(pipefd[0]);
		close(pipefd[1]);
		while((parent = wait(&status)) > 0)
		{
			//do nothing, just wait for both children to exit
		}
	}

	return 1;
}

int waitExec(int cursor, Command commands[])
{
	int i;

	for(i = 0; i < 2; i++)
	{

		pid_t pid, wpid;
		int status;

		pid = fork();

		if(pid == 0)
		{
			//exec process
			if(execvp(commands[cursor+i].argv[0], commands[cursor+i].argv) == -1)
			{
				perror("lsh");
			}
			exit(EXIT_FAILURE);
		}
		else
		{
			do
			{
				//wait for child
				wpid = waitpid(pid, &status, WUNTRACED);

			}while(!WIFEXITED(status) && !WIFSIGNALED(status));
		}
	}
	return 1;
}

void shellExit(int code)
{
    exit(code);
}

int main(int argc, char **argv)
{
	setSignals();
	shellLoop();

	return EXIT_SUCCESS;
}
