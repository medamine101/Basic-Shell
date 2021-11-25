#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

char *getInputLine(void);
char *removeLeadingSpaces(char *);
char *reduceCommandToNameAndArgs(char *);
void removeExcessSpaces(char *);
char *removeLeadingPercentSign(char *);

//The user input is returned with a newline character at the end of the returned string!
char *getInputLine()
{

    char buffer[2]; //one character and EOF character
    char *stringToReturn = 0;

    size_t currentStringLength = 0;

    //Take characters until reaching newline character
    while (fgets(buffer, sizeof(buffer), stdin))
    {

        //take length of buffer
        size_t bufferLength = strlen(buffer);

        //make intermediate string size of currentString to return plus length of buffer
        char *intermediateStr = (char *)realloc(stringToReturn, bufferLength + currentStringLength + 1);

        //break if string is somehow completely empty
        if (intermediateStr == 0)
            break;

        //make stringToReturn the same as intermediateStr
        stringToReturn = intermediateStr;

        //copy character in buffer into the string to
        strcpy(stringToReturn + currentStringLength, buffer);

        //make the currentStringLength longer by adding length of buffer
        currentStringLength = currentStringLength + bufferLength;

        if (buffer[0] == '\n')
            break;
    }

    if (stringToReturn == NULL)
        return "^D\n";

    return stringToReturn;
}

char *removeLeadingSpaces(char *str)
{
    char *stringToReturn = malloc(sizeof(char) * strlen(str));
    int count = 0, j, k;

    while (str[count] == ' ')
    {
        count++;
    }

    if (count == 0)
    {
        free(stringToReturn);
        return str;
    }

    for (j = count, k = 0;
         str[j] != '\0'; j++, k++)
    {
        stringToReturn[k] = str[j];
    }
    stringToReturn[k] = '\0';

    free(str);
    str = NULL;

    return stringToReturn;
}

void removeExcessSpaces(char *str)
{
    //dest var will be used to iterate through the string
    char *dest = str;

    //loop till the end of the string
    while (*str != '\0')
    {
        //skip spaces
        while (*str == ' ' && *(str + 1) == ' ')
            str++; /* Just skip to next character */

        //copy the character to dest
        *dest++ = *str++;
    }

    // add null terminator
    *dest = '\0';
}

char *reduceCommandToNameAndArgs(char *command)
{

    char *stringToReturn;

    for (size_t i = (strlen(command) - 1); i >= 0; i--)
    {

        if (!(command[i] == '&') && !(command[i] == '\n') && !(command[i] == '\0') && !(command[i] == ' '))
        {

            stringToReturn = malloc(sizeof(char) * (i + 2));
            //stringToReturn = malloc(sizeof(char) * (100 + 1));
            strncpy(stringToReturn, command, i + 1);
            stringToReturn[i + 1] = '\0';
            return stringToReturn;
        }
    }

    return stringToReturn;
}

char *removeLeadingPercentSign(char *str)
{
    char *str1 = malloc(sizeof(char) * strlen(str));
    stpcpy(str1, str + 1);

    return str1;
}