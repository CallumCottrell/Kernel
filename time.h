/*
 * time.h
 *
 *  Created on: Sep 27, 2019
 *      Author: Callum Cottrell
 */

#ifndef TIME_H_
#define TIME_H_

typedef struct DateStruct {
    unsigned int day;
    unsigned int month;
    unsigned int year;
} Date;

typedef struct Time {
    unsigned int tenths;
    unsigned int seconds;
    unsigned int minutes;
    unsigned int hours;
} Time;


extern void getDate(void);

extern int readDate(void);

extern int parseCommand(const char* commandConstant) ;

extern int readTime(void);

extern int readAlarm(void);

extern void printAlarm(void);

#endif /* TIME_H_ */
