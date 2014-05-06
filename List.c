#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "List.h"


Link *list_append(Link *list, char* data,char* trans) {
        if (list) {
                Link* last_Link=list;
                while (last_Link->next) {
                        last_Link=last_Link->next;
                }
                last_Link->next=(Link*)(malloc(sizeof(Link)));
                last_Link->next->next=NULL;
                last_Link->next->data=strcpy((char*)(malloc(strlen(data)+1)),data);
                last_Link->next->trans=strcpy((char*)(malloc(strlen(trans)+1)),trans);
        }
        else {  /*list is null*/
                list=(Link*)(malloc(sizeof(Link)));
                list->next=NULL;
                list->data=strcpy((char*)(malloc(strlen(data)+1)),data);
                list->trans=strcpy((char*)(malloc(strlen(trans)+1)),trans);
        }

        return list;
}

void list_free(Link *list) {
        if (list) {
                list_free(list->next);
                free(list->data);
                free(list->trans);
                free(list);
        }
        else {
                return;
        }
}

char* find_data(char* data,Link *list) {
    char* trans = NULL;
        while (list) {
            if (strcmp(data,list->data)==0)
                trans=strcpy((char*)(malloc(strlen(list->trans)+1)),list->trans);
            list=list->next;
        }
    return trans;
}

int delete_data(char* data, Link *list) {

   if (list) {
   Link* last=NULL;
   while (list) {
       if (strcmp(data,list->data)==0) {
           if (last)
               last->next=list->next;
           free(list->data);
           free(list->trans);
           free(list);
           return 0;
       }
       last=list;
       list=list->next;
   }

   }
   return -1;
}


void print_list(Link* list) {
    while(list) {
        printf("%s == %s\n",list->data,list->trans);
        list=list->next;
    }

}
