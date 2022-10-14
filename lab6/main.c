#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void* sort_by_length(const char* line) {
    if (0 != usleep(strlen(line) * 10000)) {
        perror("Error in usleep");
        return (void *) EXIT_FAILURE;
    }
    printf("%s\n", line);
    return (void *) EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    int threads_count = argc - 1;
    pthread_t* threads = (pthread_t*) malloc(threads_count * sizeof(pthread_t));
    if (threads == NULL) {
        perror("malloc");
        pthread_exit((void*) EXIT_FAILURE);
    }

    int return_value;
    for (int i = 0; i < threads_count; i++) {
        return_value = pthread_create(threads + i, NULL, (void*) sort_by_length, (void*) argv[i + 1]);
        if (0 != return_value) {
            fprintf(stderr, "Cannot create a thread: %s", strerror(return_value));
            pthread_exit((void*) EXIT_FAILURE);
        }
    }

    for (int i = 0; i < threads_count; i++) {
        return_value = pthread_join(threads[i], NULL);
        if (0 != return_value) {
            fprintf(stderr, "Cannot join a thread: %s", strerror(return_value));
            pthread_exit((void*) EXIT_FAILURE);
        }
    }

    free(threads);
    pthread_exit((void*) EXIT_SUCCESS);
}
