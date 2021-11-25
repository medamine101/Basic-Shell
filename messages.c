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
#include <inttypes.h>
#include <signal.h>
#include <sys/wait.h>

const char errorMessageFileNotFound[] = ": No such file or directory";
const char errorMessageCommandNotFound[] = ": command not found";
const char terminationMessage[] = "terminated by signal";

char *strConcat(char *, char *);
void printPrompt(void);
void printErrorCommandNotFound(char *);
void printErrorFileNotFound(char *);
void printTerminationMessage(char *, size_t, pid_t, size_t);

void printPrompt()
{

    printf("> ");
    fflush(stdout);
}

void printErrorCommandNotFound(char *commandName)
{

    char *errorMessage = strConcat(commandName, (char *)errorMessageCommandNotFound);
    printf("%s\n", errorMessage);
    free(errorMessage);
    errorMessage = NULL;
}

void printErrorFileNotFound(char *commandName)
{

    char *errorMessage = strConcat(commandName, (char *)errorMessageFileNotFound);
    printf("%s\n", errorMessage);
    free(errorMessage);
    errorMessage = NULL;
}

void printTerminationMessage(char *commandName, size_t jobID, pid_t pid, size_t signalNumber)
{

    printf("[%zu] %d %s %zu\n", jobID, pid, terminationMessage, signalNumber);
}