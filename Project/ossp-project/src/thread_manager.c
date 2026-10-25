#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "thread_manager.h"

static int g_shared_counter = 0;

static void *thread_worker(void *arg) {
    (void)arg;
    printf("[thread] worker thread started on pthread ID %lu\n", (unsigned long)pthread_self());
    return NULL;
}

int cmd_thread_demo(void) {
    pthread_t threads[3];
    int i;

    printf("THREAD DEMONSTRATION\n");
    for (i = 0; i < 3; i++) {
        if (pthread_create(&threads[i], NULL, thread_worker, NULL) != 0) {
            perror("pthread_create");
            return -1;
        }
    }

    for (i = 0; i < 3; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return -1;
        }
    }

    printf("Three real POSIX threads were created and joined successfully.\n");
    return 0;
}

static void *race_worker(void *arg) {
    int *limit = (int *)arg;
    int i;
    for (i = 0; i < *limit; i++) {
        g_shared_counter++;
    }
    return NULL;
}

int cmd_race_demo(void) {
    pthread_t threads[4];
    int limit = 50000;
    int i;

    printf("RACE CONDITION DEMO\n");
    for (i = 0; i < 4; i++) {
        if (pthread_create(&threads[i], NULL, race_worker, &limit) != 0) {
            perror("pthread_create");
            return -1;
        }
    }

    for (i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Expected result: 200000\n");
    printf("Observed result: %d\n", g_shared_counter);
    printf("If the values differ, the race is real because multiple threads accessed a shared counter without synchronization.\n");
    return 0;
}

static pthread_mutex_t mutex_demo_lock = PTHREAD_MUTEX_INITIALIZER;

static void *mutex_worker(void *arg) {
    int *limit = (int *)arg;
    int i;
    for (i = 0; i < *limit; i++) {
        pthread_mutex_lock(&mutex_demo_lock);
        g_shared_counter++;
        pthread_mutex_unlock(&mutex_demo_lock);
    }
    return NULL;
}

int cmd_mutex_demo(void) {
    pthread_t threads[4];
    int limit = 50000;
    int i;

    g_shared_counter = 0;
    printf("MUTEX DEMO\n");
    for (i = 0; i < 4; i++) {
        if (pthread_create(&threads[i], NULL, mutex_worker, &limit) != 0) {
            perror("pthread_create");
            return -1;
        }
    }

    for (i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Protected counter result: %d\n", g_shared_counter);
    printf("The mutex prevented concurrent write races on the shared counter.\n");
    return 0;
}

struct condition_data {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int ready;
};

static void *condition_worker(void *arg) {
    struct condition_data *data = (struct condition_data *)arg;
    pthread_mutex_lock(&data->lock);
    data->ready = 1;
    pthread_cond_signal(&data->cond);
    pthread_mutex_unlock(&data->lock);
    return NULL;
}

int cmd_condition_demo(void) {
    struct condition_data data;
    pthread_t worker;

    pthread_mutex_init(&data.lock, NULL);
    pthread_cond_init(&data.cond, NULL);
    data.ready = 0;

    pthread_create(&worker, NULL, condition_worker, &data);
    pthread_mutex_lock(&data.lock);
    while (!data.ready) {
        pthread_cond_wait(&data.cond, &data.lock);
    }
    pthread_mutex_unlock(&data.lock);
    pthread_join(worker, NULL);

    printf("CONDITION VARIABLE DEMO\n");
    printf("Condition satisfied: ready=%d\n", data.ready);
    printf("pthread_cond_wait() and pthread_cond_signal() were used to synchronize the worker.\n");

    pthread_mutex_destroy(&data.lock);
    pthread_cond_destroy(&data.cond);
    return 0;
}

int cmd_semaphore_demo(void) {
    sem_t sem;
    pid_t child;
    int status;

    if (sem_init(&sem, 0, 1) != 0) {
        perror("sem_init");
        return -1;
    }

    child = fork();
    if (child < 0) {
        perror("fork");
        sem_destroy(&sem);
        return -1;
    }

    if (child == 0) {
        sem_wait(&sem);
        printf("[child] acquired semaphore and is running.\n");
        sleep(1);
        sem_post(&sem);
        _exit(0);
    }

    sem_wait(&sem);
    printf("[parent] acquired semaphore and is running.\n");
    sleep(1);
    sem_post(&sem);
    waitpid(child, &status, 0);
    sem_destroy(&sem);
    printf("Semaphore demonstration completed using sem_wait() and sem_post().\n");
    return 0;
}

struct deadlock_lock_pair {
    pthread_mutex_t *mutex_a;
    pthread_mutex_t *mutex_b;
};

static void *deadlock_thread_a(void *arg) {
    struct deadlock_lock_pair *pair = (struct deadlock_lock_pair *)arg;
    pthread_mutex_lock(pair->mutex_a);
    printf("[a] acquired mutex A\n");
    sleep(1);
    if (pthread_mutex_trylock(pair->mutex_b) != 0) {
        printf("[a] mutex B is already held; circular wait pattern detected.\n");
        pthread_mutex_unlock(pair->mutex_a);
        return NULL;
    }
    pthread_mutex_unlock(pair->mutex_b);
    pthread_mutex_unlock(pair->mutex_a);
    return NULL;
}

static void *deadlock_thread_b(void *arg) {
    struct deadlock_lock_pair *pair = (struct deadlock_lock_pair *)arg;
    pthread_mutex_lock(pair->mutex_b);
    printf("[b] acquired mutex B\n");
    sleep(1);
    if (pthread_mutex_trylock(pair->mutex_a) != 0) {
        printf("[b] mutex A is already held; circular wait pattern detected.\n");
        pthread_mutex_unlock(pair->mutex_b);
        return NULL;
    }
    pthread_mutex_unlock(pair->mutex_a);
    pthread_mutex_unlock(pair->mutex_b);
    return NULL;
}

int cmd_deadlock_demo(void) {
    pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_b = PTHREAD_MUTEX_INITIALIZER;
    pthread_t t1, t2;
    struct deadlock_lock_pair pair = {&mutex_a, &mutex_b};

    printf("DEADLOCK DEMO\n");
    printf("This is a safe, controlled educational demonstration and does not freeze the console.\n");
    printf("The classical circular-wait pattern is simulated with trylock() and timeout-safe recovery.\n");

    if (pthread_create(&t1, NULL, deadlock_thread_a, &pair) != 0) {
        perror("pthread_create");
        return -1;
    }
    if (pthread_create(&t2, NULL, deadlock_thread_b, &pair) != 0) {
        perror("pthread_create");
        return -1;
    }

    sleep(2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_mutex_destroy(&mutex_a);
    pthread_mutex_destroy(&mutex_b);
    printf("Deadlock demonstration completed without hanging the console.\n");
    return 0;
}
