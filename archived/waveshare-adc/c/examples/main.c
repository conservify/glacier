#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h> 
#include <math.h>

#include "ADS1263.h"
#include "tinywav.h"

// Modify according to actual voltage external AVDD and AVSS(Default), or internal 2.5V
#define REF 2.5
#define NUMBER_OF_CHANNELS         3
#define SAMPLES_PER_FILE           500 * 60
#define BATCH_SIZE                 500
#define MAXIMUM_SAMPLES_PER_MINUTE 1024 * 100

static void signal_handler(int32_t signal_number);

int32_t mkdir_p(const char *path);

int64_t difftimespec_us(const struct timespec after, const struct timespec before) {
    return ((int64_t)after.tv_sec - (int64_t)before.tv_sec) * (int64_t)1000000
         + ((int64_t)after.tv_nsec - (int64_t)before.tv_nsec) / 1000;
}

typedef struct sample_t {
    struct timespec time;
    float values[NUMBER_OF_CHANNELS];
} sample_t;

typedef struct circular_samples_t {
    sample_t *buffer;
    sample_t *end;
    size_t capacity;
    size_t samples;
    sample_t *head;
    sample_t *tail;
} circular_samples_t;

int32_t circular_samples_create(circular_samples_t *cs) {
    bzero(cs, sizeof(circular_samples_t));

    cs->samples = 0;
    cs->capacity = 1024 * 32;
    cs->buffer = (sample_t *)malloc(sizeof(sample_t) * cs->capacity);
    cs->end = cs->buffer + cs->capacity;
    cs->head = cs->buffer;
    cs->tail = cs->buffer;

    return 0;
}

int32_t circular_samples_free(circular_samples_t *cs) {
    if (cs->buffer != NULL) {
        free(cs->buffer);
        cs->buffer = NULL;
        cs->end = NULL;
        cs->head = NULL;
        cs->tail = NULL;
        cs->capacity = 0;
    }

    return 0;
}

int32_t circular_samples_push(circular_samples_t *cs, sample_t *sample) {
    if (cs->samples >= cs->capacity) {
        fprintf(stderr, "buffer full\n");
        return -1;
    }

    memcpy(cs->head, sample, sizeof(sample_t));
    cs->head++;
    if (cs->head == cs->end) {
        cs->head = cs->buffer;
    }
    cs->samples++;

    return 0;
}

sample_t *circular_samples_pop_batch(circular_samples_t *cs, size_t batch_size) {
    if (cs->samples < batch_size) {
        return NULL;
    }

    sample_t *batch = (sample_t *)malloc(sizeof(sample_t) * batch_size);
    sample_t *iter = batch;
    while (batch_size > 0) {
        memcpy((void *)iter, cs->tail, sizeof(sample_t));
        cs->tail++;
        if (cs->tail == cs->end) {
            cs->tail = cs->buffer;
        }
        cs->samples--;
        iter++;
        batch_size--;
    }

    return batch;
}

static void *worker_main(void *arg) {
    circular_samples_t *samples = (circular_samples_t *)arg;
    uint32_t values[NUMBER_OF_CHANNELS];

    struct sched_param param = { 10 };
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    while (1) {
        sample_t sample;
        clock_gettime(CLOCK_REALTIME, &sample.time);
        ads1263_get_channels_diff(values, NUMBER_OF_CHANNELS);
        for (int32_t i = 0; i < NUMBER_OF_CHANNELS; i++) {
            if ((values[i] >> 31) == 1) {
                sample.values[i] = REF * 2 - values[i] / 2147483648.0 * REF; // 0x7fffffff + 1
            } else {
                sample.values[i] = values[i] / 2147483647.0 * REF; //  0x7fffffff
            }
        }

        circular_samples_push(samples, &sample);

#if defined(FIXED_RATE_500HZ)
        struct timespec after;
        clock_gettime(CLOCK_REALTIME, &after);

        uint64_t elapsed_us = (after.tv_nsec / 1000) - (sample.time.tv_nsec / 1000);
        if (elapsed_us < 2000) {
            usleep(2000 - elapsed_us);
        }
#endif
    }

    return NULL;
}

int32_t main() {
    signal(SIGINT, signal_handler);

    dev_module_init();
    ads1263_set_mode(1); // 0 is single channel, 1 is diff channel
    if (ads1263_init_adc1(ADS1263_14400SPS) == 1) {
        dev_module_exit();
        return 2;
    }

    circular_samples_t samples_buffer;
    circular_samples_create(&samples_buffer);

    pthread_t worker;
    if (pthread_create(&worker, NULL, &worker_main, (void *)&samples_buffer) != 0) {
        dev_module_exit();
        return 2;
    }

    uint32_t samples = 0;
    uint32_t batches = 0;
    char directory[128] = { 0 };
    char file_name[256] = { 0 };
    FILE *bin_fp = NULL;
    TinyWav wav;
    time_t second = 0;
    time_t started = 0;
    sample_t *minute = (sample_t *)malloc(sizeof(sample_t) * MAXIMUM_SAMPLES_PER_MINUTE);

    while (1) {
        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);
        time_t now = spec.tv_sec;

        if (second != now) {
            struct tm tm = *localtime(&now);
            if (tm.tm_sec == 0) {
                if (bin_fp != NULL) {
                    fclose(bin_fp);
                    bin_fp = NULL;

                    printf("%lld.%.9lld renaming %s\n", (int64_t)spec.tv_sec, (int64_t)spec.tv_nsec / 1000, file_name);

                    char new_name[512];
                    snprintf(new_name, sizeof(new_name), "%s.bin", file_name);
                    if (rename("incoming.bin", new_name) != 0) {
                        fprintf(stderr, "Error renaming incoming.bin\n");
                    }

                    /*
                    snprintf(new_name, sizeof(new_name), "%s.wav", file_name);
                    if (rename("incoming.wav", new_name) != 0) {
                        fprintf(stderr, "Error renaming incoming.wav\n");
                    }
                    */

                    started = now;
                    samples = 0;
                }

                if (bin_fp == NULL) {
                    bin_fp = fopen("incoming.bin", "w+");
                    if (bin_fp == NULL) {
                        fprintf(stderr, "Error opening incoming.bin\n");
                        return 2;
                    }

                    /*
                    tinywav_open_write(&wav,
                        NUMBER_OF_CHANNELS,
                        SAMPLE_RATE,
                        TW_FLOAT32,
                        TW_INLINE,
                        "incoming.wav"
                    );
                    */

                    snprintf(directory, sizeof(directory), "%04d%02d/%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

                    snprintf(file_name, sizeof(file_name), "%s/geophones_%04d%02d%02d_%02d%02d%02d",
                        directory, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

                    if (mkdir_p(directory) != 0) {
                        fprintf(stderr, "Error creating directory: %s\n", directory);
                    }
                }

                if (started == 0) {
                    printf("%lld.%.9lld starting %s\n", (int64_t)spec.tv_sec, (int64_t)spec.tv_nsec / 1000, file_name);
                    started = now;
                    batches = 0;
                    samples = 0;
                }
            }

            if (started > 0) {
                int32_t elapsed = now - started;
                printf("%lld.%.9lld status %s: %d batches %d samples %fsps\n",
                    (int64_t)spec.tv_sec, (int64_t)spec.tv_nsec / 1000,
                    file_name, batches, samples, (batches * BATCH_SIZE) / (float)elapsed);
            }

            second = now;
        }

        sample_t *batch = circular_samples_pop_batch(&samples_buffer, BATCH_SIZE);
        if (batch != NULL) {
            if (bin_fp != NULL) {
                for (size_t i = 0; i < BATCH_SIZE; ++i) {
                    fwrite((void *)batch[i].values, sizeof(float), NUMBER_OF_CHANNELS, bin_fp);
                }
            }
            memcpy((void *)(minute + samples), (void *)batch, BATCH_SIZE * sizeof(sample_t));
            free(batch);
            batches++;
            samples += BATCH_SIZE;
        }
    }

    return 0;
}

static void signal_handler(int32_t signal_number) {
    fprintf(stderr, "Ctrl-c\n");
    dev_module_exit();
    exit(0);
}

/* Make a directory; already existing dir okay */
static int32_t maybe_mkdir(const char *path, mode_t mode) {
    struct stat st;
    errno = 0;

    /* Try to make the directory */
    if (mkdir(path, mode) == 0)
        return 0;

    /* If it fails for any reason but EEXIST, fail */
    if (errno != EEXIST)
        return -1;

    /* Check if the existing path is a directory */
    if (stat(path, &st) != 0)
        return -1;

    /* If not, fail with ENOTDIR */
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }

    errno = 0;
    return 0;
}

int32_t mkdir_p(const char *path) {
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    char *_path = NULL;
    char *p;
    int result = -1;
    mode_t mode = 0777;

    errno = 0;

    /* Copy string so it's mutable */
    _path = strdup(path);
    if (_path == NULL)
        goto out;

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (maybe_mkdir(_path, mode) != 0)
                goto out;

            *p = '/';
        }
    }

    if (maybe_mkdir(_path, mode) != 0)
        goto out;

    result = 0;

out:
    free(_path);
    return result;
}
