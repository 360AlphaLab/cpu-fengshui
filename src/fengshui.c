#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <syscall.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>


#define THREAD_NUM 16
#define CPU_NUM 8

int cpu_counter[CPU_NUM] = {0};
int race_cpu_counter[CPU_NUM] = {0};
int padding_cpu_counter[CPU_NUM] = {0};
int cpu_last_process[CPU_NUM] = {-1};
volatile int start = 0;
volatile int end = 0;

int getcpu() {
    #ifdef SYS_getcpu
    int cpu, status;
    status = syscall(SYS_getcpu, &cpu, NULL, NULL);
    return (status == -1) ? status : cpu;
    #else
    return -1; // unavailable
    #endif
}

void *padding_thread(void *arg)
{
    int id = (int)arg;
    volatile int counter = 0;
    int last_cpu = -1;
    // printf("padding_thread %d is running\n", (int)arg);
    while(!start) {} 
    while(!end) {
        for (int i=0; i<100000; i++) 
            counter ++;
        int cpu = getcpu();
        if ((id + 0x1000) != cpu_last_process[cpu]) {
            padding_cpu_counter[cpu] ++;
            cpu_last_process[cpu] = id + 0x1000;
        }
    }

    return NULL;
}

void *thread(void *arg)
{
    int id = (int)arg;
    int last_cpu = -1;
    char buf[100];
    int times;
    volatile int counter = 0;
    sprintf(buf, "io-%d.txt", id);
    int fd = open(buf, O_CREAT | O_WRONLY, S_IRWXU);
    // printf("fd is %d\n", fd);
    while(!start) {
        // printf("start %d\n", start);
    }
    
    while(!end) {
        int cpu = getcpu();
        if (id != cpu_last_process[cpu]) {
            race_cpu_counter[cpu] ++;
            cpu_last_process[cpu] = id;
        }
        
        int type = rand() % 2;
        switch(type) {
        case 0:
            // CPU intensive task
            for (int i=0; i<100000; i++) 
                counter ++;
            break;
        case 1:
            // IO intensive task
            times = rand() % 1000;
            sprintf(buf, "buf ptr is %p\n", buf);
            for (int i=0; i<times; i++) {
                write(fd, buf, strlen(buf));
            }
            break;
        }
    }

    close(fd);
    // printf("thread %d exit\n", id);
    return NULL;
}

void print_info(int *counter, int len, char *name, char *prefix)
{
    printf("==================== %s ===================\n", name);
    printf("%7s ", prefix);
    for (int i=0; i<len; i++) {
        printf("%5d ", i);
    }
    printf("%5s\n", "sum");
    int sum = 0;
    printf("%7s ", "count");
    for (int i=0; i<len; i++) {
        printf("%5d ", counter[i]);
        sum += counter[i];
    }
    printf("%5d\n\n", sum);
}

void test(int padding)
{
    memset(race_cpu_counter, 0, sizeof(race_cpu_counter));
    memset(padding_cpu_counter, 0, sizeof(padding_cpu_counter));
    pthread_t pth;
    for (int i=0; i<padding; i++) {
        pthread_create(&pth, NULL, padding_thread, (void *)i);
    }

    for (int i=0; i<THREAD_NUM; i++) {
        pthread_create(&pth, NULL, thread, (void *)i);
    }

    sleep(3);
    // printf("ready to start\n");
    end = 0;
    start = 1;
    sleep(10);
    start = 0;
    end = 1;
    // printf("ready to stop\n");
    sleep(1);
    printf("PADDING_THREAD_NUM %d\n", padding);
    print_info(race_cpu_counter, CPU_NUM, "race cpu switch", "cpu");
    print_info(padding_cpu_counter, CPU_NUM, "padding cpu switch", "cpu");
    for (int i=0; i<CPU_NUM; i++)
        cpu_counter[i] = race_cpu_counter[i] + padding_cpu_counter[i];
    print_info(cpu_counter, CPU_NUM, "total cpu switch", "cpu");

}

int main()
{
    for (int padding=0; padding<64; padding += 1) {
        for (int i=0; i<3; i++)
            test(padding);
    }
    return 0;
}
