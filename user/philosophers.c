#include "philosophers.h"

uint32_t* forks[16];
uint32_t* waiter;

void philosopher (int id) {
    char id_string[12];
    itoa(id_string, id);
    // Philosopher locks waiter to 'ask' for forks, receives forks then eats
    while (1) {
        sem_wait(waiter);
        sem_wait(forks[id]);
        sem_wait(forks[(id + 1) %16]);
        sem_post(waiter);
        write(STDOUT_FILENO, id_string, str_len(id_string));
        sem_post(forks[id]);
        sem_post(forks[(id + 1) %16]);
    }
}

void main_dining () {
    for (int i = 0 ; i < 16 ; i++) {
        forks[i] = sem_init(1);
    }
    waiter = sem_init(1);
    for (int i = 0 ; i < 16 ; i++) {
        if (fork() == 0) {
            philosopher(i);
            break;
        }
    }
    exit(EXIT_SUCCESS);
}
