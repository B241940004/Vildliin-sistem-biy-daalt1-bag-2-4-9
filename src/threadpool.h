/* threadpool.h - POSIX thread pool (OS Concepts, Programming Project: Designing a Thread Pool) */
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

typedef struct {
    void (*function)(void *param);
    void *data;
    int id;
} task_t;

/* Event hook: used by the UI logger. ev = "submit","rejected","start","done","idle","exit" */
typedef void (*event_cb)(const char *ev, int worker, int task);

typedef struct {
    int id;
    struct threadpool *pool;
    pthread_t tid;
} worker_t;

typedef struct threadpool {
    task_t *queue;          /* circular buffer */
    int cap, head, tail, count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    int shutdown;
    worker_t *workers;
    int nworkers;
    event_cb cb;
} threadpool_t;

int  pool_init(threadpool_t *p, int workers, int queue_cap, event_cb cb);
/* returns 0 on success, 1 if the queue is full (task was NOT accepted) */
int  pool_submit(threadpool_t *p, void (*f)(void *), void *data, int task_id);
/* waits until queued tasks are finished, then joins all workers */
void pool_shutdown(threadpool_t *p);

#endif
