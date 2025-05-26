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

    // queue�� ��� front, rear �ʱ�ȭ
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

    // process p�� ť�� �ִ��� �˻�
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

    // process p�� queue���� ����
    while (index != q->rear) {
        int next = (index + 1) % q->capacity;
        q->process[index] = q->process[next];
        index = next;
    }

    // queue�� rear, count ����
    if (q->rear == 0) {
        q->rear = q->capacity - 1;
    }
    else {
        q->rear--;
    }

    q->count--;

    // queue�� ��� front, rear �ʱ�ȭ
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
        p->io_request_time = (rand() % (p->cpu_burst - 1)) + 1; // 1 ~ (cpu_burst - 1), cpu burst �� 1���� ����
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

// FCFS �˰���
void scheduling_FCFS() {
    Process* executing_process = NULL;

    int completed_process_count = 0;
    int last_event_time = 0; // ���������� ó���� event�� �߻��� ����
    int last_run_start = 0; // executing_process�� ���������� ����� ����

    // process�� ���� �ð� ������ event heap�� push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();

        int now = e.time;
        int to_io; // IO request���� ���� �ð�
        int run; // CPU �۾��� ������ �ð�

        // gantt chart ���
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
                // �̹� preemption �Ǿ� �� �̻� ���� ���� �ƴ� ���
                if (p != executing_process) {
                    break;
                }

                // ���� �ֱٿ� ������ cpu �۾� �ð�
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request �߻� ��
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // ����(now) + ���� io burst ���Ŀ� ����Ǵ� process�� push
                    push_event(now + p->remaining_io, 3, p);
                }
                // ������ ����� process
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

            // ����(now) �߻��ϴ� event�� �� ������ ��� ó��
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // cpu ���Ҵ�
        if (!executing_process && !is_empty(&ready_queue)) {
            executing_process = dequeue(&ready_queue);

            // ó�� ������ process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process�� IO �۾� ���� �ٽ� CPU �۾��� �����ϴ� ���
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // process�� IO �۾��� ���� �������� ���� ���
            else {
                run = to_io;
            }

            // process�� �����ؾ� �ϴ� �۾��� ���� ���
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process�� ������ �����ϴ� ���
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

// Non-Preemptive SJF �˰���
void scheduling_Non_Preemptive_SJF() {
    Process* executing_process = NULL;
    int completed_process_count = 0;
    int last_event_time = 0; // ���������� ó���� event�� �߻��� ����
    int last_run_start = 0; // executing_process�� ���������� ����� ����

    // process�� ���� �ð� ������ event heap�� push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();
        int now = e.time;
        int to_io; // IO request���� ���� �ð�
        int run; // CPU �۾��� ������ �ð�

        // gantt chart ���
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
                // �̹� preemption �Ǿ� �� �̻� ���� ���� �ƴ� ���
                if (p != executing_process) {
                    break;
                }

                // ���� �ֱٿ� ������ cpu �۾� �ð�
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request �߻� ��
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // ����(now) + ���� io burst ���Ŀ� ����Ǵ� process�� push
                    push_event(now + p->remaining_io, 3, p);
                }
                // ������ ����� process
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

            // ����(now) �߻��ϴ� event�� �� ������ ��� ó��
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // cpu ���Ҵ�
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

            // shortest process�� cpu �Ҵ�
            executing_process = ready_queue.process[shortest_index];

            int index = shortest_index;

            // ready queue���� ����
            while (index != ready_queue.rear) {
                int next = (index + 1) % ready_queue.capacity;
                ready_queue.process[index] = ready_queue.process[next];
                index = next;
            }

            // ready queue�� rear, count ����
            if (ready_queue.rear == 0) {
                ready_queue.rear = ready_queue.capacity - 1;
            }
            else {
                ready_queue.rear--;
            }

            ready_queue.count--;

            // ready_queue�� ��� front, rear �ʱ�ȭ
            if (ready_queue.count == 0) {
                ready_queue.front = 0;
                ready_queue.rear = 0;
            }

            // ó�� ������ process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process�� IO �۾� ���� �ٽ� CPU �۾��� �����ϴ� ���
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // process�� IO �۾��� ���� �������� ���� ���
            else {
                run = to_io;
            }

            // process�� �����ؾ� �ϴ� �۾��� ���� ���
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process�� ������ �����ϴ� ���
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

// Preemptive SJF �˰���
void scheduling_Preemptive_SJF() {
    Process* executing_process = NULL;
    int completed_process_count = 0;
    int last_event_time = 0; // ���������� ó���� event�� �߻��� ����
    int last_run_start = 0; // executing_process�� ���������� ����� ����

    // process�� ���� �ð� ������ event heap�� push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();
        int now = e.time;
        int to_io; // IO request���� ���� �ð�
        int run; // CPU �۾��� ������ �ð�

        // gantt chart ���
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
                // �̹� preemption �Ǿ� �� �̻� ���� ���� �ƴ� ���
                if (p != executing_process) {
                    break;
                }

                // ���� �ֱٿ� ������ cpu �۾� �ð�
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request �߻� ��
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // ����(now) + ���� io burst ���Ŀ� ����Ǵ� process�� push
                    push_event(now + p->remaining_io, 3, p);
                }
                // preemption �߻� ��
                else if (p->remaining_cpu > 0) {
                    enqueue(&ready_queue, p);
                }
                // ������ ����� process
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

            // ����(now) �߻��ϴ� event�� �� ������ ��� ó��
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // preemption �߻� �˻�
        if (executing_process && !is_empty(&ready_queue)) {
            executing_process->executed_time += now - last_run_start;

            int shortest_index = ready_queue.front;
            int shortest_cpu_burst = ready_queue.process[shortest_index]->remaining_cpu;

            // shortest process Ž��
            for (int k = 0; k < ready_queue.count; k++) {
                int idx = (ready_queue.front + k) % ready_queue.capacity;

                if (ready_queue.process[idx]->remaining_cpu < shortest_cpu_burst) {
                    shortest_cpu_burst = ready_queue.process[idx]->remaining_cpu;
                    shortest_index = idx;
                }
            }

            // ready queue�� shortest���� ��� ���� ���� process�� ready queue�� push
            if (shortest_cpu_burst < executing_process->remaining_cpu) {
                enqueue(&ready_queue, executing_process);

                executing_process = NULL;

                last_run_start = now;
            }
        }

        // cpu ���Ҵ�
        if (!executing_process && !is_empty(&ready_queue)) {
            int shortest_index = ready_queue.front;
            int shortest_cpu_burst = ready_queue.process[shortest_index]->remaining_cpu;

            // shortest process Ž��
            for (int k = 0; k < ready_queue.count; k++) {
                int idx = (ready_queue.front + k) % ready_queue.capacity;

                if (ready_queue.process[idx]->remaining_cpu < shortest_cpu_burst) {
                    shortest_cpu_burst = ready_queue.process[idx]->remaining_cpu;
                    shortest_index = idx;
                }
            }

            // shortest process�� cpu �Ҵ�
            executing_process = ready_queue.process[shortest_index];

            int index = shortest_index;

            // ready queue���� ����
            while (index != ready_queue.rear) {
                int next = (index + 1) % ready_queue.capacity;
                ready_queue.process[index] = ready_queue.process[next];
                index = next;
            }

            // ready queue�� rear, count ����
            if (ready_queue.rear == 0) {
                ready_queue.rear = ready_queue.capacity - 1;
            }
            else {
                ready_queue.rear--;
            }

            ready_queue.count--;

            // ready_queue�� ��� front, rear �ʱ�ȭ
            if (ready_queue.count == 0) {
                ready_queue.front = 0;
                ready_queue.rear = 0;
            }

            // ó�� ������ process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process�� IO �۾� ���� �ٽ� CPU �۾��� �����ϴ� ���
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // process�� IO �۾��� ���� �������� ���� ���
            else {
                run = to_io;
            }

            // process�� �����ؾ� �ϴ� �۾��� ���� ���
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process�� ������ �����ϴ� ���
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

// Non-Preemptive Priority �˰���
void scheduling_Non_Preemptive_Priority() {
    Process* executing_process = NULL;
    int completed_process_count = 0;
    int last_event_time = 0; // ���������� ó���� event�� �߻��� ����
    int last_run_start = 0; // executing_process�� ���������� ����� ����

    // process�� ���� �ð� ������ event heap�� push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();
        int now = e.time;
        int to_io; // IO request���� ���� �ð�
        int run; // CPU �۾��� ������ �ð�

        // gantt chart ���
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
                // �̹� preemption �Ǿ� �� �̻� ���� ���� �ƴ� ���
                if (p != executing_process) {
                    break;
                }

                // ���� �ֱٿ� ������ cpu �۾� �ð�
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request �߻� ��
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // ����(now) + ���� io burst ���Ŀ� ����Ǵ� process�� push
                    push_event(now + p->remaining_io, 3, p);
                }
                // ������ ����� process
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

            // ����(now) �߻��ϴ� event�� �� ������ ��� ó��
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // cpu ���Ҵ�
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

            // best priority�� cpu �Ҵ�
            executing_process = ready_queue.process[best_index];

            int idx = best_index;

            // ready queue���� ����
            while (idx != ready_queue.rear) {
                int next = (idx + 1) % ready_queue.capacity;
                ready_queue.process[idx] = ready_queue.process[next];
                idx = next;
            }

            // ready queue�� rear, count ����
            if (ready_queue.rear == 0) {
                ready_queue.rear = ready_queue.capacity - 1;
            }
            else {
                ready_queue.rear--;
            }

            ready_queue.count--;

            // ready_queue�� ��� front, rear �ʱ�ȭ
            if (ready_queue.count == 0) {
                ready_queue.front = 0;
                ready_queue.rear = 0;
            }

            // ó�� ������ process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process�� IO �۾� ���� �ٽ� CPU �۾��� �����ϴ� ���
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // process�� IO �۾��� ���� �������� ���� ���
            else {
                run = to_io;
            }

            // process�� �����ؾ� �ϴ� �۾��� ���� ���
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process�� ������ �����ϴ� ���
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

// Preemptive Priority �˰���
void scheduling_Preemptive_Priority() {
    Process* executing_process = NULL;
    int completed_process_count = 0;
    int last_event_time = 0; // ���������� ó���� event�� �߻��� ����
    int last_run_start = 0; // executing_process�� ���������� ����� ����

    // process�� ���� �ð� ������ event heap�� push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();
        int now = e.time;
        int to_io; // IO request���� ���� �ð�
        int run; // CPU �۾��� ������ �ð�

        // gantt chart ���
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
                // �̹� preemption �Ǿ� �� �̻� ���� ���� �ƴ� ���
                if (p != executing_process) {
                    break;
                }

                // ���� �ֱٿ� ������ cpu �۾� �ð�
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request �߻� ��
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // ����(now) + ���� io burst ���Ŀ� ����Ǵ� process�� push
                    push_event(now + p->remaining_io, 3, p);
                }
                // preemption �߻� ��
                else if (p->remaining_cpu > 0) {
                    enqueue(&ready_queue, p);
                }
                // ������ ����� process
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

            // ����(now) �߻��ϴ� event�� �� ������ ��� ó��
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // preemption �߻� �˻�
        if (executing_process && !is_empty(&ready_queue)) {
            executing_process->executed_time += now - last_run_start;

            int best_index = ready_queue.front;
            int best_priority = ready_queue.process[best_index]->priority;

            // best priority Ž��
            for (int k = 0; k < ready_queue.count; k++) {
                int idx = (ready_queue.front + k) % ready_queue.capacity;

                if (ready_queue.process[idx]->priority > best_priority) {
                    best_priority = ready_queue.process[idx]->priority;
                    best_index = idx;
                }
            }

            // ready queue�� shortest���� ��� ���� ���� process�� ready queue�� push
            if (best_priority > executing_process->priority) {
                enqueue(&ready_queue, executing_process);

                executing_process = NULL;

                last_run_start = now;
            }
        }

        // cpu ���Ҵ�
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

            // best priority�� cpu �Ҵ�
            executing_process = ready_queue.process[best_index];

            int idx = best_index;

            // ready queue���� ����
            while (idx != ready_queue.rear) {
                int next = (idx + 1) % ready_queue.capacity;
                ready_queue.process[idx] = ready_queue.process[next];
                idx = next;
            }

            // ready queue�� rear, count ����
            if (ready_queue.rear == 0) {
                ready_queue.rear = ready_queue.capacity - 1;
            }
            else {
                ready_queue.rear--;
            }

            ready_queue.count--;

            // ready_queue�� ��� front, rear �ʱ�ȭ
            if (ready_queue.count == 0) {
                ready_queue.front = 0;
                ready_queue.rear = 0;
            }

            // ó�� ������ process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process�� IO �۾� ���� �ٽ� CPU �۾��� �����ϴ� ���
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // process�� IO �۾��� ���� �������� ���� ���
            else {
                run = to_io;
            }

            // process�� �����ؾ� �ϴ� �۾��� ���� ���
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process�� ������ �����ϴ� ���
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

// Round Robin �˰���
void scheduling_Round_Robin() {
    Process* executing_process = NULL;
    int completed_process_count = 0;
    int last_event_time = 0; // ���������� ó���� event�� �߻��� ����
    int last_run_start = 0; // executing_process�� ���������� ����� ����

    // process�� ���� �ð� ������ event heap�� push
    for (int i = 0; i < PROCESS_COUNT; i++) {
        push_event(process_list[i].arrival_time, 1, &process_list[i]);
    }

    while (event_count > 0 && completed_process_count < PROCESS_COUNT) {
        Event e = pop_event();
        int now = e.time;
        int to_io; // IO request���� ���� �ð�
        int run; // CPU �۾��� ������ �ð�

        // gantt chart ���
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
                // �̹� preemption �Ǿ� �� �̻� ���� ���� �ƴ� ���
                if (p != executing_process) {
                    break;
                }

                // ���� �ֱٿ� ������ cpu �۾� �ð�
                p->executed_time += now - last_run_start;
                p->remaining_cpu -= now - last_run_start;

                // I/O request �߻� ��
                if (p->executed_time == p->io_request_time) {
                    if (now - 1 >= 0 && now - 1 < MAX_TIME) {
                        gantt_io[now - 1] = p->pid;
                    }

                    p->remaining_io = p->io_burst;
                    enqueue(&waiting_queue, p);
                    // ����(now) + ���� io burst ���Ŀ� ����Ǵ� process�� push
                    push_event(now + p->remaining_io, 3, p);
                }
                // preemption �߻� ��
                else if (p->remaining_cpu > 0) {
                    enqueue(&ready_queue, p);
                }
                // ������ ����� process
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

            // ����(now) �߻��ϴ� event�� �� ������ ��� ó��
            if (event_count > 0 && event_heap[1].time == now) {
                e = pop_event();
            }
            else {
                break;
            }
        } while (1);

        // cpu ���Ҵ�
        if (!executing_process && !is_empty(&ready_queue)) {
            executing_process = dequeue(&ready_queue);

            // ó�� ������ process
            if (executing_process->start_time < 0) {
                executing_process->start_time = now;
            }

            to_io = executing_process->io_request_time - executing_process->executed_time;

            // process�� IO �۾� ���� �ٽ� CPU �۾��� �����ϴ� ���
            if (to_io <= 0) {
                run = executing_process->remaining_cpu;
            }
            // IO request�� TIME QUANTUM ���� �߻��� ��
            else if (to_io < TIME_QUANTUM) {
                run = to_io;
            }
            // TIME QUANTUM��ŭ ����, ���� ���� cpu burst�� �� ������ ���� cpu burst��ŭ ����
            else {
                run = TIME_QUANTUM;

                if (executing_process->remaining_cpu < run) {
                    run = executing_process->remaining_cpu;
                }
            }

            // process�� �����ؾ� �ϴ� �۾��� ���� ���
            if (run > 0) {
                last_run_start = now;
                push_event(now + run, 2, executing_process);
            }
            // process�� ������ �����ϴ� ���
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
        // Idle ����
        if (gantt[t] == 0) {
            printf("|%*s", width - 1, "Idle");
        }
        // I/O request �Ǳ� ���� ����
        else if (gantt_io[t]) {
            char buffer[width + 1];
            snprintf(buffer, sizeof(buffer), "P%d(I/O)", gantt[t]);
            printf("| %*s", width - 2, buffer);
        }
        // ���� ���� process
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
