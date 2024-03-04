// A header file for helpers.c
// Declare any additional functions in this file
#include "linkedlist.h"
#include "icssh.h"
void freeHistory(list_t * list);
void printHistory(list_t * list);
void lineDeleter(void * line1);
void linePrinter(void * line, void * stream);
void terminatePrint(job_info * job, pid_t pid);


void terminateProcess(list_t * list);

void bgentryPrinter(void * bg1, void * stream);
void bgentryDeleter(void * bg);
int bgentryCompatator(const void * bg1, const void * bg2);