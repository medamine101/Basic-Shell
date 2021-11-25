#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <time.h>
#include <grp.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

void initializeHandlers(void);
void sigintHandler(int);
void sigtstpHandler(int);
void sigchildHandler(int);

bool timeToRemoveJobs;
bool timeToAddJobs;

bool wasKilled;
size_t terminationSignal;

void initializeHandlers()
{
    if (signal(SIGINT, sigintHandler) == SIG_ERR)
    {
        perror("Error setting SIGINT handler");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGTSTP, sigtstpHandler) == SIG_ERR)
    {
        perror("Error setting SIGSTP handler");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGCHLD, sigchildHandler) == SIG_ERR)
    {
        perror("Error setting SIGCHLD handler");
        exit(EXIT_FAILURE);
    }
}

void sigintHandler(int intSig)
{
    signal(SIGINT, sigintHandler);

    timeToRemoveJobs = true;

    wasKilled = true;
    terminationSignal = SIGINT;

    write(STDOUT_FILENO, "\n", 1);

    fflush(stdout);
}

void sigtstpHandler(int stpSig)
{

    signal(SIGTSTP, sigtstpHandler);

    timeToAddJobs = true;

    write(STDOUT_FILENO, "\n", 1);

    fflush(stdout);
}

void sigchildHandler(int chldSig)
{

    signal(SIGCHLD, sigchildHandler);

    timeToRemoveJobs = true;
}