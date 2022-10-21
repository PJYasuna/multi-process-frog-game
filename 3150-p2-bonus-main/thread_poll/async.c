#include <stdlib.h>
#include <pthread.h>
#include "async.h"
#include "utlist.h"
#include <unistd.h>
#include <stdio.h>

typedef struct {
    void (*function)(int);
    int argument;
} threadpool_task_t;

struct threadpool_t {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_t *threads;
  threadpool_task_t *queue;
  int thread_count;
  int queue_size;
  int head;
  int tail;
  int count;
  int shutdown;
  int started;
};

threadpool_t *pool;

void async_init(int num_threads)
{
    printf("ok\n");
    int i;
    /* Initialize */
    pool = (threadpool_t *)malloc(sizeof(threadpool_t));
    pool->thread_count = num_threads;
    pool->queue_size = num_threads;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;
    printf("ok\n");
    /* Allocate thread and task queue */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    pool->queue = (threadpool_task_t *)malloc
        (sizeof(threadpool_task_t) * num_threads);
    printf("ok\n");
    /* Initialize mutex and conditional variable first */
    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);

    void * args = NULL;

    /* Start worker threads */
    for(i = 0; i < num_threads; i++) {
        pthread_create(&(pool->threads[i]), NULL,threadpool_thread, (void*)args);

        pool->thread_count++;
        pool->started++;
    }
    printf("ok\n");
}


void async_run(void (*hanlder)(int), int args)
{

    int next;

 
    pthread_mutex_lock(&(pool->lock));

    next = (pool->tail + 1) % pool->queue_size;

    do {
        
    

        /* Add task to queue */
        pool->queue[pool->tail].function = hanlder;
        pool->queue[pool->tail].argument = args;
        pool->tail = next;
        pool->count += 1;

        /* pthread_cond_broadcast */
        if(pthread_cond_signal(&(pool->notify)) != 0) {
            break;
        }
    } while(0);

    pthread_mutex_unlock(&pool->lock);

}

static void *threadpool_thread(void *t)
{
    threadpool_task_t task;

    for(;;) {
        /* Lock must be taken to wait on conditional variable */
        pthread_mutex_lock(&(pool->lock));

        /* Wait on condition variable, check for spurious wakeups.
           When returning from pthread_cond_wait(), we own the lock. */
        while(pool->count == 0) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if(pool->count == 0) {
            break;
        }

        /* Grab our task */
        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        /* Unlock */
        pthread_mutex_unlock(&(pool->lock));

        /* Get to work */
        (*(task.function))(task.argument);
    }

    pool->started--;

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return(NULL);
}