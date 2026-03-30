#include "farm.h"

// Funzione di gestione dei segnali
void signal_handler(int signum) {
    switch (signum) {
        case SIGUSR1:
            // Invia una notifica al processo Collector per stampare i risultati
            // sino a quel momento
            // Inserire qui la logica per notificare il processo Collector
            break;
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            free(worker);
            exit(EXIT_SUCCESS);
            break;
        default:
            // Ignora gli altri segnali
            break;
    }
}

int main(int argc, char *argv[]) {

    signal(SIGUSR1, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    /* valori di default */
    int n_workers = 4;
    int q_length = 8;
    char *dir_name = NULL;
    char** dir_files = NULL;
    int num_dir_files = 0;
    int delay = 0;
    struct queue *shared_queue;
    int socket_fd = 0;
    struct sockaddr_un server_addr;
    struct sockaddr_un client_addr;
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid_t pid;

    struct mt_args *mt_arguments = malloc(sizeof(struct mt_args));
    struct wt_args *wt_arguments = malloc(sizeof(struct wt_args));
    struct cl_args *cl_arguments = malloc(sizeof(struct cl_args));
    mt_arguments->num_files = 0;

    int optional, skip=0;
    while ((optional = getopt(argc, argv, "n:q:d:t:")) != -1) {
        switch (optional) {
            case 'n':
                n_workers = atoi(optarg);
                skip += 2;
                break;
            case 'q':
                q_length = atoi(optarg);
                skip += 2;
                break;
            case 'd':
                dir_name = optarg;
                dir_files = addFiles(dir_name, &num_dir_files);
                skip += 2;
                break;
            case 't':
                delay = atoi(optarg);
                skip += 2;
                break;
            default:
                fprintf(stderr, "Usage: %s [-n n_workers] [-q query_length] [-d directory_name] [-t delay]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    printf("[DEBUG] epoll_fd %d\n", epoll_fd);
    mt_arguments->epoll_fd = epoll_fd;
    wt_arguments->epoll_fd = epoll_fd;
    cl_arguments->epoll_fd = epoll_fd;
    shared_queue = create_queue(q_length);
    mt_arguments->shared_queue = shared_queue;
    wt_arguments->shared_queue = shared_queue;
    mt_arguments->num_files += argc - skip + num_dir_files;
    mt_arguments->filenames = malloc(sizeof(char *) * mt_arguments->num_files);
    mt_arguments->pipe_fd_read = pipefd[0];
    wt_arguments->pipe_fd_write = pipefd[1];



    /*  Creiamo una socket AF_LOCAL per comunicare con il processo Collector */
    if ((socket_fd =  socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("[DEBUG]Socket_fd: %d\n", socket_fd);

    mt_arguments->socket_fd = socket_fd;
    cl_arguments->socket_fd = socket_fd;

    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, UNIX_PATH_MAX);

    printf("[DEBUG]Socket path: %s\n", server_addr.sun_path);

    unlink(SOCKET_PATH);
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        printf("Failed to bind socket: errno = %d\n", errno);
        perror("bind");
        exit(EXIT_FAILURE);
    }

    mt_arguments->server_addr = server_addr;
    cl_arguments->server_addr = server_addr;



    for(int i = 1; i < (argc - skip); i++){
        mt_arguments->filenames[i] = argv[skip + i];
        printf("[DEBUG]filename[i]: %s\n", mt_arguments->filenames[i]);
    }
    if(num_dir_files != 0) {
        for(int i = 0; i < num_dir_files; i++){
            mt_arguments->filenames[i + (argc - skip)] = dir_files[i];
        }
    }

    /*  Registriamo la pipe all'epoll    */
    struct epoll_event event_pipe_in;
    memset(&event_pipe_in, 0, sizeof(event_pipe_in));
    event_pipe_in.data.fd = pipefd[0];
    event_pipe_in.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipefd[0], &event_pipe_in) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
    struct epoll_event event_pipe_out;
    memset(&event_pipe_out, 0, sizeof(event_pipe_out));
    event_pipe_out.data.fd = pipefd[1];
    event_pipe_out.events = EPOLLOUT;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipefd[1], &event_pipe_out) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event_socket;
    memset(&event_socket, 0, sizeof(event_socket));
    event_socket.events = EPOLLIN | EPOLLOUT;
    event_socket.data.fd = socket_fd;
    //printf("[DEBUG: COLLECTOR] epoll_fd: %d socket_fd: %d\n", epoll_fd, socket_fd);
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event_socket) == -1) {
        perror("epoll_ctl EPOLL_CTL_ADD");
        exit(EXIT_FAILURE);
    }

    pthread_t master, *worker, collector;
    if((worker = malloc(n_workers * sizeof(pthread_t))) == NULL){
        perror("malloc(WORKER THREADS)");
        exit(EXIT_FAILURE);
    }
    pid = fork();
    printf("[DEBUG]pid = %d\n", pid);
    if(pid == 0){
        printf("[DEBUG]pid = %d MasterWorker Creato!\n", pid);

        if(pthread_create(&master, NULL, master_thread, mt_arguments) != 0){
            perror("pthread_create(MASTER)");
            exit(EXIT_FAILURE);
        }

        printf("[DEBUG]pid = %d Master Creato!\n", pid);
        for(int i = 0; i < n_workers; i++){
            if(pthread_create(&worker[i], NULL, worker_thread, wt_arguments) != 0){
                perror("pthread_create(WORKER)");
                exit(EXIT_FAILURE);
            }
            printf("[DEBUG]pid = %d Worker %d Creato!\n", pid, i);
        }
        while (!terminate_program) { // Utilizza la variabile di stato per controllare se terminare il programma
            // ... logica per l'elaborazione dei task ...

            // Legge i segnali in modo bloccante per evitare la CPU polling
            sigset_t sigset;
            sigfillset(&sigset);
            sigdelset(&sigset, SIGUSR1);
            sigdelset(&sigset, SIGINT);
            sigdelset(&sigset, SIGQUIT);
            sigdelset(&sigset, SIGTERM);
            int sig;
            sigwait(&sigset, &sig);
            signal_handler(sig);  // Chiamata esplicita alla funzione di gestione dei segnali
        }
        if (pthread_join(master, NULL) != 0) {
            perror("pthread_join(MASTER)");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < n_workers; i++) {
            if (pthread_join(worker[i], NULL) != 0) {
                printf("Worker = %d \n", i);
                perror("pthread_join(WORKER)");
                exit(EXIT_FAILURE);
            }
        }
        free(worker);
        exit(EXIT_SUCCESS);
    } else if (pid > 0) {

        pid = fork();
        if (pid == 0) {

            if (pthread_create(&collector, NULL, collector_thread, cl_arguments) != 0) {
                perror("pthread_create(COLLECTOR)");
                exit(EXIT_FAILURE);
            }
            printf("[DEBUG]pid = %d Collector Creato!\n", pid);

            if (pthread_join(collector, NULL) != 0) {
                perror("pthread_join(COLLECTOR)");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
        else if (pid > 0) {
            int status;
            waitpid(-1, &status, 0);
            printf("Il processo masterWorker è terminato con stato %d\n", status);
            waitpid(-1, &status, 0);
            printf("Il processo collector è terminato con stato %d\n", status);
            exit(EXIT_SUCCESS);
        } else {
            perror("Errore nella creazione del processo collector");
            exit(EXIT_FAILURE);
        }
    }

    close(pipefd[0]);
    close(pipefd[1]);
    free(dir_files);
    free(mt_arguments->filenames);
    free(mt_arguments);
    return 0;
}
