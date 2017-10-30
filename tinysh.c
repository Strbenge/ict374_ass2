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
#include "command.h"

int changeDir(Command commands[]);

int displayHelp(Command commands[]);

int changePrompt(Command commands[]);

int displayCurDir(Command commands[]);

int numOfBuiltIns();

int (*builtInsArr[])(Command commands[]);

char *builtIns[];

int executeCommand(int numCommands, Command commands[]);


int launchGlobProg(char* file, char** argv, char* stdOutFile, char* stdInFile);



void setSignals();

char *prompt = "$@@";

int tsCmdSplit(char *inputLine, char *tokens[]);

void shellExit(int code);




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

		tsCmdSplit(buffer, tokens);

		numcommands = separateCommands(tokens, commands);

		printf("BuiltIn 0 is: %s\n", builtIns[0]);

		//commands[numcommands] = NULL;

		exitFlag = executeCommand(numcommands, commands);

         //reset commands.stdout (this should move into command.c)
        commands[0].stdout_file = NULL;
        commands[0].stdin_file = NULL;

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
	int i, j, matchCount;
	glob_t globBuffer;
    char** argList;
    int numOfArgs = 0;


	if(commands[0].argv[0]== NULL)
    	{
		return 1;
	}

        //if command is a built in command, execute this
	for(i = 0; i < numOfBuiltIns(); i++)
	{

		if(strcmp(commands[0].argv[0], builtIns[i]) == 0)
		{

			return(*builtInsArr[i])(commands);

		}
	}



	return launchProg(commands[0].argv[0], commands[0].argv, commands[0].stdout_file, commands[0].stdin_file);
}




int launchProg(char* file, char** argv, char* stdOutFile, char* stdInFile)
{
    printf("launchGlobProg entered. File: %s\n", file);

	pid_t pid, wpid;
	int status;


        //init new process
	pid = fork();
	if(pid == 0)
    {
            //child process executes
        if(stdOutFile != NULL)
        {
            printf("In if for stdout \n");
                //get fd for output redirection file
            int fd = open(stdOutFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

            dup2(fd, fileno(stdout));

            close(fd);
        }

        if(stdInFile != NULL)
        {
            printf("In if for stdin \n");
            //get fd for output redirection file
            int fd = open(stdInFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            //save stdout fd for resetting later

            //redirect stdout to file
            dup2(fd, fileno(stdin));
            close(fd);
        }



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

        {
            do
            {
                //parent process
                wpid = waitpid(pid, &status, WUNTRACED);

            }while (!WIFEXITED(status) && !WIFSIGNALED(status));

	}



	printf("End of launchProgram\n");
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


void setSignals()
{
    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, SIGINT);
    sigaddset(&s, SIGQUIT);
    sigaddset(&s, SIGTSTP);

    sigprocmask(SIG_SETMASK, &s, NULL);


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
