#include "util.h"

char** addFiles(char *dir, int *size) {
    DIR* d = opendir(dir);
    if (!d) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    // Contiamo il numero di file nella directory
    *size = 0;
    struct dirent *file_p;
    while ((file_p = readdir(d)) != NULL) {
        if (file_p->d_type == DT_REG) {
            (*size)++;
        }
    }

    // Allochiamo lo spazio per i nomi dei file
    char **filenames = malloc(*size * sizeof(char *));
    if (!filenames) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Riportiamo il cursore della directory all'inizio
    rewinddir(d);

    // Copiamo i nomi dei file nella matrice
    int i = 0;
    while ((file_p = readdir(d)) != NULL) {
        if (file_p->d_type == DT_REG) {
            filenames[i] = malloc(strlen(file_p->d_name) + 1);
            if (!filenames[i]) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            strncpy(filenames[i], file_p->d_name, strlen(file_p->d_name) + 1);
            i++;
        }
    }

    closedir(d);
    return filenames;
}
