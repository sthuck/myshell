#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "History.h"
#include "List.h"
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#define NUM_OF_SHELL_CMD 9
#define AC_RED     "\x1b[31m"
#define AC_GREEN   "\x1b[32m"
#define AC_YELLOW  "\x1b[33m"
#define AC_BLUE    "\x1b[34m"
#define AC_MAGENTA "\x1b[35m"
#define AC_CYAN    "\x1b[36m"
#define AC_RESET   "\x1b[0m"

typedef struct shell_cmd {
    char* name;
    int  (*fpt)(cmdLine*);
} shellCmd;

History myHistory;
shellCmd cmdArr[NUM_OF_SHELL_CMD];
Link *shellEnv;
char* cwdbuf;
char* inpbuf;
char* prompt;

void lookupAndRun(cmdLine* pCmdLine ,shellCmd* cmdArr);

int **createPipes(int nPipes) { 
  int** result = (int**)(malloc((nPipes)*sizeof(int*)));
  int i;
  for (i=0;i<nPipes;i++) {
    result[i]=(int*)(malloc(2*sizeof(int))); 
    pipe(result[i]);
  }
  /*result[nPipes]=0;*/
  return result;
}

int NumOfPipes(cmdLine* pCmdLine) {
  int counter=0;
  while (pCmdLine->next) {
    counter++;
    pCmdLine=pCmdLine->next;
  }
  return counter;
}

void releasePipes(int **pipes, int nPipes) {
  int i=0;
  for (i=0;i<nPipes;i++) {
    free(pipes[i]);
  }
  free(pipes);
}

int *feedPipe(int **pipes, cmdLine *pCmdLine,int len) {
  int lengthToEnd=0;
  while (pCmdLine->next) {
    lengthToEnd++;
    pCmdLine=pCmdLine->next;
  }
  int place = len-lengthToEnd-1;
  if (place<0)
    return NULL;
  return pipes[place];
} 

int *sinkPipe(int **pipes, cmdLine *pCmdLine,int len) {
  int lengthToEnd=0;
  while (pCmdLine->next) {
    lengthToEnd++;
    pCmdLine=pCmdLine->next;
  }
  int place = len-lengthToEnd;
  if (place>len-1)
    return NULL;
  return pipes[place];
} 


int quit2(cmdLine* pCmdLine, int code) {
   freeCmdLines(pCmdLine);
   list_free(shellEnv);
   ClearHistory(&myHistory);
   free(inpbuf);
   free(cwdbuf);
   free(prompt);
   _exit(code);
}

int run(cmdLine* pCmdLine, int** pipes,int pipesLen) {
    int* feed=feedPipe(pipes,pCmdLine,pipesLen);
    int* sink=sinkPipe(pipes,pCmdLine,pipesLen);
    int pid = fork();
    if (pid==0) {
        int infd,outfd = -1;
        if (pCmdLine->inputRedirect) {
            close(0);
            infd = open(pCmdLine->inputRedirect,O_RDONLY);
        }

        if (pCmdLine->outputRedirect) {
            close(1);
            outfd = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT,00644) ;
        }
        
        if (sink) {
            close(1);
            dup(sink[1]);
            close(sink[1]);
        }
        if (feed) {
            close(0);
            dup(feed[0]);
            close(feed[0]);
        }

        if (execvp(pCmdLine->arguments[0],pCmdLine->arguments)==-1) {
            char myerror[80]="Execute '";
            strcat(myerror,pCmdLine->arguments[0]);
            strcat(myerror,"'");
            perror(myerror);
            quit2(pCmdLine,-1);
        }
        if (infd != -1) close(infd);
        if (outfd != -1) close(outfd);
   }
     /* Parent code */
       if (sink) close(sink[1]);
       if (feed) close(feed[0]);

       if (pCmdLine->blocking && (pipesLen=0 || pipesLen ==pCmdLine->idx)) {
             int status;
             waitpid(pid,&status,0);
             return status;
        }
       return pid; /* no blocking */

}

int waitForChildren(int* pidTable,int TableSize) {
  int i=0;
  int status;
  for (i=0;i<TableSize;i++) {
    waitpid(pidTable[i],&status,0);
  }
  return status;
}

int execute(cmdLine* pCmdLine) {
    int pipesLen=NumOfPipes(pCmdLine);
    int pidTable[pipesLen+1];
    int** pipes=createPipes(pipesLen);

    /*if (pipesLen) {*/
         int i=0;
         while(pCmdLine) {
          pidTable[i]=run(pCmdLine,pipes,pipesLen);
          i++;
          pCmdLine=pCmdLine->next;
         }
         int status = waitForChildren(pidTable,pipesLen+1);
         releasePipes(pipes,pipesLen);
         return status;
  /*  }
    else {
        return run(pCmdLine,pipes,0);
    } */
  
}

int cd(cmdLine* pCmdLine) {
    int i =chdir(pCmdLine->arguments[1]);
    if (i<0)
        perror("cd");
    return i;
}

int mygecko(cmdLine* pCmdLine) {
    int i;
    int res;
    for (i=1;i<pCmdLine->argCount;++i) {
       res=printf("%s ",pCmdLine->arguments[i]);
       if (res<0) return res;
    }
    res=printf("\n");
    if (res<0) return res; else return 0;
}

int history(cmdLine* pCmdLine) {
    PrintHistory(&myHistory);
    return 0;
}


int quit(cmdLine* pCmdLine) {
    quit2(pCmdLine,0);
    return 0;
}
int redo(cmdLine* pCmdLine){
    int num = atoi((pCmdLine->arguments[0])+1);\
    int command = ((myHistory.head - num)%NUM_OF_HISTORY);
    if (myHistory.items[command]==0) {
        printf("No such command!\n");
        return -1;
    }
    cmdLine* toRun;
    toRun = parseCmdLines(myHistory.items[command]);
    lookupAndRun(toRun,&(cmdArr[0]));
    freeCmdLines(toRun);
    return 0;
}

void replaceVars(cmdLine* pCmdLine) {
    int i;
    for (i=0;i<pCmdLine->argCount;i++) {
        if(strncmp(pCmdLine->arguments[i],"$",1)==0) {
            char* trans = find_data(pCmdLine->arguments[i],shellEnv);
            if (trans) {
                free(pCmdLine->arguments[i]);
                pCmdLine->arguments[i]=trans;
            }
        }
    }
}

void lookupAndRun(cmdLine* pCmdLine ,shellCmd* cmdArr) {
 if(pCmdLine==0 || strlen(pCmdLine->arguments[0])==0)
        return;
 int i;
 int  (*fpt)(cmdLine*) = execute;

 char temp = *(pCmdLine->arguments[0]);
 if (temp==33)
    fpt=redo;
 else {
    addH(&myHistory,pCmdLine);
    add_history(pCmdLine->arguments[0]);
 }
 replaceVars(pCmdLine);
 for (i=1;i<NUM_OF_SHELL_CMD;++i)
     if (strcmp(cmdArr[i].name,pCmdLine->arguments[0])==0)
         fpt=cmdArr[i].fpt;
 fpt(pCmdLine);
}

int set(cmdLine* pCmdLine) {
    if (pCmdLine->argCount<3) {
        printf("Nothing to set!\n");
        return -1;
    }
    char* mydata=(char*)(calloc(strlen(pCmdLine->arguments[1])+2,1));
    strcat(mydata,"$");
    strcat(mydata,pCmdLine->arguments[1]);

    char* trans =  find_data(pCmdLine->arguments[1],shellEnv);
    if (trans==NULL)
        delete_data(mydata,shellEnv);

    list_append(shellEnv,mydata,pCmdLine->arguments[2]);
    free(mydata);
    return 0;
}

int unset(cmdLine* pCmdLine) {
    if (strcmp(pCmdLine->arguments[1],"SHELL")==0) {
        printf("Can't unset $shell var!\n");
        return -1;
    }
    char* mydata=(char*)(calloc(strlen(pCmdLine->arguments[1])+2,1));
    strcat(mydata,"$");
    strcat(mydata,pCmdLine->arguments[1]);
    int res = delete_data(mydata,shellEnv);
    if (res==-1)
        printf("Can't find variable %s!\n",mydata);
    free(mydata);
    return res;
}

int env(cmdLine* pCmdLine) {
    print_list(shellEnv);
    return 0;
}

void init(shellCmd* cmdArr) {
 cmdArr[0].name="execute"; cmdArr[0].fpt=execute;
 cmdArr[1].name="quit"; cmdArr[1].fpt=quit;
 cmdArr[2].name="cd";   cmdArr[2].fpt=cd;
 cmdArr[3].name="history"; cmdArr[3].fpt=history;
 cmdArr[4].name="mygecko"; cmdArr[4].fpt=mygecko;
 cmdArr[5].name="!";       cmdArr[5].fpt=redo;
 cmdArr[6].name="set";     cmdArr[6].fpt=set;
 cmdArr[7].name="unset";   cmdArr[7].fpt=unset;
 cmdArr[8].name="env";     cmdArr[8].fpt=env;
}





int main(int argc, const char *argv[])
{
    /*using_history();*/
    int MAXPATH=40;
    init(cmdArr);
    HistoryInit(&myHistory);
    shellEnv=list_append(shellEnv,"$SHELL","avishell");


     cwdbuf = (char*)(malloc(MAXPATH));
     /*inpbuf = (char*)(malloc(2048));*/
     char* inpbuf;
    cmdLine* toRun; /* = (cmdLine*)(malloc(sizeof(cmdLine)));*/

    while (1)  {
        while (getcwd(cwdbuf,MAXPATH)==0) {
            free(cwdbuf);
            MAXPATH+=15;
            cwdbuf= (char*)(malloc(MAXPATH));
        }
        prompt = (char*)(malloc(strlen(cwdbuf)+15));
        *prompt=0;
        /*printf(AC_CYAN"%s" AC_RED " >>"AC_RESET ,cwdbuf);*/
        strcat(strcat(strcat(strcat(prompt,AC_CYAN),cwdbuf),AC_RED),">> "AC_RESET" ");

        /*fgets(inpbuf,2048,stdin)*/
         inpbuf = readline(prompt);
         if (inpbuf==NULL)
             break;
        toRun = parseCmdLines(inpbuf);
        lookupAndRun(toRun,cmdArr);

        freeCmdLines(toRun);
        free(inpbuf);
        free(prompt);
    }

    return 0;
}