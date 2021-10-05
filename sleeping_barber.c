/*
  Main guide to develop modifications in the code https://www.youtube.com/watch?v=cArBsUK1ufQ
  Base code was taken from https://github.com/Stolichnayer/sleeping_barber
  This code was modified based on ST0257_Project_II specifications.
*/


#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void *barber_function(void *idp);
void *customer_function(void *idp);
void service_customer();
void *make_customer_function();

/* Mutex*/
pthread_mutex_t srvCust;

/* Semaphores */
sem_t barber_ready;
sem_t customer_ready;
sem_t modifySeats;

/* Inputs */
int chair_cnt;
int total_customers;

int available_seats;
int no_served_customers = 0;
time_t waiting_time_sum;

void *barber_function(void *idp) {
  int counter = 0;

  while (1) {
    /* Lock semaphore "customer_ready" - try to get a customer or sleep if there
     * is none */
    sem_wait(&customer_ready);

    /* Lock semaphore "modifySeats" - try to get access to seats */
    sem_wait(&modifySeats);

    /* Increment by 1 the available seats */
    available_seats++;

    /* Unlock semaphore "modifySeats" */
    sem_post(&modifySeats);

    /*Unlock semaphore "barber_ready" - set barber ready to serve" */
    sem_post(&barber_ready);

    /* Lock mutex "srvCust - protect service from the same barber from other
     * threads */
    pthread_mutex_lock(&srvCust);

    /* serve customer */
    service_customer();

    /* Unlock mutex "srvCust - finished service */
    pthread_mutex_unlock(&srvCust);

    printf("\n\033[1;34mA customer was served, the barber is calling a new "
           "customer.\033[0m\n\n");
    counter++;
    if (counter == (total_customers - no_served_customers))
      break;
  }
  pthread_exit(NULL);
}

void *customer_function(void *idp) {
  struct timeval start, stop;

  /* Lock semaphore "modifySeats" */
  sem_wait(&modifySeats);

  /* If there is available seat*/
  if (available_seats >= 1) {
    /* Occupy a seat*/
    available_seats--;

    printf("Customer \033[0;36m[pid = %lu]\033[0m arrived and is waiting.\n",
           pthread_self());
    printf("\033[0;32mAvailable seats: %d\033[0m\n", available_seats);

    /* Start waiting-time counter */
    gettimeofday(&start, NULL);

    /* Unlock semaphore "customer_ready" - set the customer ready to be served
     */
    sem_post(&customer_ready);

    /* Unlock semaphore "modifySeats" */
    sem_post(&modifySeats);

    /* Lock semaphore "barber_ready" - wait for barber to get ready */
    sem_wait(&barber_ready);

    /* Stop waiting-time counter */
    gettimeofday(&stop, NULL);

    double sec = (double)(stop.tv_usec - start.tv_usec) / 1000000 +
                 (double)(stop.tv_sec - start.tv_sec);

    /* Assigning the time spent to global variable (ms)*/
    waiting_time_sum += 1000 * sec;

    printf("and is being served now. \033[0;36m[pid = %lu]\033[0m\n",
           pthread_self());

  } else {
    /* Unlock semaphore "modifySeats" */
    sem_post(&modifySeats);
    no_served_customers++;
    printf("\n\033[1;31mA Customer had to leave.\033[0m\n\n");
  }

  pthread_exit(NULL);
}

void service_customer() {

  int s = (rand() % 4) + 1; /* Random number between 1 and 4 (Cut hair)*/
  char str[] = "Customer selected \033[0;36mcut hair";

  switch (s) {
  case 1:
    printf("%s %d - 0.5s\033[0m ", str, s);
    usleep(500000);
    break;
  case 2:
    printf("%s %d - 1s\033[0m ", str, s);
    usleep(1000000);
    break;
  case 3:
    printf("%s %d - 2s\033[0m ", str, s);
    usleep(2000000);
    break;
  case 4:
    printf("%s %d - 3s\033[0m ", str, s);
    usleep(3000000);
    break;
  // operator doesn't match any case [1,4] /
  default:
    printf("Sorry! Hair cut not avaiable.\n");
  }
}

void *make_customer_function() {
  int tmp;
  int counter = 0;

  while (counter < total_customers) {
    /* Declare and create a customer thread */
    pthread_t customer_thread;
    tmp =
        pthread_create(&customer_thread, NULL, (void *)customer_function, NULL);
    if (tmp) {
      printf("Failed to create thread.");
    }

    /* Increment the counter */
    counter++;

    /* Sleep for random time before creating another customer [0, 3]s */
    int s = rand() % 4;
    s *= 1000000;
    usleep(s);
  }
  pthread_exit(NULL);
}

int main() {

  time_t t;
  /* Initialization, should only be called once. */
  // srand(time(NULL));
  srand((unsigned)time(&t));
  /* Barber 1 thread */
  pthread_t barber_1;

  /* Thread that creates customers */
  pthread_t customer_maker;

  int tmp;

  /* Initialize the mutex */
  pthread_mutex_init(&srvCust, NULL);

  /* Initialize semaphores */
  sem_init(&customer_ready, 0, 0);
  sem_init(&barber_ready, 0, 0);
  sem_init(&modifySeats, 0, 1);

  /* Set color to Bold green */
  printf("\033[1;32m");

  printf("Please enter the number of seats: ");
  scanf("%d", &chair_cnt);

  printf("Please enter the number of Customers: ");
  scanf("%d", &total_customers);

  printf("\n\n");


  /* Use as default values to debug

  printf("Default chair amount = 10\n");
  chair_cnt = 10;

  printf("Generatig a random amount of costumers [15, 50]: ");
  total_customers = rand() % 36 + 15;
  printf("%d\n\n", total_customers);
  */


  /* Set color to default */
  printf("\033[0m");

  available_seats = chair_cnt;

  /* Create barber thread*/
  tmp = pthread_create(&barber_1, NULL, (void *)barber_function, NULL);
  if (tmp)
    printf("Failed to create thread.");

  /* Create customer_maker thread*/
  tmp = pthread_create(&customer_maker, NULL, (void *)make_customer_function,
                       NULL);
  if (tmp)
    printf("Failed to create thread.");

  /* Wait for threads to finish */
  pthread_join(barber_1, NULL);
  pthread_join(customer_maker, NULL);

  printf("\n\033[1;35m------------------------------------------------\n");
  printf("Average customers' waiting time: %f ms.\n",
         (waiting_time_sum / (double)(total_customers - no_served_customers)));
  printf("Number of customers that were forced to leave: %d\n",
         no_served_customers);
}
