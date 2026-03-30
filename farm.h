#ifndef FARM_H
#define FARM_H
#define DEBUG 1

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <ctype.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include "master.h"
#include "worker.h"
#include "collector.h"
#include "queue.h"
#include "util.h"

//#define SOCKET_PATH "/mnt/c/Users/loren/SOL/Laboratorio/Progetto/farmSck.sck"
#define SOCKET_PATH "./farmSck.sck"
#define MAX_EV 1
#define UNIX_PATH_MAX 108

struct queue_node {
    char* filename;
    struct queue_node* next;
};

struct queue {
  struct queue_node* head;
  struct queue_node* tail;
  pthread_mutex_t lock;
  pthread_cond_t cond_not_full;
  pthread_cond_t cond_not_empty;
  int size;
  int max_size;
};

struct mt_args{
    char** filenames;
    int pipe_fd_read;
    int num_files;
    struct queue* shared_queue;
    int collector_port;
    int socket_fd;
    int epoll_fd;
    struct sockaddr_un server_addr;
    struct sockaddr_un client_addr;
};

struct wt_args{
    int pipe_fd_write;
    struct queue* shared_queue;
    int epoll_fd;
};

struct cl_args{
    int socket_fd;
    struct sockaddr_un server_addr;
    struct sockaddr_un client_addr;
    int epoll_fd;
};

struct result {
    char* filename;
    long value;
};



#endif
