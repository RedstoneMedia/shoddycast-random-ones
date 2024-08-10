#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

#define N_THREADS 12
#define STEPS_PER_ROUND 231
#define MAX_ROUNDS 1000000000
#define PRINT_EVERY 10000000

#define THREAD_MAX_ROUNDS (int) ceil((double) MAX_ROUNDS / (double) N_THREADS) // Divides up the work between the threads, otherwise we would be doing way more steps in total

static __thread uint32_t rand_state = 1;

__attribute__((always_inline)) uint32_t xorshift32(void) {
    uint32_t x = rand_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rand_state = x;
    return x;
}

__attribute__((always_inline)) uint32_t uniform_random(uint32_t min_value, uint32_t max_value) {
    return min_value + (xorshift32() % (max_value - min_value + 1));
}

uint32_t simulate_round() {
    uint32_t n_ones = 0;
    for (int i = 0; i < STEPS_PER_ROUND; ++i) {
        n_ones += !uniform_random(0, 3); // Check for "One" (in this case 0)
    }
    return n_ones;
}

typedef struct {
    int thread_i;
    uint32_t max_n_ones;
} SimulateData;

void* simulate_rounds(void* args) {
    // Unpack thread data
    SimulateData* data = (SimulateData*) args;
    int thread_i = data->thread_i;
    // Set seed based on time and thread id
    rand_state = (uint32_t) time(NULL) + thread_i;
    // Simulate rounds
    uint32_t max_n_ones = 0;
    for (int i = 0; i < THREAD_MAX_ROUNDS; ++i) {
        uint32_t n_ones = simulate_round();
        if (n_ones > max_n_ones) {
            max_n_ones = n_ones;
        }
        // The first thread is responsible for printing progress
        if (i % PRINT_EVERY == 0 && thread_i == 0) {
            printf(".");
        }
    }
    data->max_n_ones = max_n_ones;
    return NULL;
}


int main() {
    // Start n threads to simulate rounds
    SimulateData data[N_THREADS];
    pthread_t threads[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        data[i].thread_i = i;
        pthread_create(&threads[i], NULL, simulate_rounds, &data[i]);
    }
    // Wait for threads to complete and capture results
    uint32_t max_n_ones = 0;
    for (int i = 0; i < N_THREADS; i++) {
        pthread_join(threads[i], NULL);
        if (i == 0) printf("\n");
        uint32_t n_ones = data[i].max_n_ones;
        printf("[T%u]: Highest Ones Roll: %d\n", i, n_ones);
        if (n_ones > max_n_ones) {
            max_n_ones = n_ones;
        }
    }
    // Print finial data
    printf("Highest Ones Roll: %d\n", max_n_ones);
    printf("Number of Roll Sessions: %d\n", THREAD_MAX_ROUNDS * N_THREADS);
    return 0;
}
