#include "shared.h"

void init_semaphore(Semaphore *sem, int value) {
    sem->count = value;
    pthread_mutex_init(&sem->mutex, NULL);
    pthread_cond_init(&sem->cond, NULL);
}

void wait_semaphore(Semaphore *sem) {
    pthread_mutex_lock(&sem->mutex);
    while (sem->count <= 0)
        pthread_cond_wait(&sem->cond, &sem->mutex);
    sem->count--;
    pthread_mutex_unlock(&sem->mutex);
}

void signal_semaphore(Semaphore *sem) {
    pthread_mutex_lock(&sem->mutex);
    sem->count++;
    pthread_cond_signal(&sem->cond);
    pthread_mutex_unlock(&sem->mutex);
}

void destroy_semaphore(Semaphore *sem) {
    pthread_mutex_destroy(&sem->mutex);
    pthread_cond_destroy(&sem->cond);
}