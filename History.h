#define NUM_OF_HISTORY 10
#include "LineParser.h"

typedef struct History {
	int head; 						/* newest item*/
    int num;						/* number of items stored*/
    char* items[NUM_OF_HISTORY];
} History;

void HistoryInit(History* myHistory);
void addH(History* myHistory,cmdLine* pCmdLine);
void PrintHistory(History* myHistory);
void ClearHistory(History* myHistory);