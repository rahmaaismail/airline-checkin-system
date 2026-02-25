#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#define NCLERKS 5
#define BUSINESS 1
#define ECONOMY 0

/* ======================= */
/* ====== STRUCTS ======== */
/* ======================= */

typedef struct {
    int user_id;
    int class_type;      // 0 economy, 1 business
    int arrival_time;    // in tenths of seconds
    int service_time;    // in tenths of seconds

    double queue_enter_time;
} customer_info;

/* ======================= */
/* ===== GLOBALS ========= */
/* ======================= */

struct timeval init_time;

double overall_waiting_time = 0;
double business_waiting_time = 0;
double economy_waiting_time = 0;

int total_customers = 0;

/* ======================= */
/* ===== TIME FUNC ======= */
/* ======================= */

double getCurrentSimulationTime() {
    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);

    double cur_secs = cur_time.tv_sec + (double)cur_time.tv_usec / 1000000;
    double init_secs = init_time.tv_sec + (double)init_time.tv_usec / 1000000;

    return cur_secs - init_secs;
}

/* ======================= */
/* ===== THREAD FUNCS ==== */
/* ======================= */

void* customer_entry(void* arg) {
    customer_info* cust = (customer_info*)arg;

    usleep(cust->arrival_time * 100000);

    printf("Customer %d arrived at %.2f\n",
           cust->user_id,
           getCurrentSimulationTime());

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s customers.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Record simulation start time */
    gettimeofday(&init_time, NULL);

    printf("Simulation started...\n");

    /* For now just test threading */
    total_customers = 3;

    customer_info customers[3] = {
        {1, ECONOMY, 2, 5},
        {2, BUSINESS, 4, 3},
        {3, ECONOMY, 6, 2}
    };

    pthread_t threads[3];

    for (int i = 0; i < total_customers; i++) {
        if (pthread_create(&threads[i], NULL,
                           customer_entry,
                           &customers[i]) != 0) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < total_customers; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Simulation finished.\n");

    return 0;
}