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

const char *defaultWorkingDirectory = "/common/home/";

typedef struct Job
{

    //Name of job
    char *name;

    //job ID of job
    size_t jobID;

    //process ID of job
    pid_t processID;

    //status, 1 is running, 0 is stopped
    bool status;

    //isBackground, true if background process, false if foreground process
    bool isBackground;

} Job;

Job **jobList;
size_t jobListSize;

char *foregroundJobName;
size_t foregroundJobID;

bool wasKilled;
size_t terminationSignal;

int runBuiltInCommand(char *);
void runInBackground(char *);
void changeDirectory(char *);
void runInForeground(char *);
void listJobs(void);
void killProcess(char *);
char *getCommandName(char *);
char *getArgument(char *);
bool isStringEmpty(char *);
size_t addJobOnList(char *, pid_t, bool);
void removeJobOnList(size_t);
void ChangeJobStatus(size_t, bool);
void ChangeJobBackground(size_t, bool);
void setForegroundJob(char *, size_t);
char *removeLeadingPercentSign(char *);
void printTerminationMessage(char *, size_t, pid_t, size_t);
bool doesJobExist(size_t);

int runBuiltInCommand(char *command)
{

    bool commandSuccess = false;

    char *commandName = getCommandName(command);

    char *argument = getArgument(command);

    if (!strcmp(commandName, "bg"))
    { //Check if runInBackground should be run
        runInBackground(argument);
        commandSuccess = true;
    }
    else if (!strcmp(commandName, "cd"))
    { //Check if changeDirectory should be run
        changeDirectory(argument);
        commandSuccess = true;
    }
    else if (!strcmp(commandName, "fg"))
    { //Check if runInForeground should be run
        runInForeground(argument);
        commandSuccess = true;
    }
    else if (!strcmp(commandName, "jobs") || !strcmp(commandName, "jobs\n"))
    { //Check if listJobs should be run
        listJobs();
        commandSuccess = true;
    }
    else if (!strcmp(commandName, "kill"))
    { //Check if killProcess should be run
        killProcess(argument);
        commandSuccess = true;
    }

    //Free pointer
    free(commandName);
    commandName = NULL;
    free(argument);
    argument = NULL;

    //Return whether builtin command worked
    if (commandSuccess)
        return 1;

    return 0;
}

void runInBackground(char *jobID)
{

    //Check if percent sign is there
    if (!(jobID[0] == '%'))
    {
        printf("bg: job not found: %s\n", jobID);
        return;
    }

    char *tempId = removeLeadingPercentSign(jobID);

    //Convert id as string to base 10 number
    size_t id = (size_t)strtoimax(tempId, NULL, 10);

    free(tempId);
    tempId = NULL;

    if (id == 0)
    {
        printf("id cannot be 0, did you type a '%%' before the job id?\n");
        return;
    }

    //Check if job exists
    if (!doesJobExist(id))
    {
        printf("bg: %%%zu: No such job\n", id);
        return;
    }

    //Send signal to suspended process
    kill(jobList[id - 1]->processID, SIGCONT);

    //change isBackground
    ChangeJobStatus(id, true);
    ChangeJobBackground(id, true);
}

void changeDirectory(char *path)
{

    if (isStringEmpty(path))
    {
        chdir(defaultWorkingDirectory);
        return;
    }

    chdir(path);
}

void runInForeground(char *jobID)
{

    if (!(jobID[0] == '%'))
    {
        printf("fg: job not found: %s\n", jobID);
        return;
    }

    char *tempId = removeLeadingPercentSign(jobID);

    //Get JobId from input
    size_t id = (size_t)strtoimax(tempId, NULL, 10);

    free(tempId);
    tempId = NULL;

    if (id == 0)
    {
        printf("id cannot be 0, did you type a '%%' before the job id?\n");
        return;
    }

    //Check if job exists
    if (!doesJobExist(id))
    {
        printf("fg: %%%zu: No such job\n", id);
        return;
    }

    //get pid
    pid_t pidToRun = jobList[id - 1]->processID;

    setForegroundJob(jobList[id - 1]->name, pidToRun);

    ChangeJobStatus(id, true);
    ChangeJobBackground(id, false);

    kill(pidToRun, SIGCONT);

    waitpid(pidToRun, NULL, WUNTRACED);

    free(tempId);
    tempId = NULL;

    if (wasKilled)
    {
        printTerminationMessage(foregroundJobName, id, foregroundJobID, terminationSignal);
        wasKilled = false;
    }

    removeJobOnList(id);
}

void listJobs()
{

    for (size_t i = 0; i < jobListSize; i++)
    {

        if (jobList[i] != NULL)
        {

            printf("[%zu] %d %s %s %s\n", jobList[i]->jobID, jobList[i]->processID, jobList[i]->status ? "Running" : "Stopped", jobList[i]->name, jobList[i]->isBackground ? "&" : "");
        }
    }
}

void killProcess(char *jobID)
{

    if (!(jobID[0] == '%'))
    {
        printf("kill: kill %s failed: operation not permitted\n", jobID);
        return;
    }

    char *tempId = removeLeadingPercentSign(jobID);

    //Convert id as string to base 10 number
    size_t id = (size_t)strtoimax(tempId, NULL, 10);

    free(tempId);
    tempId = NULL;

    if (id == 0)
    {
        printf("id cannot be 0, did you type a '%%' before the job id?\n");
        return;
    }

    //Check if job exists
    if (!doesJobExist(id))
    {
        printf("kill: %%%zu: No such job\n", id);
        return;
    }

    //Send signal to kill process, need to make it continue first
    pid_t pidToKill = jobList[id - 1]->processID;
    kill(pidToKill, SIGCONT);
    int result = kill(pidToKill, SIGTERM);

    wasKilled = true;
    terminationSignal = SIGTERM;

    //Remove job from list
    if (result == 0)
        removeJobOnList(id);
    else
        printf("Error: Could not kill process\n");
}

char *getArgument(char *command)
{

    char *temp = malloc(strlen(command) + 1);
    strcpy(temp, command);

    //Take the command name to eliminate it
    char *commandName = strsep(&temp, " \n&");

    char *argvPtr = commandName;

    argvPtr = strsep(&temp, " \n&");

    char *argToReturn = malloc(strlen(argvPtr) + 1);
    strcpy(argToReturn, argvPtr);

    free(commandName);

    return argToReturn;
}

bool isStringEmpty(char *str)
{

    if (str == NULL)
        return true;

    for (size_t i = 0; i < strlen(str); i++)
    {

        if (!((str[i] == ' ') || (str[i] == '\n') || (str[i] == '\0')))
            return false;
    }

    return true;
}
