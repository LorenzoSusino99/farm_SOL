#include "farm.h"

struct queue* create_queue(int size);
void destroy_queue(struct queue* q);
char* dequeue(struct queue* q);
void enqueue(struct queue* q, char* data);
int is_empty(struct queue* q);
int is_full(struct queue* q);
