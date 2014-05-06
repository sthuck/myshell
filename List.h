
typedef struct Link {
     struct  Link* next;
      char* data;
      char* trans;
} Link;

Link *list_append(Link *list, char* data,char* trans);

void list_free(Link *list);

char* find_data(char* data,Link *list);
int delete_data(char* data, Link *list);
void print_list(Link* list);

