/*
 * time.c
 *
 * This function is responsible for interpreting user commands alarm, date, and time.
 * For each of the 3 commands, the command is only parsed as long as the command is valid.
 * If a command is found to be invalid at any point, a fail flag is set and returned to application.c.
 *
 * When the program leaves this file it will have the fail flag properly set, as well as the output
 * stored in the output queue for transmission in application.c.
 *
 * A global variable in this file is the *alarm* flag that will be polled while waiting for data from the user.
 *
 *  Created on: Sep 27, 2019
 *      Author: Callum Cottrell
 */

#include "time.h"
#include "cqueue.h"
#include "UART.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Time constants
#define HOURS_TO_TENTHS 36000
#define MIN_TO_TENTHS 600
#define SEC_TO_TENTHS 10
#define DAY_TO_TENTHS 864000
#define MAX_TIME_LENGTH 20

//Date constants
#define NUM_MONTHS 12
#define MONTH_STRING_LENGTH 3
#define MAX_YEAR 9999
#define DAY 24
#define TENTH_SECONDS 10
#define SECONDS_MINUTES 60

int parseCommand(const char* commandConstant);

//Time function prototypes

void printTime();
int parseTime();
int parseAlarm();
//Date function prototypes
void updateDate();
void printDate();
int parseDate();
int monthToInt(char *month);
void printDate();


// Constants
const char *timeConstant = "time";
const char *dateConstant = "date";
const char *alarmConstant = "alarm";
const char* const monthStrings[NUM_MONTHS] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
int monthDays[NUM_MONTHS] = {31,28,31,30,31,30,31,31,30,31,30,31};

//Global Variables
extern unsigned int tenthSecondsCounter;
extern int tenthSecondsAlarm;

//The data structures for holding the time and date
queue *commandQueue;
Time *time;
Date *date;

//Variable for keeping track of day rollover
int daysPassed=0;

/*------------------------Time Functions ------------------------*/
//Parses the input buffer data into the time struct, or performs the time command
int readTime(){

    int fail = TRUE;
    int size = getSize(commandQueue);

    //If the user entered more than 4 characters
    if(size >= 4){

        //dequeues the first 4 bits and validates spelling
        fail = parseCommand(timeConstant);

        //if the input passes so far
        if (!fail){
            //Check if user wants just the time
            if(size == 4){
                //Update time according to the tenths second counter
                updateTime();
                //Store the time to print in the output queue
                char out[MAX_OUTPUT_LENGTH];
                sprintf(out, "\n\r%d:%d:%d.%d", time->hours, time->minutes, time->seconds, time->tenths);
                print(out);

            }
        else {
            //remove the space between command and given time
                dequeue(commandQueue);
                fail = parseTime();

            }
        }
    }
    else
        fail = TRUE;

    return fail;
}


//Checks for alarm, time, and date regardles of capitalization
int parseCommand(const char* commandConstant) {
    int i;
    int commandLength = strlen(commandConstant);
    //Either 4 or 5 depending on the command.
    char compareString[5];
    for (i = 0; i < commandLength; i++)
         compareString[i] = tolower(dequeue(commandQueue));


    // returns TRUE if unequal for fail case
    return memcmp(compareString, commandConstant, commandLength);

}

int parseTime(void){

    unsigned int valid, hours, minutes, seconds, tenths;

    valid = sscanf(&commandQueue->buffer[commandQueue->read],"%d:%d:%d.%d", &hours, &minutes, &seconds, &tenths);

    //Valid will be four if the sscanf was a success.
    if (valid == 4){
        if (hours < DAY)
            time->hours = hours;
        else return TRUE;

        if (minutes < SECONDS_MINUTES)
            time->minutes = minutes;
        else return TRUE;

        if (seconds < SECONDS_MINUTES)
            time->seconds = seconds;
        else return TRUE;

        if (tenths < TENTH_SECONDS)
            time->tenths = tenths;
        else return TRUE;

        }
    else {
        return TRUE;
    }
    //If we reach here, the time was not a fail. Start counting tenths
    tenthSecondsCounter = HOURS_TO_TENTHS*time->hours
        + MIN_TO_TENTHS*time->minutes
        + SEC_TO_TENTHS*time->seconds
        + time->tenths;
    return FALSE;


}

//Use the tenths of a second to increment the timer
// returns the number of days passed
int updateTime(){
    unsigned int newTime;

    //If above 24 hours it's a new day (could even be multiple days)
    while (tenthSecondsCounter >= DAY_TO_TENTHS){
        //excessive counting of days
        tenthSecondsCounter -= DAY_TO_TENTHS;
        daysPassed++;
    }

    //Then add to counter
    newTime = tenthSecondsCounter;

    //Convert back from tenth seconds to time struct
    time->tenths = newTime % TENTH_SECONDS;
    newTime /= TENTH_SECONDS;

    time->seconds = newTime % SECONDS_MINUTES;
    newTime /= SECONDS_MINUTES;

    time->minutes = newTime % SECONDS_MINUTES;
    newTime /= SECONDS_MINUTES;

    time->hours = newTime;

    return daysPassed;
}

/* -------------------- Date Functions -------------------*/

// This function parses the command queue for a date command.
// Returns true if the input was wrong format
int readDate(){

    int fail = TRUE;
    int size = getSize(commandQueue);

    //If the user entered more than 4 characters
    if(size >= 4){

        //dequeues the first 4 bits
        fail = parseCommand(dateConstant);

        //if the input passes so far
        if (!fail){
            //Check if user wants just the time
            if(size == 4){
                //Update date and time according to the tenths second counter
                updateTime();
                updateDate();

                //print the time to the screen by storing in output queue
                char dateString[MAX_OUTPUT_LENGTH];
                sprintf(dateString, "\n\r%d-%s-%d", date->day, monthStrings[date->month], date->year);
                print(dateString);
            }
            else {
                //remove the space between command and given time
                dequeue(commandQueue);
                fail = parseDate();
            }
        }
    }
    else // less than 4 characters  - fail.
        fail = TRUE;

    return fail;
}

//Parses the date given and returns fail (true) if improper input
int parseDate(){
    unsigned int valid, day, year, monthIndex;
    int yearTest;
    char month[MONTH_STRING_LENGTH];

    valid = sscanf(&commandQueue->buffer[commandQueue->read],"%d-%3s-%d", &day, month, &year);
    //Valid will be three if the sscanf was a success.
    if (valid == 3){
        //Convert the month given to an integer for indexing the day count array
        monthIndex = monthToInt(month);
        int check1 = year % 4;
        int check2 = year % 100;
        //If the month string was not a match, exit function
        if (monthIndex < NUM_MONTHS)
            date->month = monthIndex;
        else return TRUE;

        //Check for leap years
        if (!(year % 4)){
                monthDays[1] = 29;
            if (!(year % 400)){
                monthDays[1] = 29;
            }
            else if (!(year % 100)){
                monthDays[1] = 28;
            }
        }
        else monthDays[1] = 28;


        //If the day given fits within the month
        if (day <= monthDays[monthIndex])
            date->day = day;
        else return TRUE;

        if (year <= MAX_YEAR)
            date->year = year;
        else return TRUE;


        }
    //Input incorrect format - fail = true
    else {
        return TRUE;
    }

    //Fail is false if function completes
    return FALSE;


}

//Converts a string from the user to a date
int monthToInt(char *month){
    int i=0;

    //Force the capitalization for the 3 chars - "Jan" format
    month[0] = toupper(month[0]);
    month[1] = tolower(month[1]);
    month[2] = tolower(month[2]);
    i=0;
    //compare the input string against all 3 letter months
    while (i < NUM_MONTHS){
        //If memcmp returns 0 it was a match
        if (!memcmp(month, monthStrings[i], MONTH_STRING_LENGTH)){
            break;
        }
        i++;
    }

    return i;
}

void updateDate(){

    date->day += daysPassed;
    daysPassed = 0;
    if (date->day > monthDays[date->month]){
        date->day %= monthDays[date->month];
        date->month++;
        if (date->month >= NUM_MONTHS){
            date->month = 0;
            date->year++;
            //Check for leap years
            if (!(date->year % 4)){
                monthDays[1] = 29;
                if (!(date->year % 400)){
                 monthDays[1] = 29;
                   }
                 else if (!(date->year % 100)){
                  monthDays[1] = 28;
                   }
                }
            else monthDays[1] = 28;

            if (date->year > MAX_YEAR){
                date->year = 0;
            }
        }

    }

}
/* ---------------------- Alarm Functions ---------------------*/

// Function for parsing an alarm command from the user. Returns true if fail
int readAlarm(){

    int fail = TRUE;
    int size = getSize(commandQueue);

    //If the user entered more than 4 characters
    if(size >= 4){

        //dequeues the first 4 bits
        fail = parseCommand(alarmConstant);

        //if the input passes so far
        if (!fail){
            //Check if user wants just the time
            if(size == 5){
                //Update date according to the tenths second counter
                updateTime();
                tenthSecondsAlarm = -1;
                print("\n\rAlarm cleared");
            }
            else {
                //remove the space between command and given time
                dequeue(commandQueue);
                fail = parseAlarm();
                updateTime();

            }
        }
    }
    else // less than 4 characters  - fail.
        fail = TRUE;

    return fail;
}

void printAlarm(){
    char alarmString[MAX_OUTPUT_LENGTH];
    updateTime();
    sprintf(alarmString, "\n\r \x07 *ALARM* %d:%d:%d.%d \n\r>", time->hours, time->minutes, time->seconds, time->tenths);
    print(alarmString);
    tenthSecondsAlarm = -1; // Set to a time value that can't be hit
}

int parseAlarm(void){

    unsigned int valid, hours, minutes, seconds, tenths, alarmTime;
    char alarmPrint[MAX_OUTPUT_LENGTH];
    valid = sscanf(&commandQueue->buffer[commandQueue->read],"%d:%d:%d.%d", &hours, &minutes, &seconds, &tenths);

    if (valid == 4)
    {

    //Convert the input time to tenth seconds for direct comparison with the counter
    alarmTime = HOURS_TO_TENTHS*hours
       + MIN_TO_TENTHS*minutes
       + SEC_TO_TENTHS*seconds
       + tenths;

    //Set the time for the alarm to go off (in tenth seconds)
    tenthSecondsAlarm = tenthSecondsCounter + alarmTime;
    //If above 24 hours it's a new day
    if (tenthSecondsAlarm >= DAY_TO_TENTHS){
        //excessive counting of days
        tenthSecondsAlarm -= DAY_TO_TENTHS;
    }

    unsigned int tempTime = tenthSecondsAlarm;
    //Convert back from tenths seconds to time for  printing
     tenths = tempTime % TENTH_SECONDS;
     tempTime /= TENTH_SECONDS;

     seconds = tempTime % SECONDS_MINUTES;
     tempTime /= SECONDS_MINUTES;

     minutes = tempTime % SECONDS_MINUTES;
     tempTime /= SECONDS_MINUTES;

     hours = tempTime;

     sprintf(alarmPrint, "\n\rAlarm at %d:%d:%d.%d", hours, minutes, seconds, tenths);
     print(alarmPrint);
    }
    else
        return TRUE;

    return FALSE;

}

void leapYear(){

}



