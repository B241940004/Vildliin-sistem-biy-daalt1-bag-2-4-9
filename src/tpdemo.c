/* tpdemo - runs the thread pool and prints a JSON event log for the UI.
 * usage: tpdemo workers queue tasks min_ms max_ms interval_ms [seed] */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "threadpool.h"

typedef struct { double t; char ev[10]; int w, k; } event_t;

static event_t *log_buf; static int log_n, log_cap;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static struct timespec t0;
static int running, max_running, completed, last_rej = -1;

static double now_ms(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec - t0.tv_sec) * 1000.0 + (t.tv_nsec - t0.tv_nsec) / 1e6;
}

static void on_event(const char *ev, int w, int k) {
    pthread_mutex_lock(&log_lock);
    /* a rejected task is retried many times; log the rejection only once per task */
    int skip = 0;
    if (ev[0] == 'r') { if (k == last_rej) skip = 1; else last_rej = k; }
    if (!skip && log_n < log_cap) {
        event_t *e = &log_buf[log_n++];
        e->t = now_ms(); e->w = w; e->k = k;
        snprintf(e->ev, sizeof e->ev, "%s", ev);
    }
    if (ev[0] == 's' && ev[1] == 't') { if (++running > max_running) max_running = running; }
    if (ev[0] == 'd' && ev[1] == 'o') { running--; completed++; }
    pthread_mutex_unlock(&log_lock);
}

static void task_fn(void *param) {
    int ms = *(int *)param;
    usleep(ms * 1000);
}

int main(int argc, char **argv) {
    if (argc < 7) { fprintf(stderr, "usage: %s workers queue tasks min_ms max_ms interval_ms [seed]\n", argv[0]); return 1; }
    int W = atoi(argv[1]), Q = atoi(argv[2]), N = atoi(argv[3]);
    int lo = atoi(argv[4]), hi = atoi(argv[5]), gap = atoi(argv[6]);
    unsigned seed = argc > 7 ? (unsigned)atoi(argv[7]) : (unsigned)time(NULL);
    if (W < 1 || W > 16 || Q < 1 || Q > 32 || N < 1 || N > 200 || lo < 1 || hi < lo || hi > 5000 || gap < 0 || gap > 2000) {
        fprintf(stderr, "invalid parameters\n"); return 1;
    }
    log_cap = N * 5 + W * 4 + 64;
    log_buf = malloc(sizeof(event_t) * log_cap);
    int *dur = malloc(sizeof(int) * N);
    srand(seed);
    for (int i = 0; i < N; i++) dur[i] = lo + rand() % (hi - lo + 1);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    threadpool_t pool;
    pool_init(&pool, W, Q, on_event);
    usleep(20000);                              /* let workers announce themselves */

    int rejected_tasks = 0;
    for (int i = 0; i < N; i++) {
        int first = 1;
        while (pool_submit(&pool, task_fn, &dur[i], i + 1)) {
            if (first) { rejected_tasks++; first = 0; }
            usleep(3000);                       /* queue full: wait and retry */
        }
        if (gap) usleep(gap * 1000);
    }
    pool_shutdown(&pool);
    double total = now_ms();

    printf("{\"params\":{\"workers\":%d,\"queue\":%d,\"tasks\":%d,\"min\":%d,\"max\":%d,\"interval\":%d,\"seed\":%u},\n", W, Q, N, lo, hi, gap, seed);
    printf("\"summary\":{\"completed\":%d,\"max_running\":%d,\"rejected_tasks\":%d,\"total_ms\":%.1f,\"ok\":%s},\n",
           completed, max_running, rejected_tasks, total, (completed == N && max_running <= W) ? "true" : "false");
    printf("\"events\":[\n");
    for (int i = 0; i < log_n; i++)
        printf("{\"t\":%.2f,\"e\":\"%s\",\"w\":%d,\"k\":%d}%s\n", log_buf[i].t, log_buf[i].ev, log_buf[i].w, log_buf[i].k, i + 1 < log_n ? "," : "");
    printf("]}\n");
    free(log_buf); free(dur);
    return 0;
}
