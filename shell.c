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

pid_t pid = 0;

char *foregroundJobName;
size_t foregroundJobID;

bool timeToRemoveJobs = false;
bool timeToAddJobs = false;

void printPrompt(void);
char *getInputLine(void);
void cleanup(void);
void executeCommand(char *);
int runBuiltInCommand(char *);
void runExternalCommand(char *);
char *removeLeadingSpaces(char *);
void removeExcessSpaces(char *);
void freeJobList(void);
void initializeHandlers(void);
size_t addJobOnList(char *, pid_t, bool, bool);
void removeJobOnListUsingProcessID(pid_t);
void terminateAllJobs(void);

int main(int argc, char const *argv[])
{
    //Set signals
    initializeHandlers();

    while (true)
    {

        //Print the terminal prompt
        printPrompt();

        //Take input from user
        char *userCommand = getInputLine();

        if (!strcmp(userCommand, "^D\n"))
        {
            printf("^D\n");
            cleanup();
            break;
        }

        //Remove all repeating spaces
        removeExcessSpaces(userCommand);

        //Remove spaces preceding the command name
        userCommand = removeLeadingSpaces(userCommand);

        //check input to kill shell
        if (!strcmp(userCommand, "exit\n"))
        {
            cleanup();
            free(userCommand);
            userCommand = NULL;
            break;
        }
        else
        {
            executeCommand(userCommand);
        }

        //See flag from signal handlers and act appropriately
        if (timeToAddJobs)
        {
            addJobOnList(foregroundJobName, foregroundJobID, false, false);
            free(foregroundJobName);
            foregroundJobName = NULL;
            foregroundJobID = 0;
            timeToAddJobs = false;
        }
        if (timeToRemoveJobs)
        {

            pid_t pidOfTerminatedJob;

            while (true)
            {

                pidOfTerminatedJob = waitpid(-1, NULL, WNOHANG);

                //If there are no more stopped processes, then break loop
                if ((pidOfTerminatedJob == 0) || (pidOfTerminatedJob == -1))
                    break;

                removeJobOnListUsingProcessID(pidOfTerminatedJob);

                timeToRemoveJobs = false;
            }
        }

        //Free memory at end of loop
        free(userCommand);
        userCommand = NULL;
    }

    return 0;
}

void cleanup()
{

    //terminate all jobs
    terminateAllJobs();

    //free jobList
    freeJobList();

    if (foregroundJobName != NULL)
    {
        free(foregroundJobName);
        foregroundJobName = NULL;
    }
}

void executeCommand(char *command)
{

    //Check if user pressed enter without any input
    if (!strcmp(command, "\n") || (command == NULL))
    {
        return;
    }

    //See if the command is from one of the built-in commands
    if (runBuiltInCommand(command) == 1)
    {

        return;
    }

    runExternalCommand(command);

}