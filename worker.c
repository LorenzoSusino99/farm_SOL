#include "worker.h"

void* worker_thread(void* arg){
    struct wt_args *args = (struct wt_args *) arg;
    char* filename;
    long value = 0;
    FILE *file;
    char* line = NULL;
    ssize_t bytes_written = 0;
    ssize_t total_bytes_written = 0;
    int i = 0;
    int nfds = 0;
    struct result res;
    res.value = 0;
    if((res.filename = malloc(sizeof(char *))) == NULL) {
        perror("malloc filename");
        exit(EXIT_FAILURE);
    }
    if((line = malloc(sizeof(char) * MAX_LINE_LEN)) == NULL) {
        perror("malloc line");
        exit(EXIT_FAILURE);
    }

    /*  Registriamo la pipe all'epoll    */
    struct epoll_event event_ready[1];
    printf("[DEBUG: WORKER]Ingresso while(1) Worker\n");
    while(1){
        /*  Recupero il contenuto dei file e calcolo il risultato
         *  con la formula: result = ∑(i*file[i]) con i={0,...,N-1} */
        if((res.filename = dequeue(args->shared_queue)) == NULL ) {
            perror("dequeue");
            exit(EXIT_FAILURE);
        }
        printf("[DEBUG: WORKER]Recuperato nome file %s\n", res.filename);
        if (access(res.filename, R_OK) == -1) {
            perror("access");
            continue;
        }
        if((file = fopen(res.filename, "r")) == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        /*  Recupero una linea intera dal file, recupero il valore long
         *  e controllo eventuali errori nel formato del file   */
        i = 0;
        while (fgets(line, MAX_LINE_LEN, file) != NULL) {
            char *endptr;
            value = strtol(line, &endptr, 10);
            printf("[DEBUG: WORKER]value = %ld endptr = %s\n", value, endptr);
            if (isspace(*endptr) || *endptr == '\0') { /*Controlla il caso in cui il file non finisca ne con spazio ne con backline*/
                res.value += value * i;
                i++;
            } else {
                fprintf(stderr, "Error: Invalid format in file %s\n", filename);
                exit(EXIT_FAILURE);
            }
        }

        /*  Carico il risultato nella pipe, poiché write può non completare il
         *  caricamento in una singola esecuizione uso un ciclo per controllare
         *  quanti byte son stati scritti e quanti ne rimangono */
        while (total_bytes_written < (sizeof(long) + sizeof(res.filename))) {
            nfds = epoll_wait(args->epoll_fd, event_ready, 1, -1);
            printf("[DEBUG: WORKER]nfds = %d\n", nfds);
            if (nfds == -1) {
                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }
            if (event_ready[0].events & EPOLLOUT) {
                bytes_written = write(args->pipe_fd_write, &res + total_bytes_written, sizeof(res.filename) + sizeof(long) - total_bytes_written);
                if (bytes_written == -1) {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
                total_bytes_written += bytes_written;
                printf("[DEBUG: WORKER]total_bytes_written = %ld bytes_written = %ld\n", total_bytes_written, bytes_written);
            }
        }
    }
    if (epoll_ctl(args->epoll_fd, EPOLL_CTL_DEL, args->pipe_fd_write, NULL) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
    free(line);
    return 0;
}
