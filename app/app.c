#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define DEVICE "/dev/mon_device"
#define BUFFER_SIZE 1024
#define NB_ITERATIONS 5

long get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

int main(void)
{
    int fd;
    char write_buf[BUFFER_SIZE];
    char read_buf[BUFFER_SIZE];
    long t_start, t_end, latency;
    int i;

    printf("=== Embedded Linux ARM - Userspace App ===\n\n");

    for (i = 0; i < NB_ITERATIONS; i++) {
        snprintf(write_buf, BUFFER_SIZE, "Message %d depuis userspace", i + 1);

        /* Ouvrir pour ecriture */
        fd = open(DEVICE, O_RDWR);
        if (fd < 0) { perror("open"); return EXIT_FAILURE; }

        t_start = get_time_ns();
        if (write(fd, write_buf, strlen(write_buf)) < 0) {
            perror("write"); close(fd); return EXIT_FAILURE;
        }
        t_end = get_time_ns();
        latency = t_end - t_start;
        printf("[%d] WRITE : \"%s\"\n", i + 1, write_buf);
        printf("     Latence write : %ld ns\n", latency);
        close(fd);

        /* Ouvrir pour lecture */
        fd = open(DEVICE, O_RDWR);
        if (fd < 0) { perror("open"); return EXIT_FAILURE; }

        memset(read_buf, 0, BUFFER_SIZE);
        t_start = get_time_ns();
        if (read(fd, read_buf, BUFFER_SIZE) < 0) {
            perror("read"); close(fd); return EXIT_FAILURE;
        }
        t_end = get_time_ns();
        latency = t_end - t_start;
        printf("     READ  : \"%s\"\n", read_buf);
        printf("     Latence read  : %ld ns\n\n", latency);
        close(fd);

        sleep(1);
    }

    printf("=== Application terminee ===\n");
    return EXIT_SUCCESS;
}
