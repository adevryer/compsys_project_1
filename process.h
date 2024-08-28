#ifndef PROCESS_H
#define PROCESS_H

#define FALSE 0
#define TRUE 1

#define NOT_READY -1
#define READY 1
#define RUNNING 2
#define FINISHED 3


typedef struct {
    unsigned int start_time;    // when the process was ready to execute max time is 2^32 
    unsigned int finish_time;   // time when process exited the process queue and finished
    unsigned int service_time;  // how much CPU time is needed
    unsigned int duration;      // how long the process still needs to finish, max time is 2^32 
    char PID[9];                // a distinct uppercase alphanumeric string, max length 8
    unsigned short memory;      // in KBs, max is 2048 (2^11)
    int* page_table;            // array indexing which pages are in which frames
    int pages;                  // total page count for process
    short allocated;            // boolean value if memory is allocated or not
    int state;                  // what state the process is currently in 
} Process_t;

typedef struct {
    Process_t* array;
    int total_processes;
} Process_list_t;


int is_complete(Process_list_t* processes);
void simulate_infinite(Process_list_t* processes, unsigned int quantum, int alloc_mode);
void simulate_firstfit(Process_list_t* processes, unsigned int quantum, int alloc_mode);
void simulate_paged(Process_list_t* processes, unsigned int quantum, int alloc_mode);
void simulate_virtual(Process_list_t* processes, unsigned int quantum, int alloc_mode);
void inf_print_state_message(int cycle, Process_t* process, int queue_length);
void c_print_state_message(int cycle, Process_t* process, void* head, int queue_length);
void find_and_print_statisics(Process_list_t* processes, int makespan);
void add_processes(Process_list_t* processes, void* queue, unsigned int cycle);

void print_finished_message(int cycle, Process_t* process, int queue_length);
void print_running_message(int cycle, Process_t* process, void* memory);
void print_evicted_message(int cycle, int* page_table, int pages);

#endif