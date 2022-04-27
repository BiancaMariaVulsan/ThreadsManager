#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h> 
#include <stdbool.h>
#include "a2_helper.h"

#define MAX_TH_P2 5
#define MAX_TH_P8 43
#define MAX_TH_P3 6

pthread_mutex_t lock1;
pthread_mutex_t lock2;
pthread_cond_t cond1;
pthread_cond_t cond2;
pthread_cond_t condT5_beg;
pthread_cond_t cond_end;
pthread_cond_t cond_beg;
bool T3beg = false;
bool T5end = false;
int th_count = 0;
int th_no = MAX_TH_P8;
sem_t sem;
sem_t barrier;

sem_t *semT21_beg;
sem_t *semT21_end;

bool T11beg = false;
bool T11end = false;

void P(sem_t *sem)
{
    sem_wait(sem);
}

void V(sem_t *sem)
{
    sem_post(sem);
}

void* thread_function_P2(void* arg) {
    int th_id = *((int*) arg);

    info(BEGIN, 2, th_id);

    info(END, 2, th_id);

    return 0;
}

void* thread_1_function_P2(void* arg) {
    int th_id = *((int*) arg);

    sem_wait(semT21_beg);
    info(BEGIN, 2, th_id);

    info(END, 2, th_id);

    sem_post(semT21_end);

    return 0;
}

void* thread_3_function_P2(void* arg) {
    int th_id = *((int*) arg);

    info(BEGIN, 2, th_id);

    pthread_mutex_lock(&lock1);
    T3beg = true;
    if (pthread_cond_signal(&condT5_beg) != 0) {
        perror("Cannot signal the condition waiters");
        exit(3);
    }
    pthread_mutex_unlock(&lock1);

    if (pthread_mutex_lock(&lock1) != 0) {
        perror("Cannot take the lock");
        exit(4);
    }

    while (T5end != true) {
        if (pthread_cond_wait(&cond1, &lock1) != 0) {
            perror("Cannot wait for condition");
            exit(2);
        }
    }

    if (pthread_mutex_unlock(&lock1) != 0) {
        perror("Cannot release the lock");
        exit(5);
    }

    info(END, 2, th_id);

    return 0;
}

void* thread_5_function_P2(void* arg) {
    int th_id = *((int*) arg);

    pthread_mutex_lock(&lock1);
    while(T3beg != true) {
        if (pthread_cond_wait(&condT5_beg, &lock1) != 0) {
            perror("Cannot wait for condition");
            exit(2);
        }
    }
    pthread_mutex_unlock(&lock1);

    info(BEGIN, 2, th_id);
    
    if (pthread_mutex_lock(&lock1) != 0) {
        perror("Cannot take the lock");
        exit(4);
    }

    T5end = true;

    if (pthread_cond_signal(&cond1) != 0) {
        perror("Cannot signal the condition waiters");
        exit(3);
    }

    info(END, 2, th_id);

    if (pthread_mutex_unlock(&lock1) != 0) {
        perror("Cannot release the lock");
        exit(5);
    }

    return 0;
}

void create_threads_P2() {
    pthread_t th[MAX_TH_P2 + 1];
    int th_args[MAX_TH_P2 + 1];

    semT21_beg = sem_open("/semT21_beg\0", O_CREAT, 0644, 0);
    semT21_end = sem_open("/semT21_end\0", O_CREAT, 0644, 0);

    // Create the N threads
    for (int i=1; i<=MAX_TH_P2; i++) {
    	th_args[i] = i;
        if(i == 1) {
            if (pthread_create(&th[i], NULL, thread_1_function_P2, &th_args[i]) != 0) {
                perror("Cannot create threads");
                exit(1);
            }
        } else if(i == 3) {
            if (pthread_create(&th[i], NULL, thread_3_function_P2, &th_args[i]) != 0) {
                perror("Cannot create threads");
                exit(1);
            }
        } else if(i == 5) {
            if (pthread_create(&th[i], NULL, thread_5_function_P2, &th_args[i]) != 0) {
                perror("Cannot create threads");
                exit(1);
            }
        } else {
            if (pthread_create(&th[i], NULL, thread_function_P2, &th_args[i]) != 0) {
                perror("Cannot create threads");
                exit(1);
            }
        }
    }

    // Wait for the termination of the N threads created
    for (int i=1; i<=MAX_TH_P2; i++) {
        pthread_join(th[i], NULL);
    }

    sem_close(semT21_beg);
    sem_close(semT21_end);
}

void* thread_8_function_P11(void* arg) {
    int th_id = *((int*) arg);

    pthread_mutex_lock(&lock2);
    while (th_count == 5) {
        if (pthread_cond_wait(&cond_beg, &lock2) != 0) {
            perror("Cannot wait for condition");
            exit(2);
        }
    }
    T11beg = true;
    th_count++;
    pthread_mutex_unlock(&lock2);

    info(BEGIN, 8, th_id);

    if (pthread_mutex_lock(&lock2) != 0) {
        perror("Cannot take the lock");
        exit(4);
    }

    while (th_count != 5) {
        if (pthread_cond_wait(&cond2, &lock2) != 0) {
            perror("Cannot wait for condition");
            exit(2);
        }
    }
    // printf("count: %d \n", th_count);
    info(END, 8, th_id);
    T11end = true;
    pthread_cond_broadcast(&cond_end);
    pthread_cond_broadcast(&cond_beg);
    
    if (pthread_mutex_unlock(&lock2) != 0) {
        perror("Cannot release the lock");
        exit(5);
    }
    return 0;
}

void* thread_function_P8(void* arg) {
    int th_id = *((int*) arg);

    // P(&sem);
    pthread_mutex_lock(&lock2);
     while (th_count == 5) {
        if (pthread_cond_wait(&cond_beg, &lock2) != 0) {
            perror("Cannot wait for condition");
            exit(2);
        }
     }
    th_count++;
    pthread_mutex_unlock(&lock2);

    info(BEGIN, 8, th_id);

    pthread_mutex_lock(&lock2);
    if (th_count == 5 && T11beg == true) {
        if (pthread_cond_signal(&cond2) != 0) {
            perror("Cannot signal the condition waiters");
            exit(3);
        }
    }
    pthread_mutex_unlock(&lock2);

    pthread_mutex_lock(&lock2);
    while ((T11end == false && T11beg == true) || (th_no == 5 && T11end == false)) {
        if (pthread_cond_wait(&cond_end, &lock2) != 0) {
            perror("Cannot wait for condition");
            exit(2);
        }
    }
    th_count--;
    th_no--;
    pthread_cond_broadcast(&cond_beg);
    info(END, 8, th_id);
    pthread_mutex_unlock(&lock2);
    // V(&sem);

    return 0;
}

void create_threads_P8() {
    pthread_t th[MAX_TH_P8 + 1];
    int th_args[MAX_TH_P8 + 1];

    // Create unamed semaphore
    if (sem_init(&sem, 0, 5) < 0) {
        perror("Error creating the semaphore");
        exit(2);
    }    
    // Create unamed semaphore
    if (sem_init(&barrier, 0, 0) < 0) {
        perror("Error creating the semaphore");
        exit(2);
    }

    // Create the N threads
    for (int i=1; i<=MAX_TH_P8; i++) {
    	th_args[i] = i;
        if(i == 11) {
            if (pthread_create(&th[i], NULL, thread_8_function_P11, &th_args[i]) != 0) {
                perror("Cannot create threads");
                exit(1);
            }
        } else {
            if (pthread_create(&th[i], NULL, thread_function_P8, &th_args[i]) != 0) {
                perror("Cannot create threads");
                exit(1);
            }
        }
    }

    // Wait for the termination of the N threads created
    for (int i=1; i<=MAX_TH_P8; i++) {
        pthread_join(th[i], NULL);
    }

    sem_destroy(&sem);
    sem_destroy(&barrier);
}

void* thread_function_P3(void* arg) {
    int th_id = *((int*) arg);

    // if(th_id == 6) {
    //     sem_wait(semT21_end);
    // }

    info(BEGIN, 3, th_id);

    info(END, 3, th_id);

    // if(th_id == 5) {
    //     sem_post(semT21_beg);
    // }

    return 0;
}

void* thread_5_function_P3(void* arg) {
    int th_id = *((int*) arg);

    info(BEGIN, 3, th_id);
    // printf("runnind %d\n", th_id);
    info(END, 3, th_id);
    sem_post(semT21_beg);

    return 0;
}


void* thread_6_function_P3(void* arg) {
    int th_id = *((int*) arg);

    sem_wait(semT21_end);
    info(BEGIN, 3, th_id);

    info(END, 3, th_id);

    return 0;
}

void create_threads_P3() {
    pthread_t th[MAX_TH_P3 + 1];
    int th_args[MAX_TH_P3 + 1];

    // Create named semaphores
    semT21_beg = sem_open("/semT21_beg\0", O_CREAT, 0644, 0);
    semT21_end = sem_open("/semT21_end\0", O_CREAT, 0644, 0);

    if(semT21_beg == SEM_FAILED){
        printf("Sem not initialized\n");
        return;
    }

    if(semT21_end == SEM_FAILED){
        printf("Sem not initialized\n");
        return;
    }

    // Create the N threads
    for (int i=1; i<=MAX_TH_P3; i++) {
    	th_args[i] = i;
        if(i == 5) {
            if (pthread_create(&th[i], NULL, thread_5_function_P3, &th_args[i]) != 0) {
                perror("Cannot create threads");
                exit(1);
            }
        } else if(i == 6) {
            if (pthread_create(&th[i], NULL, thread_6_function_P3, &th_args[i]) != 0) {
                perror("Cannot create threads");
                exit(1);
            }
        } else {
            if (pthread_create(&th[i], NULL, thread_function_P3, &th_args[i]) != 0) {
                perror("Cannot create threads");
                exit(1);
            }
        }
    }

    // Wait for the termination of the N threads created
    for (int i=1; i<=MAX_TH_P3; i++) {
        pthread_join(th[i], NULL);
    }

    sem_close(semT21_beg);
    sem_close(semT21_end);
}

int main(){
    init();

    info(BEGIN, 1, 0);

    pid_t pid2, pid3, pid4, pid5, pid6, pid7, pid8;

    // P2
    pid2 = fork();
    if(pid2 == 0) {
        info(BEGIN, 2, 0);
        // pid2 = getpid();
        create_threads_P2();
        info(END, 2, 0);
    } else {

        // P3
        pid3 = fork();
        if(pid3 == 0) {
            info(BEGIN, 3, 0);
            // pid3 = getpid();
            create_threads_P3();

            // P8
            pid8 = fork();
            if(pid8 == 0) {
                info(BEGIN, 8, 0);
                // pid8 = getpid();
                create_threads_P8();
                info(END, 8, 0);
            } else {
                waitpid(pid8, NULL, 0);
                info(END, 3, 0);
            }
        } else {

            // P4
            pid4 = fork();
            if(pid4 == 0) {
                info(BEGIN, 4, 0);
                // pid4 = getpid();

                // P6
                pid6 = fork();
                if(pid6 == 0) {
                    info(BEGIN, 6, 0);
                    // pid6 = getpid();
                    info(END, 6, 0);
                } else {
                    waitpid(pid6, NULL, 0);
                    info(END, 4, 0);
                }

            } else {

                // P5
                pid5 = fork();
                if(pid5 == 0) {
                    info(BEGIN, 5, 0);
                    // pid5 = getpid();
                    info(END, 5, 0);
                } else {

                    // P7
                    pid7 = fork();
                    if(pid7 == 0) {
                        info(BEGIN, 7, 0);
                        // pid7 = getpid();
                        info(END, 7, 0);
                    } else {
                        waitpid(pid2, NULL, 0);
                        waitpid(pid3, NULL, 0);
                        waitpid(pid4, NULL, 0);
                        waitpid(pid5, NULL, 0);
                        waitpid(pid7, NULL, 0);
                        info(END, 1, 0);
                    }
                }
            }
        }
    }
    sem_unlink("/semT21_beg\0");
    sem_unlink("/semT21_end\0");
    return 0;
}
