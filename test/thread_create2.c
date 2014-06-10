/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <abt.h>

#define DEFAULT_NUM_STREAMS     4
#define DEFAULT_NUM_THREADS     4

#define HANDLE_ERROR(ret,msg)                           \
    if (ret != ABT_SUCCESS) {                           \
        fprintf(stderr, "ERROR[%d]: %s\n", ret, msg);   \
        exit(EXIT_FAILURE);                             \
    }

int num_threads = DEFAULT_NUM_THREADS;

void thread_func(void *arg)
{
    size_t my_id = (size_t)arg;
    printf("[TH%lu]: Hello, world!\n", my_id);
}

void thread_create(void *arg)
{
    int i, ret;
    size_t my_id = (size_t)arg;
    ABT_stream my_stream;

    ret = ABT_stream_self(&my_stream);
    HANDLE_ERROR(ret, "ABT_stream_self");

    /* Create threads */
    for (i = 0; i < num_threads; i++) {
        size_t tid = 100 * my_id + i;
        ret = ABT_thread_create(my_stream,
                thread_func, (void *)tid, 16384,
                NULL);
        HANDLE_ERROR(ret, "ABT_thread_create");
    }

    printf("[TH%lu]: created %d threads\n", my_id, num_threads);
}

int main(int argc, char *argv[])
{
    int i;
    int ret;
    int num_streams = DEFAULT_NUM_STREAMS;
    if (argc > 1) num_streams = atoi(argv[1]);
    assert(num_streams >= 0);
    if (argc > 2) num_threads = atoi(argv[2]);
    assert(num_threads >= 0);

    ABT_stream *streams;
    streams = (ABT_stream *)malloc(sizeof(ABT_stream) * num_streams);

    /* Initialize */
    ret = ABT_init(argc, argv);
    HANDLE_ERROR(ret, "ABT_init");

    /* Create streams */
    ret = ABT_stream_self(&streams[0]);
    HANDLE_ERROR(ret, "ABT_stream_self");
    for (i = 1; i < num_streams; i++) {
        ret = ABT_stream_create(ABT_SCHEDULER_NULL, &streams[i]);
        HANDLE_ERROR(ret, "ABT_stream_create");
    }

    /* Create one thread for each stream */
    for (i = 0; i < num_streams; i++) {
        size_t tid = i + 1;
        ret = ABT_thread_create(streams[i],
                thread_create, (void *)tid, 16384,
                NULL);
        HANDLE_ERROR(ret, "ABT_thread_create");
    }

    /* Switch to other user level threads */
    ABT_thread_yield();

    /* Join streams */
    for (i = 1; i < num_streams; i++) {
        ret = ABT_stream_join(streams[i]);
        HANDLE_ERROR(ret, "ABT_stream_join");
    }

    /* Free streams */
    for (i = 1; i < num_streams; i++) {
        ret = ABT_stream_free(&streams[i]);
        HANDLE_ERROR(ret, "ABT_stream_free");
    }

    /* Finalize */
    ret = ABT_finalize();
    HANDLE_ERROR(ret, "ABT_finalize");

    free(streams);

    return EXIT_SUCCESS;
}
