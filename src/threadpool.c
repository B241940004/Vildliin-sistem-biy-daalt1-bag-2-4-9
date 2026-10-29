#include <stdlib.h>
#include "threadpool.h"

static void emit(threadpool_t *p, const char *ev, int w, int t) {
    if (p->cb) p->cb(ev, w, t);
}

static void *worker_main(void *arg) {
    worker_t *me = (worker_t *)arg;
    threadpool_t *p = me->pool;

    pthread_mutex_lock(&p->lock);
    emit(p, "idle", me->id, -1);
    for (;;) {
        while (p->count == 0 && !p->shutdown)
            pthread_cond_wait(&p->not_empty, &p->lock);
        if (p->count == 0 && p->shutdown)
            break;                              /* queue drained, time to leave */

        task_t t = p->queue[p->head];           /* dequeue */
        p->head = (p->head + 1) % p->cap;
        p->count--;
        emit(p, "start", me->id, t.id);
        pthread_mutex_unlock(&p->lock);

        t.function(t.data);                     /* run outside the lock */

        pthread_mutex_lock(&p->lock);
        emit(p, "done", me->id, t.id);
        emit(p, "idle", me->id, -1);
    }
    emit(p, "exit", me->id, -1);
    pthread_mutex_unlock(&p->lock);
    return NULL;
}

void pool_shutdown(threadpool_t *p) {
    if (!p->queue) return;                      /* already shut down */
    pthread_mutex_lock(&p->lock);
    p->shutdown = 1;
    pthread_cond_broadcast(&p->not_empty);
    pthread_mutex_unlock(&p->lock);
    for (int i = 0; i < p->nworkers; i++)
        pthread_join(p->workers[i].tid, NULL);
    pthread_mutex_destroy(&p->lock);
    pthread_cond_destroy(&p->not_empty);
    free(p->queue);
    free(p->workers);
    p->queue = NULL;
    p->workers = NULL;
}

int pool_init(threadpool_t *p, int workers, int queue_cap, event_cb cb) {
    if (workers < 1 || queue_cap < 1) return 1;
    p->queue = malloc(sizeof(task_t) * queue_cap);
    p->workers = malloc(sizeof(worker_t) * workers);
    if (!p->queue || !p->workers) {
        free(p->queue);
        free(p->workers);
        p->queue = NULL;
        p->workers = NULL;
        return 1;
    }
    p->cap = queue_cap; p->head = p->tail = p->count = 0;
    p->shutdown = 0; p->nworkers = 0; p->cb = cb;
    pthread_mutex_init(&p->lock, NULL);
    pthread_cond_init(&p->not_empty, NULL);
    for (int i = 0; i < workers; i++) {
        p->workers[i].id = i + 1;
        p->workers[i].pool = p;
        if (pthread_create(&p->workers[i].tid, NULL, worker_main, &p->workers[i])) {
            pool_shutdown(p);                   /* joins the i workers already running */
            return 1;
        }
        p->nworkers = i + 1;
    }
    return 0;
}

int pool_submit(threadpool_t *p, void (*f)(void *), void *data, int task_id) {
    pthread_mutex_lock(&p->lock);
    if (p->shutdown || p->count == p->cap) {
        emit(p, "rejected", -1, task_id);
        pthread_mutex_unlock(&p->lock);
        return 1;
    }
    p->queue[p->tail] = (task_t){ f, data, task_id };
    p->tail = (p->tail + 1) % p->cap;
    p->count++;
    emit(p, "submit", -1, task_id);
    pthread_cond_signal(&p->not_empty);
    pthread_mutex_unlock(&p->lock);
    return 0;
}
