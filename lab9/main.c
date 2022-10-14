#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>

long int threads_count;
pthread_t* threads;
struct thread_data* threads_data;
bool is_time_to_stop = false;
sigset_t mask;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct thread_data {
    int thread_number;
};

double* calculate_pi_part(struct thread_data* data) {
    double* pi_part = (double*) malloc(sizeof(double));
    for (long i = data->thread_number; i * 4 + 3 <= LONG_MAX; i += threads_count) {
        if (((i - data->thread_number)  / threads_count) % 1000000 == 0 && i != data->thread_number) {
            pthread_mutex_lock(&lock);
            if (is_time_to_stop) {
                pthread_mutex_unlock(&lock);
                return pi_part;
            }
            pthread_mutex_unlock(&lock);
        }
        *pi_part += 1.0 / (double) (i * 4 + 1);
        *pi_part -= 1.0 / (double) (i * 4 + 3);
    }
    return pi_part;
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

    sigset_t oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    int return_value;
    if ((return_value = pthread_sigmask(SIG_BLOCK, &mask, &oldmask)) != 0) {
        fprintf(stderr, "pthread_sigmask SIG_BLOCK: %s", strerror(return_value));
        pthread_exit((void*) EXIT_FAILURE);
    }


    threads = (pthread_t*) malloc(threads_count * sizeof(pthread_t));
    if (threads == NULL) {
        perror("threads malloc");
        pthread_exit((void*) EXIT_FAILURE);
    }

    threads_data = (struct thread_data*) malloc(threads_count * sizeof(struct thread_data));
    if (threads_data == NULL) {
        perror("threads_data malloc");
        pthread_exit((void*) EXIT_FAILURE);
    }

    for (int i = 0; i < threads_count; i++) {
        threads_data[i].thread_number = i;
        return_value = pthread_create(threads + i, NULL, (void* (*)(void*)) calculate_pi_part, (void*) &threads_data[i]);
        if (0 != return_value) {
            fprintf(stderr, "Cannot create a thread: %s", strerror(return_value));
            pthread_exit((void*) EXIT_FAILURE);
        }
    }

    int sig_number;
    while (true) {
        return_value = sigwait(&mask, &sig_number);
        if (return_value != 0)
            perror("sigwait");
        if (sig_number == SIGINT) {
            pthread_mutex_lock(&lock);
            is_time_to_stop = true;
            pthread_mutex_unlock(&lock);
            break;
        }
    }

    double pi_div_by_4 = 0;
    double* pi_div_by_4_part = NULL;
    for (int i = 0; i < threads_count; i++) {
        return_value = pthread_join(threads[i], (void*) &pi_div_by_4_part);
        if (0 != return_value) {
            fprintf(stderr, "Cannot join a thread: %s", strerror(return_value));
            pthread_exit((void*) EXIT_FAILURE);
        }
        pi_div_by_4 += *pi_div_by_4_part;
        free(pi_div_by_4_part);
    }
    printf("\nPi = %.15lf\n", pi_div_by_4 * 4);

    free(threads_data);
    free(threads);
    pthread_exit((void*) EXIT_SUCCESS);
}