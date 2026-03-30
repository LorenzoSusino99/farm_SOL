#include "collector.h"

struct result_list {
    char *filename;
    long value;
    struct result_list *next;
};

void* collector_thread(void* arg){
    struct cl_args *args = (struct cl_args *) arg;
    int epoll_fd = args->epoll_fd, socket_fd = args->socket_fd;
    struct result_list *list;
    if((list = malloc(sizeof(struct result_list))) == NULL){
        perror("malloc list");
        exit(EXIT_FAILURE);
    }
    void init_list(struct result_list *list);

    printf("[DEBUG: COLLECTOR] Inizializzata la lista di risultati\n");

    printf("[DEBUG: COLLECTOR] listen socket_fd %d MAX_EV %d\n", socket_fd, MAX_EV);

    if (listen(socket_fd, MAX_EV) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[DEBUG: COLLECTOR] Mettiamo il file socket in ascolto\n");

    struct epoll_event ev;

    struct epoll_event events[MAX_EV];
    int n_events;
    while (1) {
        /*  Attendo richieste di connessione tramite epoll  */
        n_events = epoll_wait(epoll_fd, events, MAX_EV, -1);
        printf("[DEBUG: COLLECTOR]Il collector ha ricevuto %d eventi\n", n_events);
        if (n_events == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        /*  Il ciclo scansiona tutti gli eventi registrati  */
        for (int i = 0; i < n_events; i++) {
            // Se l'evento riguarda la socket di ascolto
            if (events[i].data.fd == socket_fd) {
                printf("[DEBUG: COLLECTOR]Evento nella socket di ascolto\n");
                // Accetta la connessione
                int conn_fd = accept(socket_fd, NULL, NULL);
                if (conn_fd == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }

                // Aggiunge il file descriptor del client all'epoll set
                ev.events = EPOLLIN;
                ev.data.fd = conn_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1) {
                    perror("epoll_ctl EPOLL_CTL_ADD");
                    exit(EXIT_FAILURE);
                }
            } else { // Altrimenti l'evento riguarda una connessione di un client
                printf("[DEBUG: COLLECTOR]Evento riguarda la connessione di un client\n");
                int conn_fd = events[i].data.fd;

                // Legge i dati dalla socket
                struct result res;
                ssize_t n = read(conn_fd, &res, sizeof(struct result));
                if (n == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                } else if (n == 0) { // Connessione chiusa dal client
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn_fd, NULL) == -1) {
                        perror("epoll_ctl EPOLL_CTL_DEL");
                        exit(EXIT_FAILURE);
                    }
                    close(conn_fd);
                } else { // Dati letti con successo
                    insert_result(&list, strdup(res.filename), res.value);
                }
            }
        }
    }
    print_results(list);
    free_results(list);
    return NULL;
}
/*  Funzione per inserire i risultati ordinatamente in una lista    */
void insert_result(struct result_list **head, char *filename, long value) {
    struct result_list *new_node = malloc(sizeof(struct result_list));
    new_node->filename = strdup(filename);
    new_node->value = value;
    new_node->next = NULL;

    /*  Inserisce il nodo in posizione già ordinata */
    if (*head == NULL || (*head)->value >= value) {
        new_node->next = *head;
        *head = new_node;
    } else {
        struct result_list *current = *head;
        while (current->next != NULL && current->next->value < value) {
            current = current->next;
        }
        new_node->next = current->next;
        current->next = new_node;
    }
}

/*  Funzione per liberare la memoria allocata dalla linked list */
void free_results(struct result_list *head) {
    while (head != NULL) {
        struct result_list *temp = head;
        head = head->next;
        free(temp->filename);
        free(temp);
    }
}

/* Funzione per stampare i risultati ordinati   */
void print_results(struct result_list *head) {
    while (head != NULL) {
        printf("%ld %s\n", head->value, head->filename);
        head = head->next;
    }
}
/* Inizializza la lista */
void init_list(struct result_list *list) {
    list->next = NULL;
    list->filename = NULL;
    list->value = 0;
}
