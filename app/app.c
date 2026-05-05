#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

#define DEVICE          "/dev/mon_device"
#define BUFFER_SIZE     1024
#define NB_ITERATIONS   100
#define LATENCY_THRESHOLD_US 1000  /* seuil alerte : 1000 µs */

/* ─── Statistiques ─────────────────────────────────────── */
typedef struct {
    long min_ns;
    long max_ns;
    long total_ns;
    int  count;
    int  violations;
} Stats;

/* ─── Données partagées entre threads ───────────────────── */
typedef struct {
    char     message[BUFFER_SIZE];
    int      ready;
    pthread_mutex_t mutex;
    pthread_cond_t  cond_produced;
    pthread_cond_t  cond_consumed;
    Stats    write_stats;
    Stats    read_stats;
    int      done;
} SharedData;

/* ─── Utilitaires temps ─────────────────────────────────── */
static long get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static void update_stats(Stats *s, long latency_ns)
{
    if (s->count == 0 || latency_ns < s->min_ns) s->min_ns = latency_ns;
    if (latency_ns > s->max_ns) s->max_ns = latency_ns;
    s->total_ns += latency_ns;
    s->count++;
    if (latency_ns > LATENCY_THRESHOLD_US * 1000)
        s->violations++;
}

static void print_stats(const char *label, const Stats *s)
{
    long avg = s->count ? s->total_ns / s->count : 0;
    printf("  %-10s | min: %6ld µs | max: %6ld µs | avg: %6ld µs | violations: %d/%d\n",
           label,
           s->min_ns / 1000,
           s->max_ns / 1000,
           avg / 1000,
           s->violations,
           s->count);
}

/* ─── Thread Producteur ─────────────────────────────────── */
static void *producer(void *arg)
{
    SharedData *data = (SharedData *)arg;
    int fd;
    long t_start, t_end;
    int i;

    for (i = 0; i < NB_ITERATIONS; i++) {
        pthread_mutex_lock(&data->mutex);
        while (data->ready)
            pthread_cond_wait(&data->cond_consumed, &data->mutex);

        snprintf(data->message, BUFFER_SIZE,
                 "[%03d] SENSOR_TEMP=%.1f SENSOR_PRESS=%.1f SENSOR_FLOW=%.1f",
                 i + 1,
                 20.0 + (rand() % 600) / 10.0,
                 1.0  + (rand() % 50)  / 10.0,
                 0.5  + (rand() % 100) / 10.0);

        pthread_mutex_unlock(&data->mutex);

        /* Ecriture dans le driver */
        fd = open(DEVICE, O_WRONLY);
        if (fd < 0) { perror("producer open"); break; }

        t_start = get_time_ns();
        write(fd, data->message, strlen(data->message));
        t_end = get_time_ns();
        close(fd);

        update_stats(&data->write_stats, t_end - t_start);

        pthread_mutex_lock(&data->mutex);
        data->ready = 1;
        pthread_cond_signal(&data->cond_produced);
        pthread_mutex_unlock(&data->mutex);

        usleep(10000); /* 10 ms entre chaque mesure */
    }

    pthread_mutex_lock(&data->mutex);
    data->done = 1;
    pthread_cond_signal(&data->cond_produced);
    pthread_mutex_unlock(&data->mutex);

    return NULL;
}

/* ─── Thread Consommateur ───────────────────────────────── */
static void *consumer(void *arg)
{
    SharedData *data = (SharedData *)arg;
    int fd;
    char buf[BUFFER_SIZE];
    long t_start, t_end;

    while (1) {
        pthread_mutex_lock(&data->mutex);
        while (!data->ready && !data->done)
            pthread_cond_wait(&data->cond_produced, &data->mutex);

        if (!data->ready && data->done) {
            pthread_mutex_unlock(&data->mutex);
            break;
        }
        pthread_mutex_unlock(&data->mutex);

        /* Lecture depuis le driver */
        fd = open(DEVICE, O_RDONLY);
        if (fd < 0) { perror("consumer open"); break; }

        memset(buf, 0, BUFFER_SIZE);
        t_start = get_time_ns();
        read(fd, buf, BUFFER_SIZE);
        t_end = get_time_ns();
        close(fd);

        update_stats(&data->read_stats, t_end - t_start);
        printf("  READ : %s\n", buf);

        pthread_mutex_lock(&data->mutex);
        data->ready = 0;
        pthread_cond_signal(&data->cond_consumed);
        pthread_mutex_unlock(&data->mutex);
    }

    return NULL;
}

/* ─── Main ──────────────────────────────────────────────── */
int main(void)
{
    SharedData data;
    pthread_t prod_thread, cons_thread;

    memset(&data, 0, sizeof(data));
    pthread_mutex_init(&data.mutex, NULL);
    pthread_cond_init(&data.cond_produced, NULL);
    pthread_cond_init(&data.cond_consumed, NULL);
    srand(42);

    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║   Embedded Linux ARM — Kernel Driver Benchmark   ║\n");
    printf("║   Device : %-36s  ║\n", DEVICE);
    printf("║   Iterations : %-32d  ║\n", NB_ITERATIONS);
    printf("║   Latency threshold : %-26d µs  ║\n", LATENCY_THRESHOLD_US);
    printf("╚══════════════════════════════════════════════════╝\n\n");

    pthread_create(&prod_thread, NULL, producer, &data);
    pthread_create(&cons_thread, NULL, consumer, &data);

    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);

    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║                 Rapport Final                    ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");
    print_stats("WRITE", &data.write_stats);
    print_stats("READ",  &data.read_stats);
    printf("\n  Total iterations : %d\n", NB_ITERATIONS);
    printf("  Threshold        : %d µs\n", LATENCY_THRESHOLD_US);
    printf("\n=== Benchmark termine ===\n\n");

    pthread_mutex_destroy(&data.mutex);
    pthread_cond_destroy(&data.cond_produced);
    pthread_cond_destroy(&data.cond_consumed);

    return EXIT_SUCCESS;
}
