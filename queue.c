#include "queue.h"

int is_empty(struct queue* q) {
    if(q->size == 0) return 1;
    return 0;
}

int is_full(struct queue* q){
    if(q->max_size == q->size) return 1;
    return 0;
}

struct queue* create_queue(int size) {
    struct queue* q;
    if((q = malloc(sizeof(struct queue))) != NULL) {
        q->max_size = size;
        q->size = 0;
        q->head = NULL;
        q->tail = NULL;
        pthread_cond_init(&q->cond_not_full, NULL);
        pthread_cond_init(&q->cond_not_empty, NULL);
        pthread_mutex_init(&q->lock, NULL);
        return q;
    }
    fprintf(stderr, "Cannot create queue\n");
    perror("create_queue");
    exit(EXIT_FAILURE);
}

void enqueue(struct queue* q, char* data){
  struct queue_node* new;
  if(!(new = malloc(sizeof(struct queue_node)))) {
    fprintf(stderr, "Cannot create new node\n");
    perror("enqueue");
  }
  new->filename = data;
  new->next = NULL;
  pthread_mutex_lock(&q->lock);

  while (is_full(q)) {
    pthread_cond_wait(&q->cond_not_full, &q->lock);
  }

  if (q->head == NULL) {
    q->head = new;
    q->tail = new;
  } else {
    q->tail->next = new;
    q->tail = new;
  }

  q->size++;

  pthread_mutex_unlock(&q->lock);
  pthread_cond_signal(&q->cond_not_empty);
}

char* dequeue(struct queue* q){
  pthread_mutex_lock(&q->lock);

  while (is_empty(q)) {
    pthread_cond_wait(&q->cond_not_empty, &q->lock);
  }

  struct queue_node *tmp = q->head;
  char* data = tmp->filename;
  q->head = tmp->next;
  q->size--;
  free(tmp);

  pthread_mutex_unlock(&q->lock);
  pthread_cond_signal(&q->cond_not_full);

  return data;
}

void destroy_queue(struct queue* q){
  pthread_mutex_lock(&q->lock);

  struct queue_node* current;
  while(!is_empty(q)){
    current = q->head;
    q->head = current->next;
    q->size--;
    free(current);
  }
  q->tail = NULL;
  pthread_mutex_unlock(&q->lock);
  pthread_mutex_destroy(&q->lock);
  pthread_cond_destroy(&q->cond_not_full);
  pthread_cond_destroy(&q->cond_not_empty);
  free(q);
}
