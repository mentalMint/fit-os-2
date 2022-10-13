#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>

long int threads_count;
pthread_t* threads;
struct thread_data* threads_data;

struct thread_data {
    int thread_number;
    double result;
};

void termination_handler() {
    for (int i = 0; i < threads_count; i++) {
        int return_value = pthread_cancel(threads[i]);
        if (0 != return_value) {
            fprintf(stderr, "Error while canceling the thread: %s\n", strerror(return_value));
            pthread_exit((void*) EXIT_FAILURE);
        }
    }

    double pi_div_by_4 = 0;
    for (int i = 0; i < threads_count; i++) {
        int return_value = pthread_join(threads[i], NULL);
        if (0 != return_value) {
            fprintf(stderr, "Cannot join a thread: %s", strerror(return_value));
            pthread_exit((void*) EXIT_FAILURE);
        }
        pi_div_by_4 += threads_data[i].result;
    }
    printf("\nPi = %.15lf\n", pi_div_by_4 * 4);

    free(threads_data);
    free(threads);
    pthread_exit((void*) EXIT_SUCCESS);
}

void calculate_pi_part(struct thread_data* data) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    if (0 != pthread_sigmask(SIG_BLOCK, &set, NULL)) {
        perror("pthread_sigmask");
        pthread_exit((void*) EXIT_FAILURE);
    }

    for (long i = data->thread_number; i * 4 + 3 <= LONG_MAX; i += threads_count) {
        if ((i / threads_count) % 10000000 == 0 && i != data->thread_number) {
            pthread_testcancel();
        }
        data->result += 1.0 / (double) (i * 4 + 1);
        data->result -= 1.0 / (double) (i * 4 + 3);
    }
}

int main(int argc, char* argv[]) {
    if (argv[1] == NULL) {
        threads_count = 1;
    } else {
        threads_count = strtol(argv[1], NULL, 0);
        if (threads_count == LONG_MIN) {
            perror("Error in strtol");
            pthread_exit((void*) EXIT_FAILURE);
        } else if (threads_count <= 0) {
            fprintf(stderr, "Wrong argument. Threads number should be above zero.\n");
            pthread_exit((void*) EXIT_FAILURE);
        }
    }

    threads = (pthread_t*) malloc(threads_count * sizeof(pthread_t));
    threads_data = (struct thread_data*) malloc(threads_count * sizeof(struct thread_data));
    int return_value;
    for (int i = 0; i < threads_count; i++) {
        threads_data[i].thread_number = i;
        return_value = pthread_create(threads + i, NULL, (void*) calculate_pi_part, (void*) &threads_data[i]);
        if (0 != return_value) {
            fprintf(stderr, "Cannot create a thread: %s", strerror(return_value));
            pthread_exit((void*) EXIT_FAILURE);
        }
    }

    struct sigaction new_action;
    new_action.sa_handler = termination_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGINT, &new_action, NULL);

    pause();
    perror("pause");
    pthread_exit((void *)EXIT_FAILURE);
}