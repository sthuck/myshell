#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "History.h"


#define AC_GREEN   "\x1b[32m"
#define AC_RESET   "\x1b[0m"



void HistoryInit(History* myHistory) {
	int i;
	myHistory->head=-1;
	myHistory->num=0;
	for (i=0;i<NUM_OF_HISTORY;i++)
		myHistory->items[i]=0;
}

void addH(History* myHistory,cmdLine* pCmdLine) {
	char* toAdd=(char*)(malloc(2048));
	*toAdd=0;
	int i;
	for (i=0;i<pCmdLine->argCount;i++) {
		strcat(toAdd,pCmdLine->arguments[i]);
		strcat(toAdd," ");
	}

	myHistory->head=((myHistory->head+1)%NUM_OF_HISTORY);		  /*new head*/
	if (myHistory->num==NUM_OF_HISTORY)               /*we need to delete old*/
	    free(myHistory->items[myHistory->head]);
	else myHistory->num++;

	myHistory->items[myHistory->head]=(char*)(malloc(strlen(toAdd)+1));
	strcpy(myHistory->items[myHistory->head],toAdd);
	free(toAdd);
}

void PrintHistory(History* myHistory) {
	int i;
	if (myHistory->num==NUM_OF_HISTORY)
		for (i=0;i<myHistory->num;i++)
			printf("%d. "AC_GREEN"%s\n"AC_RESET,(abs(NUM_OF_HISTORY-i)-1),myHistory->items[((myHistory->head+i+1)%NUM_OF_HISTORY)]);
	else
		for (i=0;i<myHistory->num;i++)
			printf("%d. "AC_GREEN"%s\n"AC_RESET,(abs(myHistory->head-i)),myHistory->items[i]);
}

void ClearHistory(History* myHistory) {
    int i=0;
    for (i=0;i<NUM_OF_HISTORY;i++)
        free(myHistory->items[i]);
}

