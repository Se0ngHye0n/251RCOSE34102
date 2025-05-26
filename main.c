#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>


#define PROCESS_COUNT 5
#define QUEUE_SIZE (PROCESS_COUNT + 1)
#define MAX_TIME 100
#define TIME_QUANTUM 3
#define MAX_EVENTS 100


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
    int front;
    int rear;
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

    // queue가 비면 front, rear 초기화
    if (q->count == 0) {
        q->front = 0;
        q->rear = 0;
    }

    return p;
}

void dequeue_from_waiting_queue(Process_Queue* q, Process* p) {
    int idx = q->front;
    int target = -1;

    if (q->count == 0) {
        return;
    }

    // process p가 큐에 있는지 검사
    for (int i = 0; i < q->count; i++) {
        if (q->process[idx] == p) {
            target = idx;

            break;
        }

        idx = (idx + 1) % q->capacity;
    }

    if (target == -1) {
        return;
    }

    int index = target;

    // process p를 queue에서 제거
    while (index != q->rear) {
        int next = (index + 1) % q->capacity;
        q->process[index] = q->process[next];
        index = next;
    }

    // queue의 rear, count 조정
    if (q->rear == 0) {
        q->rear = q->capacity - 1;
    }
    else {
        q->rear--;
    }

    q->count--;

    // queue가 비면 front, rear 초기화
    if (q->count == 0) {
        q->front = 0;
        q->rear = 0;
    }
}


Process_Queue ready_queue;
Process_Queue waiting_queue;

Process process_list[PROCESS_COUNT];


typedef struct {
    int time;
    int type; // 1. Process Arrival  2. CPU Complete  3. IO Complete
    Process* p;
} Event;

static Event event_heap[MAX_EVENTS + 1];
static int event_count = 0;

void push_event(int time, int type, Process* p) {
    if (event_count >= MAX_EVENTS) {
        return;
    }

    int index = ++event_count;
    event_heap[index].time = time;
    event_heap[index].type = type;
    event_heap[index].p = p;

    while (index > 1) {
        if (event_heap[index].time >= event_heap[index / 2].time) {
            break;
        }
        else {
            Event tmp = event_heap[index / 2];
            event_heap[index / 2] = event_heap[index];
            event_heap[index] = tmp;
            index = index / 2;
        }
    }
}

Event pop_event() {
    Event e = event_heap[1];
    event_heap[1] = event_heap[event_count--];
    int index = 1;

    while (1) {
        int left = 2 * index;
        int right = 2 * index + 1;
        int smallest = index;

        if (left <= event_count && event_heap[left].time < event_heap[smallest].time) {
            smallest = left;
        }

        if (right <= event_count && event_heap[right].time < event_heap[smallest].time) {
            smallest = right;
        }

        if (smallest == index) {
            break;
        }
        else {
            Event tmp = event_heap[index];
            event_heap[index] = event_heap[smallest];
            event_heap[smallest] = tmp;
            index = smallest;
        }
    }
    return e;
}


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

        p->pid = i + 1; // 1 ~ 5
        p->priority = rand() % 5 + 1; // 1 ~ 5
        p->arrival_time = rand() % 10; // 0 ~ 9
        p->cpu_burst = (rand() % 9) + 2; // 2 ~ 10
        p->io_burst = (rand() % 5) + 1; // 1 ~ 5
        p->io_request_time = (rand() % (p->cpu_burst - 1)) + 1; // 1 ~ (cpu_burst - 1), cpu burst 중 1번만 실행
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

    event_count = 0;

    event_count = 0;

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

// FCFS 알고리즘
void scheduling_FCFS() {
    Process* executing_process = NULL;

    int completed_process_count = 0;
    int last_event_time = 0; // 마지막으로 처리한 event가 발생한 시점
    int last_run_start = 0; // executing_process가 마지막으로 실행된 시점

    // process를 도착 시간 순으로 event heap에 push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();

        int now = e.time;
        int to_io; // IO request까지 남은 시간
        int run; // CPU 작업을 수행할 시간

        // gantt chart 기록
        for (int t = last_event_time; t < now && t < MAX_TIME; t++) {
            if (executing_process) {
                gantt[t] = executing_process->pid;
            }
            else {
                gantt[t] = 0;
            }

            gantt_io[t] = 0;
        }

        last_event_time = now;

        do {
            Process* p = e.p;

            // 1. Process Arrival
            if (e.type == 1) {
                enqueue(&ready_queue, p);
            }
            // 2. CPU Complete
            else if (e.type == 2) {
                // 이미 preemption 되어 더 이상 실행 중이 아닌 경우
                if (p != executing_process) {
                    break;
                }

                // 가장 최근에 수행한 cpu 작업 시간
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request 발생 시
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // 현재(now) + 남은 io burst 이후에 실행되는 process로 push
                    push_event(now + p->remaining_io, 3, p);
                }
                // 완전히 종료된 process
                else {
                    p->completion_time = now;
                    completed_process_count++;
                }

                executing_process = NULL;
            }
            // 3. IO Complete
            else if (e.type == 3) {
                p->remaining_io = 0;
                dequeue_from_waiting_queue(&waiting_queue, p);
                enqueue(&ready_queue, p);
            }

            // 현재(now) 발생하는 event가 더 있으면 계속 처리
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // cpu 재할당
        if (!executing_process && !is_empty(&ready_queue)) {
            executing_process = dequeue(&ready_queue);

            // 처음 도착한 process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process가 IO 작업 이후 다시 CPU 작업을 수행하는 경우
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // process가 IO 작업을 아직 수행하지 않은 경우
            else {
                run = to_io;
            }

            // process가 수행해야 하는 작업이 남은 경우
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process를 완전히 종료하는 경우
            else {
                executing_process->completion_time = now;
                completed_process_count++;
                executing_process = NULL;
            }
        }
    }

    gantt_end = last_event_time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];

        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

// Non-Preemptive SJF 알고리즘
void scheduling_Non_Preemptive_SJF() {
    Process* executing_process = NULL;
    int completed_process_count = 0;
    int last_event_time = 0; // 마지막으로 처리한 event가 발생한 시점
    int last_run_start = 0; // executing_process가 마지막으로 실행된 시점

    // process를 도착 시간 순으로 event heap에 push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();
        int now = e.time;
        int to_io; // IO request까지 남은 시간
        int run; // CPU 작업을 수행할 시간

        // gantt chart 기록
        for (int t = last_event_time; t < now && t < MAX_TIME; t++) {
            if (executing_process) {
                gantt[t] = executing_process->pid;
            }
            else {
                gantt[t] = 0;
            }

            gantt_io[t] = 0;
        }

        last_event_time = now;

        do {
            Process* p = e.p;

            // 1. Process Arrival
            if (e.type == 1) {
                enqueue(&ready_queue, p);
            }
            // 2. CPU Complete
            else if (e.type == 2) {
                // 이미 preemption 되어 더 이상 실행 중이 아닌 경우
                if (p != executing_process) {
                    break;
                }

                // 가장 최근에 수행한 cpu 작업 시간
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request 발생 시
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // 현재(now) + 남은 io burst 이후에 실행되는 process로 push
                    push_event(now + p->remaining_io, 3, p);
                }
                // 완전히 종료된 process
                else {
                    p->completion_time = now;
                    completed_process_count++;
                }

                executing_process = NULL;
            }
            // 3. IO Complete
            else if (e.type == 3) {
                p->remaining_io = 0;
                dequeue_from_waiting_queue(&waiting_queue, p);
                enqueue(&ready_queue, p);
            }

            // 현재(now) 발생하는 event가 더 있으면 계속 처리
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // cpu 재할당
        if (!executing_process && !is_empty(&ready_queue)) {
            int shortest_index = ready_queue.front;
            int shortest_cpu_burst = ready_queue.process[shortest_index]->remaining_cpu;

            for (int k = 0; k < ready_queue.count; k++) {
                int idx = (ready_queue.front + k) % ready_queue.capacity;

                if (ready_queue.process[idx]->remaining_cpu < shortest_cpu_burst) {
                    shortest_cpu_burst = ready_queue.process[idx]->remaining_cpu;
                    shortest_index = idx;
                }
            }

            // shortest process에 cpu 할당
            executing_process = ready_queue.process[shortest_index];

            int index = shortest_index;

            // ready queue에서 제거
            while (index != ready_queue.rear) {
                int next = (index + 1) % ready_queue.capacity;
                ready_queue.process[index] = ready_queue.process[next];
                index = next;
            }

            // ready queue의 rear, count 조정
            if (ready_queue.rear == 0) {
                ready_queue.rear = ready_queue.capacity - 1;
            }
            else {
                ready_queue.rear--;
            }

            ready_queue.count--;

            // ready_queue가 비면 front, rear 초기화
            if (ready_queue.count == 0) {
                ready_queue.front = 0;
                ready_queue.rear = 0;
            }

            // 처음 도착한 process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process가 IO 작업 이후 다시 CPU 작업을 수행하는 경우
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // process가 IO 작업을 아직 수행하지 않은 경우
            else {
                run = to_io;
            }

            // process가 수행해야 하는 작업이 남은 경우
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process를 완전히 종료하는 경우
            else {
                executing_process->completion_time = now;
                completed_process_count++;
                executing_process = NULL;
            }
        }
    }

    gantt_end = last_event_time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

// Preemptive SJF 알고리즘
void scheduling_Preemptive_SJF() {
    Process* executing_process = NULL;
    int completed_process_count = 0;
    int last_event_time = 0; // 마지막으로 처리한 event가 발생한 시점
    int last_run_start = 0; // executing_process가 마지막으로 실행된 시점

    // process를 도착 시간 순으로 event heap에 push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();
        int now = e.time;
        int to_io; // IO request까지 남은 시간
        int run; // CPU 작업을 수행할 시간

        // gantt chart 기록
        for (int t = last_event_time; t < now && t < MAX_TIME; t++) {
            if (executing_process) {
                gantt[t] = executing_process->pid;
            }
            else {
                gantt[t] = 0;
            }

            gantt_io[t] = 0;
        }

        last_event_time = now;

        do {
            Process* p = e.p;

            // 1. Process Arrival
            if (e.type == 1) {
                enqueue(&ready_queue, p);
            }
            // 2. CPU Complete
            else if (e.type == 2) {
                // 이미 preemption 되어 더 이상 실행 중이 아닌 경우
                if (p != executing_process) {
                    break;
                }

                // 가장 최근에 수행한 cpu 작업 시간
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request 발생 시
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // 현재(now) + 남은 io burst 이후에 실행되는 process로 push
                    push_event(now + p->remaining_io, 3, p);
                }
                // preemption 발생 시
                else if (p->remaining_cpu > 0) {
                    enqueue(&ready_queue, p);
                }
                // 완전히 종료된 process
                else {
                    p->completion_time = now;
                    completed_process_count++;
                }

                executing_process = NULL;
            }
            // 3. IO Complete
            else if (e.type == 3) {
                p->remaining_io = 0;
                dequeue_from_waiting_queue(&waiting_queue, p);
                enqueue(&ready_queue, p);
            }

            // 현재(now) 발생하는 event가 더 있으면 계속 처리
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // preemption 발생 검사
        if (executing_process && !is_empty(&ready_queue)) {
            executing_process->executed_time += now - last_run_start;

            int shortest_index = ready_queue.front;
            int shortest_cpu_burst = ready_queue.process[shortest_index]->remaining_cpu;

            // shortest process 탐색
            for (int k = 0; k < ready_queue.count; k++) {
                int idx = (ready_queue.front + k) % ready_queue.capacity;

                if (ready_queue.process[idx]->remaining_cpu < shortest_cpu_burst) {
                    shortest_cpu_burst = ready_queue.process[idx]->remaining_cpu;
                    shortest_index = idx;
                }
            }

            // ready queue의 shortest보다 길면 실행 중인 process는 ready queue에 push
            if (shortest_cpu_burst < executing_process->remaining_cpu) {
                enqueue(&ready_queue, executing_process);

                executing_process = NULL;

                last_run_start = now;
            }
        }

        // cpu 재할당
        if (!executing_process && !is_empty(&ready_queue)) {
            int shortest_index = ready_queue.front;
            int shortest_cpu_burst = ready_queue.process[shortest_index]->remaining_cpu;

            // shortest process 탐색
            for (int k = 0; k < ready_queue.count; k++) {
                int idx = (ready_queue.front + k) % ready_queue.capacity;

                if (ready_queue.process[idx]->remaining_cpu < shortest_cpu_burst) {
                    shortest_cpu_burst = ready_queue.process[idx]->remaining_cpu;
                    shortest_index = idx;
                }
            }

            // shortest process에 cpu 할당
            executing_process = ready_queue.process[shortest_index];

            int index = shortest_index;

            // ready queue에서 제거
            while (index != ready_queue.rear) {
                int next = (index + 1) % ready_queue.capacity;
                ready_queue.process[index] = ready_queue.process[next];
                index = next;
            }

            // ready queue의 rear, count 조정
            if (ready_queue.rear == 0) {
                ready_queue.rear = ready_queue.capacity - 1;
            }
            else {
                ready_queue.rear--;
            }

            ready_queue.count--;

            // ready_queue가 비면 front, rear 초기화
            if (ready_queue.count == 0) {
                ready_queue.front = 0;
                ready_queue.rear = 0;
            }

            // 처음 도착한 process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process가 IO 작업 이후 다시 CPU 작업을 수행하는 경우
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // process가 IO 작업을 아직 수행하지 않은 경우
            else {
                run = to_io;
            }

            // process가 수행해야 하는 작업이 남은 경우
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process를 완전히 종료하는 경우
            else {
                executing_process->completion_time = now;
                completed_process_count++;
                executing_process = NULL;
            }
        }
    }

    gantt_end = last_event_time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

// Non-Preemptive Priority 알고리즘
void scheduling_Non_Preemptive_Priority() {
    Process* executing_process = NULL;
    int completed_process_count = 0;
    int last_event_time = 0; // 마지막으로 처리한 event가 발생한 시점
    int last_run_start = 0; // executing_process가 마지막으로 실행된 시점

    // process를 도착 시간 순으로 event heap에 push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();
        int now = e.time;
        int to_io; // IO request까지 남은 시간
        int run; // CPU 작업을 수행할 시간

        // gantt chart 기록
        for (int t = last_event_time; t < now && t < MAX_TIME; t++) {
            if (executing_process) {
                gantt[t] = executing_process->pid;
            }
            else {
                gantt[t] = 0;
            }

            gantt_io[t] = 0;
        }

        last_event_time = now;

        do {
            Process* p = e.p;

            // 1. Process Arrival
            if (e.type == 1) {
                enqueue(&ready_queue, p);
            }
            // 2. CPU Complete
            else if (e.type == 2) {
                // 이미 preemption 되어 더 이상 실행 중이 아닌 경우
                if (p != executing_process) {
                    break;
                }

                // 가장 최근에 수행한 cpu 작업 시간
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request 발생 시
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // 현재(now) + 남은 io burst 이후에 실행되는 process로 push
                    push_event(now + p->remaining_io, 3, p);
                }
                // 완전히 종료된 process
                else {
                    p->completion_time = now;
                    completed_process_count++;
                }

                executing_process = NULL;
            }
            // 3. IO Complete
            else if (e.type == 3) {
                p->remaining_io = 0;
                dequeue_from_waiting_queue(&waiting_queue, p);
                enqueue(&ready_queue, p);
            }

            // 현재(now) 발생하는 event가 더 있으면 계속 처리
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // cpu 재할당
        if (!executing_process && !is_empty(&ready_queue)) {
            int best_index = ready_queue.front;
            int best_priority = ready_queue.process[best_index]->priority;

            for (int k = 0; k < ready_queue.count; k++) {
                int idx = (ready_queue.front + k) % ready_queue.capacity;

                if (ready_queue.process[idx]->priority > best_priority) {
                    best_priority = ready_queue.process[idx]->priority;
                    best_index = idx;
                }
            }

            // best priority에 cpu 할당
            executing_process = ready_queue.process[best_index];

            int idx = best_index;

            // ready queue에서 제거
            while (idx != ready_queue.rear) {
                int next = (idx + 1) % ready_queue.capacity;
                ready_queue.process[idx] = ready_queue.process[next];
                idx = next;
            }

            // ready queue의 rear, count 조정
            if (ready_queue.rear == 0) {
                ready_queue.rear = ready_queue.capacity - 1;
            }
            else {
                ready_queue.rear--;
            }

            ready_queue.count--;

            // ready_queue가 비면 front, rear 초기화
            if (ready_queue.count == 0) {
                ready_queue.front = 0;
                ready_queue.rear = 0;
            }

            // 처음 도착한 process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process가 IO 작업 이후 다시 CPU 작업을 수행하는 경우
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // process가 IO 작업을 아직 수행하지 않은 경우
            else {
                run = to_io;
            }

            // process가 수행해야 하는 작업이 남은 경우
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process를 완전히 종료하는 경우
            else {
                executing_process->completion_time = now;
                completed_process_count++;
                executing_process = NULL;
            }

        }
    }

    gantt_end = last_event_time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

// Preemptive Priority 알고리즘
void scheduling_Preemptive_Priority() {
    Process* executing_process = NULL;
    int completed_process_count = 0;
    int last_event_time = 0; // 마지막으로 처리한 event가 발생한 시점
    int last_run_start = 0; // executing_process가 마지막으로 실행된 시점

    // process를 도착 시간 순으로 event heap에 push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();
        int now = e.time;
        int to_io; // IO request까지 남은 시간
        int run; // CPU 작업을 수행할 시간

        // gantt chart 기록
        for (int t = last_event_time; t < now && t < MAX_TIME; t++) {
            if (executing_process) {
                gantt[t] = executing_process->pid;
            }
            else {
                gantt[t] = 0;
            }

            gantt_io[t] = 0;
        }

        last_event_time = now;

        do {
            Process* p = e.p;

            // 1. Process Arrival
            if (e.type == 1) {
                enqueue(&ready_queue, p);
            }
            // 2. CPU Complete
            else if (e.type == 2) {
                // 이미 preemption 되어 더 이상 실행 중이 아닌 경우
                if (p != executing_process) {
                    break;
                }

                // 가장 최근에 수행한 cpu 작업 시간
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request 발생 시
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // 현재(now) + 남은 io burst 이후에 실행되는 process로 push
                    push_event(now + p->remaining_io, 3, p);
                }
                // preemption 발생 시
                else if (p->remaining_cpu > 0) {
                    enqueue(&ready_queue, p);
                }
                // 완전히 종료된 process
                else {
                    p->completion_time = now;
                    completed_process_count++;
                }

                executing_process = NULL;
            }
            // 3. IO Complete
            else if (e.type == 3) {
                p->remaining_io = 0;
                dequeue_from_waiting_queue(&waiting_queue, p);
                enqueue(&ready_queue, p);
            }

            // 현재(now) 발생하는 event가 더 있으면 계속 처리
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // preemption 발생 검사
        if (executing_process && !is_empty(&ready_queue)) {
            executing_process->executed_time += now - last_run_start;

            int best_index = ready_queue.front;
            int best_priority = ready_queue.process[best_index]->priority;

            // best priority 탐색
            for (int k = 0; k < ready_queue.count; k++) {
                int idx = (ready_queue.front + k) % ready_queue.capacity;

                if (ready_queue.process[idx]->priority > best_priority) {
                    best_priority = ready_queue.process[idx]->priority;
                    best_index = idx;
                }
            }

            // ready queue의 shortest보다 길면 실행 중인 process는 ready queue에 push
            if (best_priority > executing_process->priority) {
                enqueue(&ready_queue, executing_process);

                executing_process = NULL;

                last_run_start = now;
            }
        }

        // cpu 재할당
        if (!executing_process && !is_empty(&ready_queue)) {
            int best_index = ready_queue.front;
            int best_priority = ready_queue.process[best_index]->priority;

            for (int k = 0; k < ready_queue.count; k++) {
                int idx = (ready_queue.front + k) % ready_queue.capacity;

                if (ready_queue.process[idx]->priority > best_priority) {
                    best_priority = ready_queue.process[idx]->priority;
                    best_index = idx;
                }
            }

            // best priority에 cpu 할당
            executing_process = ready_queue.process[best_index];

            int idx = best_index;

            // ready queue에서 제거
            while (idx != ready_queue.rear) {
                int next = (idx + 1) % ready_queue.capacity;
                ready_queue.process[idx] = ready_queue.process[next];
                idx = next;
            }

            // ready queue의 rear, count 조정
            if (ready_queue.rear == 0) {
                ready_queue.rear = ready_queue.capacity - 1;
            }
            else {
                ready_queue.rear--;
            }

            ready_queue.count--;

            // ready_queue가 비면 front, rear 초기화
            if (ready_queue.count == 0) {
                ready_queue.front = 0;
                ready_queue.rear = 0;
            }

            // 처음 도착한 process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process가 IO 작업 이후 다시 CPU 작업을 수행하는 경우
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // process가 IO 작업을 아직 수행하지 않은 경우
            else {
                run = to_io;
            }

            // process가 수행해야 하는 작업이 남은 경우
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process를 완전히 종료하는 경우
            else {
                executing_process->completion_time = now;
                completed_process_count++;
                executing_process = NULL;
            }

        }
    }

    gantt_end = last_event_time;

    for (int i = 0; i < PROCESS_COUNT; i++) {
        Process* p = &process_list[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->cpu_burst - p->io_burst;
    }
}

// Round Robin 알고리즘
void scheduling_Round_Robin() {
    Process* executing_process = NULL;
    int completed_process_count = 0;
    int last_event_time = 0; // 마지막으로 처리한 event가 발생한 시점
    int last_run_start = 0; // executing_process가 마지막으로 실행된 시점

    // process를 도착 시간 순으로 event heap에 push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();
        int now = e.time;
        int to_io; // IO request까지 남은 시간
        int run; // CPU 작업을 수행할 시간

        // gantt chart 기록
        for (int t = last_event_time; t < now && t < MAX_TIME; t++) {
            if (executing_process) {
                gantt[t] = executing_process->pid;
            }
            else {
                gantt[t] = 0;
            }

            gantt_io[t] = 0;
        }

        last_event_time = now;

        do {
            Process* p = e.p;

            // 1. Process Arrival
            if (e.type == 1) {
                enqueue(&ready_queue, p);
            }
            // 2. CPU Complete
            else if (e.type == 2) {
                // 이미 preemption 되어 더 이상 실행 중이 아닌 경우
                if (p != executing_process) {
                    break;
                }

                // 가장 최근에 수행한 cpu 작업 시간
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request 발생 시
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // 현재(now) + 남은 io burst 이후에 실행되는 process로 push
                    push_event(now + p->remaining_io, 3, p);
                }
                // preemption 발생 시
                else if (p->remaining_cpu > 0) {
                    enqueue(&ready_queue, p);
                }
                // 완전히 종료된 process
                else {
                    p->completion_time = now;
                    completed_process_count++;
                }

                executing_process = NULL;
            }
            // 3. IO Complete
            else if (e.type == 3) {
                p->remaining_io = 0;
                dequeue_from_waiting_queue(&waiting_queue, p);
                enqueue(&ready_queue, p);
            }

            // 현재(now) 발생하는 event가 더 있으면 계속 처리
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // cpu 재할당
        if (!executing_process && !is_empty(&ready_queue)) {
            executing_process = dequeue(&ready_queue);

            // 처음 도착한 process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process가 IO 작업 이후 다시 CPU 작업을 수행하는 경우
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // IO request가 TIME QUANTUM 전에 발생할 때
            else if (to_io < TIME_QUANTUM) {
                run = to_io;
            }
            // TIME QUANTUM만큼 실행, 만약 남은 cpu burst가 더 작으면 남은 cpu burst만큼 실행
            else {
                run = TIME_QUANTUM;

                if (executing_process->remaining_cpu < run) {
                    run = executing_process->remaining_cpu;
                }
            }

            // process가 수행해야 하는 작업이 남은 경우
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process를 완전히 종료하는 경우
            else {
                executing_process->completion_time = now;
                completed_process_count++;
                executing_process = NULL;
            }
        }
    }

    gantt_end = last_event_time;

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
        for (int i = 0; i < width; i++) {
            printf("-");
        }
    }

    printf("\nPID  :");
    for (int t = 0; t <= gantt_end; t++) {
        // Idle 상태
        if (gantt[t] == 0) {
            printf("|%*s", width - 1, "Idle");
        }
        // I/O request 되기 직전 실행
        else if (gantt_io[t]) {
            char buffer[width + 1];
            snprintf(buffer, sizeof(buffer), "P%d(I/O)", gantt[t]);
            printf("| %*s", width - 2, buffer);
        }
        // 실행 중인 process
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
