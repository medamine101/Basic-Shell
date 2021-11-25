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
#include <sys/wait.h>
#include <errno.h>

pid_t pid;
size_t argvSize = 0;
const char *usrPath = "/user/bin/";
const char *binPath = "/bin/";

bool wasKilled;
size_t terminationSignal;

char *foregroundJobName;
size_t foregroundJobID;

void runExternalCommand(char *);
void runGivenPath(char *, char *);
void searchAndExecute(char *, char *);
void forkAndExecute(char *, char *, char **, char **, bool);
void runExternalInForeground(char *, char **, char **);
void runExternalInBackground(char *, char **, char **);
char *getCommandName(char *);
char **getArguments(char *);
void cleanArguments(char **);
bool checkBackground(char *);
bool checkIfFileExists(const char *);
char *strConcat(char *, char *);
void printErrorCommandNotFound(char *);
void printErrorFileNotFound(char *);
char *removeLeadingSpaces(char *);
size_t addJobOnList(char *, pid_t, bool, bool);
void removeJobOnList(size_t);
char *reduceCommandToNameAndArgs(char *);
void setForegroundJob(char *, pid_t);
void printTerminationMessage(char *, size_t, pid_t, size_t);

void runExternalCommand(char *command)
{

    //Get command name
    char *commandName = getCommandName(command);

    //If command name is a path, execute normally, else, search in /usr/bin and /bin
    if ((commandName[0] == '.') || (commandName[0] == '/'))
        runGivenPath(commandName, command);
    else
        searchAndExecute(commandName, command);
}

void runGivenPath(char *commandName, char *command)
{

    //Print error and return if command does not exit in directory
    if (!checkIfFileExists(commandName))
    {

        printErrorFileNotFound(commandName);
        free(commandName);
        commandName = NULL;
        return;
    }

    //Get arguments as array of strings, first index is always empty string ""
    char **arguments = getArguments(command);

    //Check if ampersand (&) symbol is present in command, set true if & is present
    bool shouldRunInBackground = checkBackground(command);

    //Execute command
    forkAndExecute(command, commandName, arguments, NULL, shouldRunInBackground);

    free(commandName);
    commandName = NULL;
    cleanArguments(arguments);
    free(arguments);
    arguments = NULL;
}

void searchAndExecute(char *commandName, char *command)
{

    char *commandWithUsrPath = strConcat((char *)usrPath, commandName);
    char *commandWithBinPath = strConcat((char *)binPath, commandName);

    if (checkIfFileExists(commandWithUsrPath))
    {
        char **arguments = getArguments(command);
        bool shouldRunInBackground = checkBackground(command);
        forkAndExecute(command, commandWithUsrPath, arguments, NULL, shouldRunInBackground);
        free(commandName);
        commandName = NULL;
        cleanArguments(arguments);
        free(arguments);
        arguments = NULL;
        free(commandWithUsrPath);
        commandWithUsrPath = NULL;
        free(commandWithBinPath);
        commandWithBinPath = NULL;
        return;
    }

    if (checkIfFileExists(commandWithBinPath))
    {
        char **arguments = getArguments(command);
        bool shouldRunInBackground = checkBackground(command);
        forkAndExecute(command, commandWithBinPath, arguments, NULL, shouldRunInBackground);
        free(commandName);
        commandName = NULL;
        cleanArguments(arguments);
        free(arguments);
        arguments = NULL;
        free(commandWithUsrPath);
        commandWithUsrPath = NULL;
        free(commandWithBinPath);
        commandWithBinPath = NULL;
        return;
    }

    //If neither of the above is successful, print error
    printErrorCommandNotFound(commandName);

    free(commandName);
    commandName = NULL;
    free(commandWithUsrPath);
    commandWithUsrPath = NULL;
    free(commandWithBinPath);
    commandWithBinPath = NULL;
}

void forkAndExecute(char *commandName, char *pathName, char **argv, char **envp, bool background)
{
    pid = fork();

    //Only triggered by child process
    if (pid == 0)
    {

        execve(pathName, argv, (char *const *)0);
        printf("Failure: %d\n", errno);
        exit(0);
        return;
    }

    if (background)
    {
        char *commandNameAndArgs = reduceCommandToNameAndArgs(commandName);
        //Add job to list
        size_t potision = addJobOnList(commandNameAndArgs, pid, true, background);
        printf("[%zu] %d\n", potision, pid);
        free(commandNameAndArgs);
        commandNameAndArgs = NULL;
    }
    else
    {
        char *commandNameAndArgs = reduceCommandToNameAndArgs(commandName);
        setForegroundJob(commandNameAndArgs, pid);
        free(commandNameAndArgs);
        commandNameAndArgs = NULL;
    }

    //Wait if process is running in foreground, skip if running in background
    if (!background)
    {
        size_t id = addJobOnList(commandName, pid, false, background);

        pause();
        //Taking process off of joblist is done via handler for SIGCHLD
        if (wasKilled)
        {
            printTerminationMessage(foregroundJobName, id, foregroundJobID, terminationSignal);
            wasKilled = false;
        }

        removeJobOnList(id);
    }
}

char *getCommandName(char *command)
{
    //Make copy of command
    char *temp = malloc(strlen(command) + 1);
    strcpy(temp, command);

    //Take the first word as the command name
    char *commandName = strsep(&temp, " \n&");
    return commandName;
}

char **getArguments(char *command)
{

    //Make copy of command
    char *temp = malloc(strlen(command) + 1);
    strcpy(temp, command);

    char **argv = malloc(sizeof(char *) * 1);

    //Take the command name to eliminate it
    char *commandName = strsep(&temp, " \n&");
    char *argvPtr = commandName;

    argvSize++;
    argv = realloc(argv, sizeof(char *) * (argvSize));
    argv[argvSize - 1] = malloc(strlen(commandName) + 1);
    strcpy(argv[argvSize - 1], commandName);

    while (true)
    {

        argvPtr = strsep(&temp, " \n&");

        if ((argvPtr == NULL) || !(strcmp(argvPtr, "")) || (argvPtr[0] == '\0') || !(strcmp(argvPtr, "&")))
            break;

        argvSize++;
        argv = realloc(argv, sizeof(char *) * (argvSize));
        argv[argvSize - 1] = malloc(strlen(argvPtr) + 1);
        strcpy(argv[argvSize - 1], argvPtr);
    }

    argvSize++;
    argv = realloc(argv, sizeof(char *) * (argvSize));
    argv[argvSize - 1] = NULL;

    free(commandName);
    commandName = NULL;

    return argv;
}

void cleanArguments(char **argv)
{

    for (size_t i = 0; i < argvSize; i++)
    {

        free(argv[i]);
        argv[i] = NULL;
    }

    argvSize = 0;
}

bool checkBackground(char *command)
{

    if (strstr(command, "&"))
        return true;

    return false;
}

bool checkIfFileExists(const char *filename)
{
    struct stat buffer;
    int exist = stat(filename, &buffer);
    if (exist == 0)
        return true;
    else
        return false;
}

char *strConcat(char *s1, char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}