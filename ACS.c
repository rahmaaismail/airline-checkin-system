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

typedef struct node {
    customer_info *customer;
    struct node *next;
} node;

node *business_queue = NULL;
node *economy_queue = NULL;

int business_count = 0;
int economy_count = 0;

/* ======================= */
/* ===== GLOBALS ========= */
/* ======================= */

struct timeval init_time;

double overall_waiting_time = 0;
double business_waiting_time = 0;
double economy_waiting_time = 0;

int total_customers = 0;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

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

void enqueue(node **queue, customer_info *cust) {
    node *new_node = (node*)malloc(sizeof(node));
    new_node->customer = cust;
    new_node->next = NULL;

    if (*queue == NULL) {
        *queue = new_node;
    } else {
        node *temp = *queue;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = new_node;
    }
}

customer_info* dequeue(node **queue) {
    if (*queue == NULL)
        return NULL;

    node *temp = *queue;
    customer_info *cust = temp->customer;

    *queue = temp->next;
    free(temp);

    return cust;
}

void* customer_entry(void* arg) {

    customer_info* cust = (customer_info*)arg;

    /* Simulate arrival delay */
    usleep(cust->arrival_time * 100000);

    printf("Customer %d arrives at %.2f\n",
           cust->user_id,
           getCurrentSimulationTime());

    pthread_mutex_lock(&queue_mutex);

    cust->queue_enter_time = getCurrentSimulationTime();

    if (cust->class_type == BUSINESS) {
        enqueue(&business_queue, cust);
        business_count++;
        printf("Customer %d enters business queue\n",
               cust->user_id);
    } else {
        enqueue(&economy_queue, cust);
        economy_count++;
        printf("Customer %d enters economy queue\n",
               cust->user_id);
    }

    /* Signal clerks that someone is waiting */
    pthread_cond_signal(&queue_cond);

    pthread_mutex_unlock(&queue_mutex);

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s customers.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    gettimeofday(&init_time, NULL);

    /* Read number of customers */
    fscanf(fp, "%d\n", &total_customers);

    customer_info *customers =
        (customer_info*)malloc(sizeof(customer_info) * total_customers);

    if (!customers) {
        perror("malloc failed");
        fclose(fp);
        return EXIT_FAILURE;
    }

    char line[256];

    for (int i = 0; i < total_customers; i++) {

        fgets(line, sizeof(line), fp);

        sscanf(line, "%d:%d,%d,%d",
               &customers[i].user_id,
               &customers[i].class_type,
               &customers[i].arrival_time,
               &customers[i].service_time);

        customers[i].queue_enter_time = 0;
    }

    fclose(fp);

    printf("Simulation started...\n");

    pthread_t *threads =
        (pthread_t*)malloc(sizeof(pthread_t) * total_customers);

    for (int i = 0; i < total_customers; i++) {
        pthread_create(&threads[i], NULL,
                       customer_entry,
                       &customers[i]);
    }

    for (int i = 0; i < total_customers; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Simulation finished.\n");

    free(customers);
    free(threads);

    return 0;
}