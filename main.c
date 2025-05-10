#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define MAX_PROCESS_COUNT 10
#define MAX_TIME_UNITS 200


typedef struct {
    int pid;
    int priority;
    int arrival_time;
    int cpu_burst;
    int io_burst;
    int io_request_time;
    int start_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int remaining_cpu;
    int remaining_io;
    int executed_time;
} Process;

Process process_list[MAX_PROCESS_COUNT];
int process_count = MAX_PROCESS_COUNT;

Process* ready_queue[MAX_PROCESS_COUNT];
int ready_front = 0;
int ready_rear = 0;
Process* waiting_queue[MAX_PROCESS_COUNT];
int waiting_front = 0;
int waiting_rear = 0;

int timeline[MAX_TIME_UNITS];
int timeline_end = 0;


void create_process();
void print_process_list();
void initialization();
void scheduling_FCFS();
void print_gantt();
void evaluation();


int main(void) {
    srand((unsigned)time(NULL));

    create_process();

    print_process_list();

    initialization();

    printf("\n=== FCFS Scheduling ===\n");
    scheduling_FCFS();

    print_gantt();
    evaluation();
    return 0;
}


void create_process() {
    for (int i = 0; i < process_count; i++) {
        Process* p = &process_list[i];
        p->pid = i + 1;
        p->priority = rand() % 5;
        p->arrival_time = rand() % 10;
        p->cpu_burst = (rand() % 10) + 1;
        p->io_burst = (rand() % 5) + 1;
        p->io_request_time = rand() % p->cpu_burst;
        p->start_time = -1;
        p->completion_time = 0;
        p->waiting_time = 0;
        p->turnaround_time = 0;
        p->remaining_cpu = p->cpu_burst;
        p->remaining_io = 0;
        p->executed_time = 0;
    }
}

void print_process_list() {
    printf("\nProcess List:\n");
    printf("PID  Priority  Arrival  CPU_Burst\n");

    for (int i = 0; i < process_count; i++) {
        Process* p = &process_list[i];
        printf("%3d  %8d  %7d  %9d\n",
            p->pid,
            p->priority,
            p->arrival_time,
            p->cpu_burst);
    }
}

void initialization() {
    ready_front = 0;
    ready_rear = 0;
    waiting_front = 0;
    waiting_rear = 0;

    for (int i = 0; i < MAX_TIME_UNITS; i++) {
        timeline[i] = 0;
    }
}

void scheduling_FCFS() {
    int time = 0;
    int completed_process_count = 0;
    Process* current = NULL;
    int idle = 1;
    
    while (completed_process_count < process_count && time < MAX_TIME_UNITS) {
        for (int i = 0; i < process_count; i++) {
            Process* p = &process_list[i];
            if (p->arrival_time == time) {
                ready_queue[ready_rear] = p;
                if (++ready_rear == MAX_PROCESS_COUNT) {
                    ready_rear = 0;
                }
            }
        }

        int len = (waiting_rear - waiting_front + MAX_PROCESS_COUNT) % MAX_PROCESS_COUNT;

        for (int i = 0; i < len; i++) {
            Process* p = waiting_queue[waiting_front];
            if (++waiting_front == MAX_PROCESS_COUNT) {
                waiting_front = 0;
            }
            p->remaining_io--;
            if (p->remaining_io > 0) {
                waiting_queue[waiting_rear] = p;
                if (++waiting_rear == MAX_PROCESS_COUNT) {
                    waiting_rear = 0;
                }
            }
            else {
                ready_queue[ready_rear] = p;
                if (++ready_rear == MAX_PROCESS_COUNT) {
                    ready_rear = 0;
                }
            }
        }

        if (!current) {
            if (ready_front != ready_rear) {
                current = ready_queue[ready_front];
                if (++ready_front == MAX_PROCESS_COUNT) {
                    ready_front = 0;
                }
                if (current->start_time < 0) {
                    current->start_time = time;
                }
                idle = 0;
            }
            else {
                idle = 1;
            }
        }

        if (current) {
            current->executed_time++;
            current->remaining_cpu--;
            timeline[time] = current->pid;
            if (current->executed_time == current->io_request_time) {
                current->remaining_io = current->io_burst;
                waiting_queue[waiting_rear] = current;
                if (++waiting_rear == MAX_PROCESS_COUNT) {
                    waiting_rear = 0;
                }
                current = NULL;
                idle = 0;
            }
            else if (current->remaining_cpu == 0) {
                current->completion_time = time + 1;
                completed_process_count++;
                current = NULL;
                idle = 0;
            }
        }

        if (idle) {
            timeline[time] = 0;
        }
        time++;
    }
    timeline_end = time;

    for (int i = 0; i < process_count; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst;
    }
}

void print_gantt() {
    const int width = 5;

    printf("\nGantt Chart:\nTime :");

    for (int t = 0; t <= timeline_end; t++) {
        printf("%*d", width, t);
}

    printf("\n ");
        for (int t = 0; t <= timeline_end; t++) {
            for (int i = 0; i < width; i++) printf("-");
        }

    printf("\nPID  :");
        for (int t = 0; t <= timeline_end; t++) {
            if (timeline[t] == 0) {
                printf("|%-*s", width - 1, "Idle");
            }
            else {
                char buffer[width + 1];
                snprintf(buffer, sizeof(buffer), "P%d", timeline[t]);
                printf("| %*s", width - 2, buffer);
            }
        }
    printf("|\n");
}

void evaluation() {
    int total_waiting_time = 0;
    int total_turnaround_time = 0;

    for (int i = 0; i < process_count; i++) {
        total_waiting_time += process_list[i].waiting_time;
        total_turnaround_time += process_list[i].turnaround_time;
    }

    double average_waiting_time = (double)total_waiting_time / process_count;
    double average_turnaround_time = (double)total_turnaround_time / process_count;

    printf("\nEvaluation: Average waiting time = %.2f, Average turnaround time = %.2f\n", 
        average_waiting_time, average_turnaround_time);
}
