#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>


#define PROCESS_COUNT 5
#define QUEUE_SIZE (PROCESS_COUNT + 1)
#define MAX_TIME 100
#define TIME_QUANTUM 3


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

typedef struct {
    Process** process;
    int capacity;
    int front, rear;
    int count;
} Process_Queue;

void init(Process_Queue* q, int initial_capacity) {
    q->process = malloc(sizeof(Process*) * initial_capacity);
    q->capacity = initial_capacity;
    q->front = 0;
    q->rear = 0;
    q->count = 0;
}

bool is_empty(Process_Queue* q) {
    return q->count == 0;
}

bool is_full(Process_Queue* q) {
    return q->count == q->capacity;
}

void expand_queue(Process_Queue* q) {
    int previous_capacity = q->capacity;
    int new_capacity = previous_capacity * 2;
    Process** new_process = malloc(sizeof(Process*) * new_capacity);

    int length = (previous_capacity + q->rear - q->front) % previous_capacity;

    for (int i = 0; i < length; i++) {
        new_process[i] = q->process[(q->front + i) % previous_capacity];
    }

    free(q->process);

    q->process = new_process;
    q->capacity = new_capacity;
    q->front = 0;
    q->rear = length;
    q->count = length;
}

void enqueue(Process_Queue* q, Process* p) {
    if (is_full(q)) {
        expand_queue(q);
    }
    
    q->process[q->rear] = p;
    q->rear = (q->rear + 1) % q->capacity;
    q->count++;
}

Process* dequeue(Process_Queue* q) {
    if (is_empty(q)) {
        return NULL;
    }

    Process* p = q->process[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->count--;

    return p;
}


Process_Queue ready_queue;
Process_Queue waiting_queue;

Process process_list[PROCESS_COUNT];

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

    free(ready_queue.process);
    free(waiting_queue.process);

    return 0;
}


void create_process() {
    for (int i = 0; i < PROCESS_COUNT; i++) {
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
    printf("PID  Priority  Arrival  CPU_Burst  IO_Request  IO_Burst\n");

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];

        printf("%3d  %8d  %7d  %9d  %10d  %8d\n", p->pid, p->priority, p->arrival_time, p->cpu_burst, p->io_request_time, p->io_burst);
    }
}

void initialization() {
    init(&ready_queue, QUEUE_SIZE);
    init(&waiting_queue, QUEUE_SIZE);

    for (int i = 0; i < MAX_TIME; i++) {
        gantt[i] = 0;
        gantt_io[i] = 0;
    }

    gantt_end = 0;

    for (int i = 0; i < PROCESS_COUNT; i++) {
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

// FCFS 스케줄링 알고리즘
void scheduling_FCFS() {
    int time = 0;
    int completed_process_count = 0;
    Process* executing_process = NULL;
    bool idle = true;
    
    while (completed_process_count < PROCESS_COUNT && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        // Process 도착 시 ready queue에 push
        for (int i = 0; i < PROCESS_COUNT; i++) {
            Process* p = &process_list[i];

            if (p->arrival_time == time) {
                enqueue(&ready_queue, p);
            }
        }

        // 실행 중인 process가 없을 때
        if (!executing_process) {
            executing_process = dequeue(&ready_queue);
            
            if (executing_process) {
                if (executing_process->start_time < 0) {
                    executing_process->start_time = time;
                }

                idle = false;
            }
            else {
                idle = true;
            }
        }

        // CPU 작업 처리
        if (executing_process) {
            executing_process->executed_time++;
            executing_process->remaining_cpu--;
            gantt[time] = executing_process->pid;

            // IO 발생 시 waiting queue에 push
            if (executing_process->executed_time == executing_process->io_request_time) {
                executing_process->remaining_io = executing_process->io_burst;
                enqueue(&waiting_queue, executing_process);
                executing_process = NULL;
                gantt_io[time] = 1;
                idle = false;
            }
            // CPU 작업 종료 시
            else if (executing_process->remaining_cpu == 0) {
                executing_process->completion_time = time + 1;
                completed_process_count++;
                executing_process = NULL;
                idle = false;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        // IO 작업 처리
        for (int i = 0; i < waiting_queue.count; i++) {
            Process* p = dequeue(&waiting_queue);

            if (p) {
                p->remaining_io--;

                // IO burst가 남았으면 waiting queue에 push
                if (p->remaining_io > 0) {
                    enqueue(&waiting_queue, p);
                }
                // IO 작업 종료 시 ready queue에 push
                else {
                    enqueue(&ready_queue, p);
                }
            }
            else {
                break;
            }

        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

// Non-Preemptive SJF 알고리즘
void scheduling_Non_Preemptive_SJF() {
    int time = 0;
    int completed_process_count = 0;
    Process* executing_process = NULL;
    bool idle = true;

    while (completed_process_count < PROCESS_COUNT && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < PROCESS_COUNT; i++) {
            Process* p = &process_list[i];

            if (p->arrival_time == time) {
                enqueue(&ready_queue, p);
            }
        }

        if (!executing_process) {
            if (!is_empty(&ready_queue)) {
                int shortest_index = ready_queue.front;
                int shortest_cpu_burst = ready_queue.process[shortest_index]->remaining_cpu;
                
                for (int k = 0; k < ready_queue.count; k++) {
                    int idx = (ready_queue.front + k) % ready_queue.capacity;
                    Process* p = ready_queue.process[idx];

                    if (p->remaining_cpu < shortest_cpu_burst) {
                        shortest_cpu_burst = p->remaining_cpu;
                        shortest_index = idx;
                    }
                }

                executing_process = ready_queue.process[shortest_index];

                int index = shortest_index;

                while (index != ready_queue.rear) {
                    int next = (index + 1) % ready_queue.capacity;
                    ready_queue.process[index] = ready_queue.process[next];
                    index = next;
                }

                if (ready_queue.rear == 0) {
                    ready_queue.rear = ready_queue.capacity - 1;
                }
                else {
                    ready_queue.rear--;
                }

                ready_queue.count--;

                if (executing_process->start_time < 0) {
                    executing_process->start_time = time;
                }

                idle = false;
            }
            else {
                idle = true;
            }
        }

        if (executing_process) {
            gantt[time] = executing_process->pid;
            executing_process->executed_time++;
            executing_process->remaining_cpu--;

            if (executing_process->executed_time == executing_process->io_request_time) {
                executing_process->remaining_io = executing_process->io_burst;
                enqueue(&waiting_queue, executing_process);
                executing_process = NULL;
                gantt_io[time] = 1;
                idle = false;
            }
            else if (executing_process->remaining_cpu == 0) {
                executing_process->completion_time = time + 1;
                completed_process_count++;
                executing_process = NULL;
                idle = false;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        for (int i = 0; i < waiting_queue.count; i++) {
            Process* p = dequeue(&waiting_queue);

            if (p) {
                if (--p->remaining_io > 0) {
                    enqueue(&waiting_queue, p);
                }
                else {
                    enqueue(&ready_queue, p);
                }
            }
            else {
                break;
            }
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

// Preemptive SJF 알고리즘
void scheduling_Preemptive_SJF() {
    int time = 0;
    int completed_process_count = 0;
    Process* executing_process = NULL;
    bool idle = true;

    while (completed_process_count < PROCESS_COUNT && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < PROCESS_COUNT; i++) {
            Process* p = &process_list[i];

            if (p->arrival_time == time) {
                enqueue(&ready_queue, p);
            }
        }

        if (!executing_process) {
            if (!is_empty(&ready_queue)) {
                int shortest_index = ready_queue.front;
                int shortest_cpu_burst = ready_queue.process[shortest_index]->remaining_cpu;

                for (int k = 0; k < ready_queue.count; k++) {
                    int idx = (ready_queue.front + k) % ready_queue.capacity;
                    Process* p = ready_queue.process[idx];

                    if (p->remaining_cpu < shortest_cpu_burst) {
                        shortest_cpu_burst = p->remaining_cpu;
                        shortest_index = idx;
                    }
                }

                executing_process = ready_queue.process[shortest_index];

                int index = shortest_index;

                while (index != ready_queue.rear) {
                    int next = (index + 1) % ready_queue.capacity;
                    ready_queue.process[index] = ready_queue.process[next];
                    index = next;
                }

                if (ready_queue.rear == 0) {
                    ready_queue.rear = ready_queue.capacity - 1;
                }
                else {
                    ready_queue.rear--;
                }

                ready_queue.count--;

                if (executing_process->start_time < 0) {
                    executing_process->start_time = time;
                }

                idle = false;
            }
            else {
                idle = true;
            }
        }

        if (executing_process) {
            gantt[time] = executing_process->pid;
            executing_process->executed_time++;
            executing_process->remaining_cpu--;

            if (executing_process->executed_time == executing_process->io_request_time) {
                executing_process->remaining_io = executing_process->io_burst;
                enqueue(&waiting_queue, executing_process);
                executing_process = NULL;
                gantt_io[time] = 1;
                idle = false;
            }
            else if (executing_process->remaining_cpu == 0) {
                executing_process->completion_time = time + 1;
                completed_process_count++;
                executing_process = NULL;
                idle = false;
            }
            else {
                enqueue(&ready_queue, executing_process);
                executing_process = NULL;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        for (int i = 0; i < waiting_queue.count; i++) {
            Process* p = dequeue(&waiting_queue);

            if (p) {
                if (--p->remaining_io > 0) {
                    enqueue(&waiting_queue, p);
                }
                else {
                    enqueue(&ready_queue, p);
                }
            }
            else {
                break;
            }
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];

        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

// Non-Preemptive Priority 알고리즘
void scheduling_Non_Preemptive_Priority() {
    int time = 0;
    int completed_process_count = 0;
    Process* executing_process = NULL;
    bool idle = true;

    while (completed_process_count < PROCESS_COUNT && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < PROCESS_COUNT; i++) {
            Process* p = &process_list[i];

            if (p->arrival_time == time) {
                enqueue(&ready_queue, p);
            }
        }

        if (!executing_process) {
            if (!is_empty(&ready_queue)) {
                int best_index = ready_queue.front;
                int best_priority = ready_queue.process[best_index]->priority;

                for (int k = 0; k < ready_queue.count; k++) {
                    int idx = (ready_queue.front + k) % ready_queue.capacity;
                    Process* p = ready_queue.process[idx];

                    if (p->priority > best_priority) {
                        best_priority = p->priority;
                        best_index = idx;
                    }
                }

                executing_process = ready_queue.process[best_index];

                int index = best_index;

                while (index != ready_queue.rear) {
                    int next = (index + 1) % ready_queue.capacity;
                    ready_queue.process[index] = ready_queue.process[next];
                    index = next;
                }

                if (ready_queue.rear == 0) {
                    ready_queue.rear = ready_queue.capacity - 1;
                }
                else {
                    ready_queue.rear--;
                }

                ready_queue.count--;

                if (executing_process->start_time < 0) {
                    executing_process->start_time = time;
                }

                idle = false;
            }
            else {
                idle = true;
            }
        }

        if (executing_process) {
            gantt[time] = executing_process->pid;
            executing_process->executed_time++;
            executing_process->remaining_cpu--;

            if (executing_process->executed_time == executing_process->io_request_time) {
                executing_process->remaining_io = executing_process->io_burst;
                enqueue(&waiting_queue, executing_process);
                executing_process = NULL;
                gantt_io[time] = 1;
                idle = false;
            }
            else if (executing_process->remaining_cpu == 0) {
                executing_process->completion_time = time + 1;
                completed_process_count++;
                executing_process = NULL;
                idle = false;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        for (int i = 0; i < waiting_queue.count; i++) {
            Process* p = dequeue(&waiting_queue);

            if (p) {
                if (--p->remaining_io > 0) {
                    enqueue(&waiting_queue, p);
                }
                else {
                    enqueue(&ready_queue, p);
                }
            }
            else {
                break;
            }
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];

        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

// Preemptive Priority 알고리즘
void scheduling_Preemptive_Priority() {
    int time = 0;
    int completed_process_count = 0;
    Process* executing_process = NULL;
    bool idle = true;

    while (completed_process_count < PROCESS_COUNT && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < PROCESS_COUNT; i++) {
            Process* p = &process_list[i];

            if (p->arrival_time == time) {
                enqueue(&ready_queue, p);
            }
        }

        if (!executing_process) {
            if (!is_empty(&ready_queue)) {
                int best_index = ready_queue.front;
                int best_priority = ready_queue.process[best_index]->priority;

                for (int k = 0; k < ready_queue.count; k++) {
                    int idx = (ready_queue.front + k) % ready_queue.capacity;
                    Process* p = ready_queue.process[idx];

                    if (p->priority > best_priority) {
                        best_priority = p->priority;
                        best_index = idx;
                    }
                }

                executing_process = ready_queue.process[best_index];

                int index = best_index;

                while (index != ready_queue.rear) {
                    int next = (index + 1) % ready_queue.capacity;
                    ready_queue.process[index] = ready_queue.process[next];
                    index = next;
                }

                if (ready_queue.rear == 0) {
                    ready_queue.rear = ready_queue.capacity - 1;
                }
                else {
                    ready_queue.rear--;
                }

                ready_queue.count--;

                if (executing_process->start_time < 0) {
                    executing_process->start_time = time;
                }

                idle = false;
            }
            else {
                idle = true;
            }
        }

        if (executing_process) {
            gantt[time] = executing_process->pid;
            executing_process->executed_time++;
            executing_process->remaining_cpu--;

            if (executing_process->executed_time == executing_process->io_request_time) {
                executing_process->remaining_io = executing_process->io_burst;
                enqueue(&waiting_queue, executing_process);
                executing_process = NULL;
                gantt_io[time] = 1;
                idle = false;
            }
            else if (executing_process->remaining_cpu == 0) {
                executing_process->completion_time = time + 1;
                completed_process_count++;
                executing_process = NULL;
                idle = false;
            }
            else {
                enqueue(&ready_queue, executing_process);
                executing_process = NULL;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        for (int i = 0; i < waiting_queue.count; i++) {
            Process* p = dequeue(&waiting_queue);

            if (p) {
                if (--p->remaining_io > 0) {
                    enqueue(&waiting_queue, p);
                }
                else {
                    enqueue(&ready_queue, p);
                }
            }
            else {
                break;
            }
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];

        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

// Round Robin 알고리즘
void scheduling_Round_Robin() {
    int time = 0;
    int completed_process_count = 0;
    Process* executing_process = NULL;
    bool idle = true;
    int quantum_count = 0;

    while (completed_process_count < PROCESS_COUNT && time < MAX_TIME) {
        gantt[time] = 0;
        gantt_io[time] = 0;

        for (int i = 0; i < PROCESS_COUNT; i++) {
            Process* p = &process_list[i];

            if (p->arrival_time == time) {
                enqueue(&ready_queue, p);
            }
        }

        if (!executing_process || quantum_count == TIME_QUANTUM) {
            if (executing_process) {
                enqueue(&ready_queue, executing_process);
                executing_process = NULL;
            }

            if (!is_empty(&ready_queue)) {
                executing_process = dequeue(&ready_queue);

                if (executing_process->start_time < 0) {
                    executing_process->start_time = time;
                }

                quantum_count = 0;
                idle = false;
            }
            else {
                idle = true;
            }
        }

        if (executing_process) {
            gantt[time] = executing_process->pid;
            executing_process->executed_time++;
            executing_process->remaining_cpu--;
            quantum_count++;

            if (executing_process->executed_time == executing_process->io_request_time) {
                executing_process->remaining_io = executing_process->io_burst;
                enqueue(&waiting_queue, executing_process);
                executing_process = NULL;
                gantt_io[time] = 1;
                idle = false;
                quantum_count = 0;
            }
            else if (executing_process->remaining_cpu == 0) {
                executing_process->completion_time = time + 1;
                completed_process_count++;
                executing_process = NULL;
                idle = false;
                quantum_count = 0;
            }
        }

        if (idle) {
            gantt[time] = 0;
        }

        for (int i = 0; i < waiting_queue.count; i++) {
            Process* p = dequeue(&waiting_queue);

            if (p) {
                if (--p->remaining_io > 0) {
                    enqueue(&waiting_queue, p);
                }
                else {
                    enqueue(&ready_queue, p);
                }
            }
            else {
                break;
            }
            
        }

        time++;
    }

    gantt_end = time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];

        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

void print_gantt() {
    int width = 10;

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

    for (int i = 0; i < PROCESS_COUNT; i++) {
        total_waiting_time += process_list[i].waiting_time;
        total_turnaround_time += process_list[i].turnaround_time;
    }

    double average_waiting_time = (double)total_waiting_time / PROCESS_COUNT;
    double average_turnaround_time = (double)total_turnaround_time / PROCESS_COUNT;

    printf("\nEvaluation: Average waiting time = %.2f, Average turnaround time = %.2f\n", average_waiting_time, average_turnaround_time);
}
