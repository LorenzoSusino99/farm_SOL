#include "farm.h"

struct result_list;

// Prototipi delle funzioni
void insert_result(struct result_list **head, char *filename, long value);
void free_results(struct result_list *head);
void print_results(struct result_list *head);
void init_list(struct result_list *list);
void* collector_thread(void* arg);
