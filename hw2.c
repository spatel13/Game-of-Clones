/**
 * @Author Sahil Patel (spatel32@umbc.edu)
 * This file contains a program that generate a fork
 * based on user input. It also has an alternative child
 * mode if a command line arguement is given.
 *
 *
 *https://stackoverflow.com/questions/25703797/trouble-sending-and-handling-signals-with-children-processes-in-c
 * https://stackoverflow.com/questions/903864/how-to-exit-a-child-process-and-return-its-status-from-execvp
 *
 * Question 1: Orphans on a system can take up computer resources that could be vital for other processes 
 * running on the system.
 *
 * Question 2: The second process has 4 file descriptors because it has the original 2 file descriptors from the parent process
 * and 2 file descriptors are from the reading end of the pipe from both processes. Whereas the first child process only has the 
 * file descriptors from the parent process and one file descriptor from its own reading end of the pipe.
 */

#define _POSIX_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int childrenCounter;

struct childProccess
{
  pid_t pid;
  int fd;
  int alive;
};

static void saSigUsr1(int signum)
{
    exit(0);
}

static void saSigUsr2(int signum)
{
    unsigned int randomValue;
    // open dev/urandom
    int randomData = open("/dev/urandom", 0, O_RDONLY);
    // read the first four bytes to an unsigned int
    size_t randomValueSize = read(randomData, &randomValue, sizeof(randomValue));
    // write unsigned int to stdout using write()
    //printf("%u \n", randomValue);
    if( write(1, &randomValue, sizeof(randomValue)) != randomValueSize )
    {
	printf("Something went wrong");
    }
    close(randomData);
}

int main( int argc, char *argv[] )  
{
    if( argc == 1 ) 
    {
	childrenCounter = 0;
	struct childProccess children[100];
	int userInput = 0;
	while( userInput != 3)
	{
	    printf("Main Menu: \n");
	    printf("0. Spawn a child \n");
	    printf("1. Kill a child \n");
	    printf("2. Kill a random child \n");
	    printf("3. End program \n");
	    if( scanf("%d", &userInput) != 1)
	    {
		printf("You picked %d. \n", userInput);
	    }
	    
	    if(userInput == 0)
	    {
		    int fd[2];
		    if(pipe(fd) != 0)
		    {
			fprintf(stderr, "Pipe not created \n");
			exit(EXIT_FAILURE);
		    }
		    pid_t pid = fork();
		    if(pid < 0)
		    {
			fprintf(stderr, "fork failed\n");
			exit(EXIT_FAILURE);
		    }
		    else if(pid == 0)
		    {
			int dupFD = dup2(fd[1], 1);
			if( dupFD == -1)
			    fprintf(stderr, "dup2 failed \n");
			char childrenID[10];
			snprintf(childrenID, sizeof(childrenID), "%d", childrenCounter);
			execlp("./hw2", "./hw2", childrenID, NULL);
		    }
		    children[childrenCounter].pid = pid;
		    children[childrenCounter].fd = fd[0];
		    children[childrenCounter].alive = 1;
		    childrenCounter++;
	    }
	    else if(userInput == 1)
	    {
		int childToKill;
		for(int i = 0; i < childrenCounter; i++)
		{
		    if(children[i].alive == 1)
		    {
			printf("%d: PID %ld \n", i, (long) children[i].pid);
		    } 
		}
		
		if( scanf("%d", &childToKill) == 1)
		{
		    printf("Killed child #%d \n", childToKill); 
		}
		
		kill(children[childToKill].pid, SIGUSR1);
		waitpid(children[childToKill].pid, NULL, 0);
		children[childToKill].alive = 0;
	    }
	    else if(userInput == 2)
	    {
		unsigned int greatestRandomNum = 0;
		pid_t greatestPid = children[0].pid;
		int greatestNum = 0;
		int counter = 0;
		for(int i = 0; i < childrenCounter; i++)
		{
		    if(children[i].alive == 1)
		    {
			unsigned int randomValue;
			kill(children[i].pid, SIGUSR2);
			if(read(children[i].fd, &randomValue, sizeof(randomValue)) > 0)
			{
			    if(counter == 0)
			    {
				greatestRandomNum = randomValue;
				greatestPid = children[i].pid;
				greatestNum = i;
				counter = 1;
			    }
			    else if(randomValue > greatestRandomNum)
			    {
				greatestRandomNum = randomValue;
				greatestPid = children[i].pid;
				greatestNum = i;
			    }
			}
		    }
		}
		kill(greatestPid, SIGUSR1);
		waitpid(greatestPid, NULL, 0);
		children[greatestNum].alive = 0;
		printf("Killed child #%d \n", greatestNum); 
	    }
	    else
	    {
		for(int i = 0; i < childrenCounter; i++)
		{
		    if(children[i].alive == 1)
		    {
			kill(children[i].pid, SIGUSR1);
			waitpid(children[i].pid, NULL, 0);
			children[i].alive = 0;
		    } 
		}
	    }
	}
    }
    else 
    {
	sigset_t mask;
	sigemptyset(&mask);
	struct sigaction sig1 =
	    {
		.sa_handler = saSigUsr1,
		.sa_mask = mask,
		.sa_flags = 0
	    };

	struct sigaction sig2 =
	    {
		.sa_handler = saSigUsr2,
		.sa_mask = mask,
		.sa_flags = 0
	    };
	
	sigaction(SIGUSR1, &sig1, NULL);
	sigaction(SIGUSR2, &sig2, NULL);
	fprintf(stderr, "Spawned child #%s, PID %ld \n", argv[1], (long) getpid());
	while(1)
	{
	    sleep(1);
	}
    }
    
    return 0;
}
