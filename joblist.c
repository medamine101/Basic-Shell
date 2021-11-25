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

typedef struct Job
{

    //Name of job
    char *name;

    //job ID of job
    size_t jobID;

    //process ID of job
    pid_t processID;

    //status, true is running, false is stopped
    bool status;

    //isBackground, true if background process, false if foreground process
    bool isBackground;

} Job;

Job **jobList = NULL;
size_t jobListSize = 0;

char *foregroundJobName = NULL;
size_t foregroundJobID = 0;

size_t terminationSignal = 0;
bool wasKilled = false;

size_t addJobOnList(char *, pid_t, bool, bool);
void removeJobOnList(size_t);
void removeJobOnListUsingProcessID(pid_t);
void ChangeJobStatus(size_t, bool);
void ChangeJobBackground(size_t, bool);
void freeJobList(void);
void setForegroundJob(char *, size_t);
void printTerminationMessage(char *, size_t, pid_t, size_t);
void terminateAllJobs(void);
bool doesJobExist(size_t);

size_t addJobOnList(char *commandName, pid_t processID, bool status, bool runningInBackground)
{

    //Initialize jobList if it does not yet exist
    if (jobList == NULL)
    {
        jobListSize = 1;
        jobList = malloc(sizeof(Job *) * (jobListSize + 1));
        jobList[0] = malloc(sizeof(Job));
        jobList[1] = NULL; //Ensure that the next index is NULL
        jobList[0]->name = malloc(sizeof(char) * (strlen(commandName) + 1));
        strcpy(jobList[0]->name, commandName);
        jobList[0]->jobID = 1;
        jobList[0]->processID = processID;
        jobList[0]->status = status;
        jobList[0]->isBackground = runningInBackground;
        return 1;
    }

    size_t position = 0;

    //Look for first empty slot on list
    while (jobList[position] != NULL)
    {
        position++;
    }
    //printf("%zu", position);

    //If empty slot is at the end of list, list needs to be expanded.
    if (position >= jobListSize)
    {
        jobListSize++;
        jobList = (Job **)realloc(jobList, sizeof(Job *) * (jobListSize + 1));
        jobList[jobListSize] = NULL; //Ensure that the next index is NULL
    }

    //Create new slot for job
    jobList[position] = malloc(sizeof(Job));

    //set name of job
    jobList[position]->name = malloc(strlen(commandName) + 1);
    strcpy(jobList[position]->name, commandName);

    //set job number
    jobList[position]->jobID = position + 1;

    //set job process id
    jobList[position]->processID = processID;

    //set isBackground
    jobList[position]->isBackground = runningInBackground;

    //set status of job as running
    jobList[position]->status = status;

    //printf("%d", jobList[position]->processID);

    return position + 1;
}

void removeJobOnList(size_t jobID)
{

    //print termination message
    if (wasKilled)
        printTerminationMessage(jobList[jobID - 1]->name, jobID, jobList[jobID - 1]->processID, terminationSignal);

    wasKilled = false;

    //free space for name
    free(jobList[jobID - 1]->name);
    jobList[jobID - 1]->name = NULL;

    //free position in jobList
    free(jobList[jobID - 1]);
    jobList[jobID - 1] = NULL;
}

void removeJobOnListUsingProcessID(pid_t id)
{

    for (size_t i = 0; i < jobListSize; i++)
    {

        if (jobList[i] != NULL)
        {

            if (jobList[i]->processID == id)
            {
                removeJobOnList(i + 1);
                break;
            }
        }
    }
}

void ChangeJobStatus(size_t jobID, bool newStatus)
{

    jobList[jobID - 1]->status = newStatus;
}

void ChangeJobBackground(size_t jobID, bool newBackground)
{

    jobList[jobID - 1]->isBackground = newBackground;
}

void freeJobList()
{

    for (size_t i = 0; i < jobListSize; i++)
    {

        if (jobList[i] != NULL)
        {

            free(jobList[i]->name);
            jobList[i]->name = NULL;
            free(jobList[i]);
            jobList[i] = NULL;
        }
    }

    free(jobList);
    jobList = NULL;
}

void setForegroundJob(char *name, size_t processID)
{

    foregroundJobName = realloc(foregroundJobName, sizeof(char) * (strlen(name) + 10));

    strcpy(foregroundJobName, name);

    foregroundJobID = processID;

}

void terminateAllJobs()
{

    for (size_t i = 0; i < jobListSize; i++)
    {

        if (jobList[i] != NULL)
        {

            if (jobList[i]->status == true)
            {

                kill(jobList[i]->processID, SIGHUP);
            }
            else
            {
                kill(jobList[i]->processID, SIGHUP);
                kill(jobList[i]->processID, SIGCONT);
            }
        }
    }
}

bool doesJobExist(size_t jobID)
{

    if (jobList == NULL)
        return false;
    else if (jobID > jobListSize)
        return false;
    else if (jobList[jobID - 1] == NULL)
        return false;
    else
        return true;
}