#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define PROCESS_COUNT 5
#define QUEUE_SIZE (PROCESS_COUNT + 1)
#define MAX_TIME 100
#define TIME_QUANTUM 4


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

Process process_list[PROCESS_COUNT];
int process_count = PROCESS_COUNT;

Process* ready_queue[QUEUE_SIZE];
int ready_front = 0;
int ready_rear = 0;

Process* waiting_queue[QUEUE_SIZE];
int waiting_front = 0;
int waiting_rear = 0;

int gantt[MAX_TIME];
int gantt_io[MAX_TIME];
int gantt_end = 0;


void create_process();
void print_process_list();
void initialization();
void scheduling_FCFS();
void scheduling_Non_Preemptive_SJF();
void scheduling_Preemptive_SJF();
void scheduling_Non_Preemptive_Priority();
void scheduling_Preemptive_Priority();
void scheduling_Round_Robin();
void print_gantt();
void evaluation();


int main(void) {
    srand(time(NULL));

    create_process();

    print_process_list();

    initialization();
    printf("\nFCFS Scheduling\n");
    scheduling_FCFS();
    print_gantt();
    evaluation();

    initialization();
    printf("\nNon-Preemptive SJF Scheduling\n");
    scheduling_Non_Preemptive_SJF();
    print_gantt();
    evaluation();

    initialization();
    printf("\nPreemptive SJF Scheduling\n");
    scheduling_Preemptive_SJF();
    print_gantt();
    evaluation();

    initialization();
    printf("\nNon-Preemptive Priority Scheduling\n");
    scheduling_Non_Preemptive_Priority();
    print_gantt();
    evaluation();

    initialization();
    printf("\nPreemptive Priority Scheduling\n");
    scheduling_Preemptive_Priority();
    print_gantt();
    evaluation();

    initialization();
    printf("\nRound Robin Scheduling\n");
    scheduling_Round_Robin();
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
        p->cpu_burst = (rand() % 9) + 2;
        p->io_burst = (rand() % 5) + 1;
        p->io_request_time = (rand() % (p->cpu_burst - 1)) + 1;
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

    for (int i = 0; i < MAX_TIME; i++) {
        gantt[i] = 0;
        gantt_io[i] = 0;
    }

    gantt_end = 0;

    for (int i = 0; i < process_count; i++) {
        Process* p = &process_list[i];
        p->remaining_cpu = p->cpu_burst;
        p->remaining_io = 0;
        p->executed_time = 0;
        p->start_time = -1;
        p->completion_time = 0;
        p->waiting_time = 0;
        p->turnaround_time = 0;
    }
}

void scheduling_FCFS() {
    int time = 0;
    int completed_process_count = 0;
    Process* current = NULL;
    int idle = 1;

    while (completed_process_count < process_count && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < process_count; i++) {
            Process* p = &process_list[i];
            if (p->arrival_time == time) {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        int waiting_length = (waiting_rear - waiting_front + QUEUE_SIZE) % QUEUE_SIZE;

        for (int i = 0; i < waiting_length; i++) {
            Process* p = waiting_queue[waiting_front];
            if (++waiting_front == QUEUE_SIZE) {
                waiting_front = 0;
            }
            p->remaining_io--;
            if (p->remaining_io > 0) {
                waiting_queue[waiting_rear] = p;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
            }
            else {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        if (!current) {
            if (ready_front != ready_rear) {
                current = ready_queue[ready_front];
                if (++ready_front == QUEUE_SIZE) {
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
            gantt[time] = current->pid;

            if (current->executed_time == current->io_request_time) {
                current->remaining_io = current->io_burst;
                waiting_queue[waiting_rear] = current;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
                current = NULL;
                gantt_io[time] = 1;
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
            gantt[time] = 0;
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < process_count; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->start_time - p->arrival_time;
    }
}

void scheduling_Non_Preemptive_SJF() {
    int time = 0;
    int completed_count = 0;
    Process* current = NULL;
    int idle = 1;

    while (completed_count < process_count && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < process_count; i++) {
            Process* p = &process_list[i];
            if (p->arrival_time == time) {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        int waiting_length = (waiting_rear - waiting_front + QUEUE_SIZE) % QUEUE_SIZE;
        for (int i = 0; i < waiting_length; i++) {
            Process* p = waiting_queue[waiting_front];
            if (++waiting_front == QUEUE_SIZE) {
                waiting_front = 0;
            }
            if (--p->remaining_io > 0) {
                waiting_queue[waiting_rear] = p;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
            }
            else {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        if (!current) {
            if (ready_front != ready_rear) {
                int idx = ready_front;
                int shortest_process = idx;
                int min_cpu_burst = ready_queue[idx]->remaining_cpu;

                int ready_length = (ready_rear - ready_front + QUEUE_SIZE) % QUEUE_SIZE;
                for (int k = 0; k < ready_length; k++) {
                    Process* p = ready_queue[(ready_front + k) % QUEUE_SIZE];
                    if (p->remaining_cpu < min_cpu_burst) {
                        min_cpu_burst = p->remaining_cpu;
                        shortest_process = (ready_front + k) % QUEUE_SIZE;
                    }
                }

                current = ready_queue[shortest_process];

                for (int k = shortest_process; k != ready_rear; k = (k + 1) % QUEUE_SIZE) {
                    int next = (k + 1) % QUEUE_SIZE;
                    ready_queue[k] = ready_queue[next];
                }

                if (ready_rear == 0) {
                    ready_rear = QUEUE_SIZE - 1;
                }
                else {
                    ready_rear--;
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
            gantt[time] = current->pid;
            current->executed_time++;
            current->remaining_cpu--;
            if (current->executed_time == current->io_request_time) {
                current->remaining_io = current->io_burst;
                waiting_queue[waiting_rear] = current;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
                current = NULL;
                gantt_io[time] = 1;
                idle = 0;
            }
            else if (current->remaining_cpu == 0) {
                current->completion_time = time + 1;
                completed_count++;
                current = NULL;
                idle = 0;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < process_count; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->start_time - p->arrival_time;
    }
}

void scheduling_Preemptive_SJF() {
    int time = 0;
    int completed_count = 0;
    Process* current = NULL;
    int idle = 1;

    while (completed_count < process_count && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < process_count; i++) {
            Process* p = &process_list[i];
            if (p->arrival_time == time) {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        int waiting_length = (waiting_rear - waiting_front + QUEUE_SIZE) % QUEUE_SIZE;
        for (int i = 0; i < waiting_length; i++) {
            Process* p = waiting_queue[waiting_front];
            if (++waiting_front == QUEUE_SIZE) {
                waiting_front = 0;
            }
            if (--p->remaining_io > 0) {
                waiting_queue[waiting_rear] = p;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
            }
            else {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        if (!current) {
            if (ready_front != ready_rear) {
                int idx = ready_front;
                int shortest_process = idx;
                int min_cpu_burst = ready_queue[idx]->remaining_cpu;

                int ready_length = (ready_rear - ready_front + QUEUE_SIZE) % QUEUE_SIZE;
                for (int k = 0; k < ready_length; k++) {
                    Process* p = ready_queue[(ready_front + k) % QUEUE_SIZE];
                    if (p->remaining_cpu < min_cpu_burst) {
                        min_cpu_burst = p->remaining_cpu;
                        shortest_process = (ready_front + k) % QUEUE_SIZE;
                    }
                }

                current = ready_queue[shortest_process];

                for (int k = shortest_process; k != ready_rear; k = (k + 1) % QUEUE_SIZE) {
                    int next = (k + 1) % QUEUE_SIZE;
                    ready_queue[k] = ready_queue[next];
                }

                if (ready_rear == 0) {
                    ready_rear = QUEUE_SIZE - 1;
                }
                else {
                    ready_rear--;
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
            gantt[time] = current->pid;
            current->executed_time++;
            current->remaining_cpu--;
            if (current->executed_time == current->io_request_time) {
                current->remaining_io = current->io_burst;
                waiting_queue[waiting_rear] = current;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
                current = NULL;
                gantt_io[time] = 1;
                idle = 0;
            }
            else if (current->remaining_cpu == 0) {
                current->completion_time = time + 1;
                completed_count++;
                current = NULL;
                idle = 0;
            }
            else {
                ready_queue[ready_rear] = current;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
                current = NULL;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < process_count; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->start_time - p->arrival_time;
    }
}

void scheduling_Non_Preemptive_Priority() {
    int time = 0;
    int completed_count = 0;
    Process* current = NULL;
    int idle = 1;

    while (completed_count < process_count && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < process_count; i++) {
            Process* p = &process_list[i];
            if (p->arrival_time == time) {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        int waiting_length = (waiting_rear - waiting_front + QUEUE_SIZE) % QUEUE_SIZE;
        for (int i = 0; i < waiting_length; i++) {
            Process* p = waiting_queue[waiting_front];
            if (++waiting_front == QUEUE_SIZE) {
                waiting_front = 0;
            }
            if (--p->remaining_io > 0) {
                waiting_queue[waiting_rear] = p;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
            }
            else {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        if (!current) {
            if (ready_front != ready_rear) {
                int idx = ready_front;
                int selected_process = idx;
                int best_priority = ready_queue[idx]->priority;

                int ready_length = (ready_rear - ready_front + QUEUE_SIZE) % QUEUE_SIZE;
                for (int k = 0; k < ready_length; k++) {
                    Process* p = ready_queue[(ready_front + k) % QUEUE_SIZE];
                    if (p->priority > best_priority) {
                        best_priority = p->priority;
                        selected_process = (ready_front + k) % QUEUE_SIZE;
                    }
                }

                current = ready_queue[selected_process];

                for (int k = selected_process; k != ready_rear; k = (k + 1) % QUEUE_SIZE) {
                    int next = (k + 1) % QUEUE_SIZE;
                    ready_queue[k] = ready_queue[next];
                }

                if (ready_rear == 0) {
                    ready_rear = QUEUE_SIZE - 1;
                }
                else {
                    ready_rear--;
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
            gantt[time] = current->pid;
            current->executed_time++;
            current->remaining_cpu--;
            if (current->executed_time == current->io_request_time) {
                current->remaining_io = current->io_burst;
                waiting_queue[waiting_rear] = current;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
                current = NULL;
                gantt_io[time] = 1;
                idle = 0;
            }
            else if (current->remaining_cpu == 0) {
                current->completion_time = time + 1;
                completed_count++;
                current = NULL;
                idle = 0;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < process_count; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->start_time - p->arrival_time;
    }
}

void scheduling_Preemptive_Priority() {
    int time = 0;
    int completed_count = 0;
    Process* current = NULL;
    int idle = 1;

    while (completed_count < process_count && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < process_count; i++) {
            Process* p = &process_list[i];
            if (p->arrival_time == time) {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        int waiting_length = (waiting_rear - waiting_front + QUEUE_SIZE) % QUEUE_SIZE;
        for (int i = 0; i < waiting_length; i++) {
            Process* p = waiting_queue[waiting_front];
            if (++waiting_front == QUEUE_SIZE) {
                waiting_front = 0;
            }
            if (--p->remaining_io > 0) {
                waiting_queue[waiting_rear] = p;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
            }
            else {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        if (!current) {
            if (ready_front != ready_rear) {
                int idx = ready_front;
                int selected_process = idx;
                int best_priority = ready_queue[idx]->priority;

                int ready_length = (ready_rear - ready_front + QUEUE_SIZE) % QUEUE_SIZE;
                for (int k = 0; k < ready_length; k++) {
                    Process* p = ready_queue[(ready_front + k) % QUEUE_SIZE];
                    if (p->priority > best_priority) {
                        best_priority = p->priority;
                        selected_process = (ready_front + k) % QUEUE_SIZE;
                    }
                }

                current = ready_queue[selected_process];

                for (int k = selected_process; k != ready_rear; k = (k + 1) % QUEUE_SIZE) {
                    int next = (k + 1) % QUEUE_SIZE;
                    ready_queue[k] = ready_queue[next];
                }

                if (ready_rear == 0) {
                    ready_rear = QUEUE_SIZE - 1;
                }
                else {
                    ready_rear--;
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
            gantt[time] = current->pid;
            current->executed_time++;
            current->remaining_cpu--;

            if (current->executed_time == current->io_request_time) {
                current->remaining_io = current->io_burst;
                waiting_queue[waiting_rear] = current;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
                current = NULL;
                gantt_io[time] = 1;
                idle = 0;
            }
            else if (current->remaining_cpu == 0) {
                current->completion_time = time + 1;
                completed_count++;
                current = NULL;
                idle = 0;
            }
            else {
                ready_queue[ready_rear] = current;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
                current = NULL;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < process_count; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->start_time - p->arrival_time;
    }
}

void scheduling_Round_Robin() {
    int time = 0;
    int completed_count = 0;
    Process* current = NULL;
    int idle = 1;
    int quantum_count = 0;

    while (completed_count < process_count && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < process_count; i++) {
            Process* p = &process_list[i];
            if (p->arrival_time == time) {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        int waiting_length = (waiting_rear - waiting_front + QUEUE_SIZE) % QUEUE_SIZE;
        for (int i = 0; i < waiting_length; i++) {
            Process* p = waiting_queue[waiting_front];
            if (++waiting_front == QUEUE_SIZE) {
                waiting_front = 0;
            }
            if (--p->remaining_io > 0) {
                waiting_queue[waiting_rear] = p;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }
            }
            else {
                ready_queue[ready_rear] = p;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
            }
        }

        if (!current || quantum_count == TIME_QUANTUM) {
            if (current) {
                ready_queue[ready_rear] = current;
                if (++ready_rear == QUEUE_SIZE) {
                    ready_rear = 0;
                }
                current = NULL;
            }

            if (ready_front != ready_rear) {
                current = ready_queue[ready_front];
                if (++ready_front == QUEUE_SIZE) {
                    ready_front = 0;
                }
                if (current->start_time < 0) {
                    current->start_time = time;
                }

                quantum_count = 0;
                idle = 0;
            }
            else {
                idle = 1;
            }
        }

        if (current) {
            gantt[time] = current->pid;
            current->executed_time++;
            current->remaining_cpu--;
            quantum_count++;

            if (current->executed_time == current->io_request_time) {
                current->remaining_io = current->io_burst;
                waiting_queue[waiting_rear] = current;
                if (++waiting_rear == QUEUE_SIZE) {
                    waiting_rear = 0;
                }

                current = NULL;
                gantt_io[time] = 1;
                idle = 0;
                quantum_count = 0;
            }
            else if (current->remaining_cpu == 0) {
                current->completion_time = time + 1;
                completed_count++;
                current = NULL;
                idle = 0;
                quantum_count = 0;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < process_count; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->start_time - p->arrival_time;
    }
}

void print_gantt() {
    const int width = 10;

    printf("\nGantt Chart:\nTime :");

    for (int t = 0; t <= gantt_end; t++) {
        printf("%*d", width, t);
    }

    printf("\n ");
    for (int t = 0; t <= gantt_end; t++) {
        for (int i = 0; i < width; i++) printf("-");
    }

    printf("\nPID  :");
    for (int t = 0; t <= gantt_end; t++) {
        if (gantt[t] == 0) {
            printf("|%*s", width - 1, "Idle");
        }
        else if (gantt_io[t]) {
            char buffer[width + 1];
            snprintf(buffer, sizeof(buffer), "P%d(I/O)", gantt[t]);
            printf("| %*s", width - 2, buffer);
        }
        else {
            char buffer[width + 1];
            snprintf(buffer, sizeof(buffer), "P%d", gantt[t]);
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

    printf("\nEvaluation: Average waiting time = %.2f, Average turnaround time = %.2f\n", average_waiting_time, average_turnaround_time);
}
