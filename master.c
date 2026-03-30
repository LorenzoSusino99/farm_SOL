#include "master.h"

void* master_thread(void* args) {
    printf("[DEBUG: MASTER] Ingresso nel master thread\n");
    struct mt_args *arg = (struct mt_args *)args;
    struct result *buf;
    if((buf = malloc(sizeof(struct result *))) == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if((buf->filename = malloc(MAX_FILENAME_LEN)) == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    printf("[DEBUG: MASTER] Buffer Inizializzato\n");
    //printf("[DEBUG: MASTER] event_pipe Inizializzato e aggiunto a epoll_ctl\n");
    //printf("[DEBUG: MASTER] epoll_fd: %d pipe_fd_read %d\n", arg->epoll_fd, arg->pipe_fd_read);

    /*  Impostiamo l'indirizzo della socket */
    struct sockaddr_un addr = arg->server_addr;

    /*  Tentiamo di connetterci alla socket */
    if (connect(arg->socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
/*
      Aggiungiamo la socket all'epoll
    struct epoll_event event_socket;
    event_socket.data.fd = arg->socket_fd;
    event_socket.events = EPOLLOUT;
    if (epoll_ctl(arg->epoll_fd, EPOLL_CTL_ADD, arg->socket_fd, &event_socket) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
*/
    printf("[DEBUG: MASTER]Ingresso nel for di lettura dei filenames\n");
    printf("[DEBUG: MASTER]arg->num_files = % d\n", arg->num_files);
    /*  Controllo se i file sono validi e invio i loro nomi ai worker   */
    for(int i=0; i < arg->num_files; i++){
        if (access(arg->filenames[i], R_OK) == -1) {
            perror("access");
            exit(EXIT_FAILURE);
        }
        struct stat info;
        if ( stat(arg->filenames[i], &info) == -1 ){
            perror("stat");
            exit(EXIT_FAILURE);
        } else if(S_ISREG(info.st_mode)){
#ifdef DEBUG
            printf("[DEBUG: MASTER]File %s scanned successfully\n", arg->filenames[i]);
#endif
            enqueue(arg->shared_queue, arg->filenames[i]);
        } else {
            fprintf(stderr, "%s is neither a file or a directory\n", arg->filenames[i]);
        }
    }

    /*  Creo il loop per le comunicazioni Worker/Master e Master/Collector  */
    int nfds = 0;
    while (1) {
        struct epoll_event events[MAX_EV];
        /* Attendiamo i risultati da parte dei Worker */
        nfds = epoll_wait(arg->epoll_fd, events, MAX_EV, -1);
        if (nfds == -1) {
            /* Verifichiamo se l'errore è dovuto a un interruzione */
            if (errno == EINTR) {
                exit(EXIT_FAILURE);
            } else {
                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }
        }
        printf("[DEBUG: MASTER]Ottenuti risultati da un worker, nfds: %d\n", nfds);
        /*  Verifichiamo la presenza di risultati nella pipe    */
        if (events[0].events & EPOLLIN) {
            int result;
            /*  Leggiamo i dati e li inviamo al processo Collector  */
            while ((result = read(arg->pipe_fd_read, buf, sizeof(buf))) > 0) {

                if (send(arg->socket_fd, buf, result, 0) == -1) {
                    printf("[DEBUG: MASTER]send: %s\n", strerror(errno));
                    perror("send");
                    exit(EXIT_FAILURE);
                }
            }
            if (result == -1) {
                printf("[DEBUG: MASTER]read: %s\n", strerror(errno));
                perror("read");
                exit(EXIT_FAILURE);
            }
        }
    }
    // Chiudi la pipe
    close(arg->pipe_fd_read);

    // Libera la memoria occupata dalla struct struct queuecondivisa
    destroy_queue(arg->shared_queue);

    // Libera la memoria occupata dagli argomenti
    free(arg->filenames);
    free(arg);

    return NULL;
}
