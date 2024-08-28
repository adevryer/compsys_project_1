#include "process.h"
#include "processqueue.h"
#include "memory.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define DEBUG 0

int _g_alloc_mode;
/*
The process manager runs in cycles. A cycle occurs after one quantum has elapsed. The process
manager has its own notion of time, referred to from here on as the simulation time. The simulation
time (TS ) starts at 0 and increases by the length of the quantum (Q) every cycle. For this project,
Q will be an integer value between 1 and 3 (1 ≤ Q ≤ 3).
*/
void simulate_infinite(Process_list_t* processes, unsigned int quantum, int alloc_mode) {
    unsigned int cycle = 0;
    _g_alloc_mode = alloc_mode;
    // create process queue
    queue_t* process_q = create_queue();

    while (is_complete(processes) == FALSE) {
        // add newly submitted processes to the queue
        add_processes(processes, process_q, cycle);
        // if no process in queue, finish cycle
        if (process_q->head == NULL) {
            cycle += quantum;
            continue;
        }
        
        // if currently running process is finished, eject it from CPU and queue
        if (process_q->head->process->duration == 0) {
            Process_t* process = dequeue(process_q);
            process->state = FINISHED; // put in queue functions?
            process->finish_time = cycle;
            print_finished_message(cycle, process, process_q->length);
            
        } 
        // if not finished, take top of the queue to the back
        else if (cycle != 0) {
            requeue(process_q);
        }

        // if no process, finish cycle
        if (process_q->head == NULL) {
            cycle += quantum;
            continue;
        }

        // a new process has started
        if (process_q->head->process->state != RUNNING) {
            process_q->head->process->state = RUNNING; // ditto
            print_running_message(cycle, process_q->head->process, NULL);
        }

        //underflow protection
        if (quantum > process_q->head->process->duration) {
            process_q->head->process->duration = 0;
        } else {
            process_q->head->process->duration -= quantum;
        }
        
        cycle += quantum;
    }
    find_and_print_statisics(processes, cycle-quantum);
    free(process_q);
}

void simulate_firstfit(Process_list_t* processes, unsigned int quantum, int alloc_mode) {
    unsigned int cycle = 0;
    _g_alloc_mode = alloc_mode;
    // create process queue
    queue_t* process_q = create_queue();
    c_Memory_t* memory_head = create_c_memory_block(NULL, SYSMEM, 0);

    while (is_complete(processes) == FALSE) {
        // add newly submitted processes to the queue
        add_processes(processes, process_q, cycle);
        // if no process in queue, finish cycle
        if (process_q->head == NULL) {
            cycle += quantum;
            continue;
        }

        if (DEBUG) c_display_memory(memory_head);

        // if currently running process is finished, eject it from CPU and queue
        if (process_q->head->process->state == RUNNING && process_q->head->process->duration == 0) {
            Process_t* process = dequeue(process_q);
            c_eject_mem(memory_head, process);
            
            process->finish_time = cycle;
            print_finished_message(cycle, process, process_q->length);    
        } 

        // if not finished, take top of the queue to the back
        else if (cycle != 0) {
            requeue(process_q);
        }

        // if no process, finish cycle
        if (process_q->head == NULL) {
            cycle += quantum;
            continue;
        }
        
        // a new process has started
        if (process_q->head->process->state != RUNNING) {
            while (process_q->head->process->allocated == FALSE && !c_inject_mem(memory_head, process_q->head->process)) {
                requeue(process_q);
            }
            process_q->head->process->state = RUNNING; // ditto
            print_running_message(cycle, process_q->head->process, memory_head);
        }

        //underflow protection
        if (quantum > process_q->head->process->duration) {
            process_q->head->process->duration = 0;
        } else {
            process_q->head->process->duration -= quantum;
        }
        
        cycle += quantum;

        if (cycle > 500) break;
    }
    c_block_free(memory_head);
    free(process_q);
    find_and_print_statisics(processes, cycle-quantum);
}

void simulate_paged(Process_list_t* processes, unsigned int quantum, int alloc_mode) {
    unsigned int cycle = 0;
    _g_alloc_mode = alloc_mode;
    // create process queue
    queue_t* process_q = create_queue();
    f_Memory_t* memory = create_f_memory_table(SYSMEM, PAGESIZE);

    while (is_complete(processes) == FALSE) {
        if (DEBUG && cycle > 500) break;
        // add newly submitted processes to the queue
        add_processes(processes, process_q, cycle);
        // if no process in queue, finish cycle
        if (process_q->head == NULL) {
            cycle += quantum;
            continue;
        }


        // if currently running process is finished, eject it from CPU and queue
        if (process_q->head->process->state == RUNNING && process_q->head->process->duration == 0) {
            Process_t* process = dequeue(process_q);
            print_evicted_message(cycle, process->page_table, process->pages);
            f_eject_mem(NULL, memory, process);
            process->finish_time = cycle;
            print_finished_message(cycle, process, process_q->length);    
        } 

        // if not finished, take top of the queue to the back
        else if (cycle != 0) {
            requeue(process_q);
        }

        // if no process, finish cycle
        if (process_q->head == NULL) {
            cycle += quantum;
            continue;
        }
        
        // a new process has started
        if (process_q->head->process->state != RUNNING) {
            if (process_q->head->process->allocated == FALSE) f_inject_mem(cycle, memory, process_q->head->process, process_q); 
            process_q->head->process->state = RUNNING; // ditto
            print_running_message(cycle, process_q->head->process, memory);
        }

        //underflow protection
        if (quantum > process_q->head->process->duration) {
            process_q->head->process->duration = 0;
        } else {
            process_q->head->process->duration -= quantum;
        }
        
        cycle += quantum;
    }
    f_table_free(memory);
    free(process_q);
    find_and_print_statisics(processes, cycle-quantum);
}

void simulate_virtual(Process_list_t* processes, unsigned int quantum, int alloc_mode) {
        unsigned int cycle = 0;
    _g_alloc_mode = alloc_mode;
    // create process queue
    queue_t* process_q = create_queue();
    f_Memory_t* memory = create_f_memory_table(SYSMEM, PAGESIZE);

    while (is_complete(processes) == FALSE) {
        if (DEBUG && cycle > 500) break;
        // add newly submitted processes to the queue
        add_processes(processes, process_q, cycle);
        // if no process in queue, finish cycle
        if (process_q->head == NULL) {
            cycle += quantum;
            continue;
        }

        //print_queue(process_q);

        // if currently running process is finished, eject it from CPU and queue
        if (process_q->head->process->state == RUNNING && process_q->head->process->duration == 0) {
            Process_t* process = dequeue(process_q);
            print_evicted_message(cycle, process->page_table, process->pages);
            f_eject_mem(NULL, memory, process);
            process->finish_time = cycle;
            print_finished_message(cycle, process, process_q->length);    
        } 

        // if not finished, take top of the queue to the back
        else if (cycle != 0) {
            requeue(process_q);
        }

        // if no process, finish cycle
        if (process_q->head == NULL) {
            cycle += quantum;
            continue;
        }
        
        // a new process has started
        if (process_q->head->process->state != RUNNING) {
            if (v_cnt_allocated(process_q->head->process) < REQ_PAGES) {
                v_inject_mem(process_q->head->process, memory, process_q, cycle);
            }
            
            process_q->head->process->state = RUNNING; 
            print_running_message(cycle, process_q->head->process, memory);
        }

        //underflow protection
        if (quantum > process_q->head->process->duration) {
            process_q->head->process->duration = 0;
        } else {
            process_q->head->process->duration -= quantum;
        }
        
        cycle += quantum;
    }
    f_table_free(memory);
    free(process_q);
    find_and_print_statisics(processes, cycle-quantum);
}

// Add processes that have elapsed their starting time into the process queue
void add_processes(Process_list_t* processes, void* queue, unsigned int cycle) {
     queue_t* process_q = (queue_t*) queue;
     for (int i=0; i<processes->total_processes; i++) {
        // add processes if their start time is now or has been
        if (processes->array[i].start_time <= cycle && processes->array[i].state == NOT_READY) {
            enqueue(process_q, &processes->array[i]);
        }
    }
}

// Checks if any process is still to run
int is_complete(Process_list_t* processes) {
    // if all processes are finished, the simulation is done, if any are not done, continue on
    for (int i=0; i<processes->total_processes; i++) {
        if (processes->array[i].state != FINISHED) {
            return FALSE;
        }
    }
    return TRUE;
}

void print_finished_message(int cycle, Process_t* process, int queue_length) {
    printf("%u,FINISHED,process-name=%s,proc-remaining=%u\n", cycle, process->PID, queue_length);
}

void print_running_message(int cycle, Process_t* process, void* memory) {
    c_Memory_t* mem_head;
    f_Memory_t* table;
    switch (_g_alloc_mode) {
        case INFINITE:
            printf("%u,RUNNING,process-name=%s,remaining-time=%u\n", cycle, process->PID, process->duration);
            break;
        case FIRSTFIT:
            mem_head = (c_Memory_t*) memory;
            printf("%u,RUNNING,process-name=%s,remaining-time=%u,mem-usage=%.f%%,allocated-at=%d\n", 
                    cycle, process->PID, process->duration, (c_calc_mem_usage(mem_head)*100), c_get_offset(mem_head, process->PID));
            break;
        case PAGED:
            table = (f_Memory_t*) memory;
            printf("%u,RUNNING,process-name=%s,remaining-time=%u,mem-usage=%.f%%,mem-frames=", 
                    cycle, process->PID, process->duration, ceil(f_mem_usage(table)*100));
            f_print_mem_frames(process->page_table, process->pages);
            break;
        case VIRTUAL:
            table = (f_Memory_t*) memory;
            printf("%u,RUNNING,process-name=%s,remaining-time=%u,mem-usage=%.f%%,mem-frames=", 
                    cycle, process->PID, process->duration, ceil(f_mem_usage(table)*100));
            f_print_mem_frames(process->page_table, process->pages);
            break;
        
    }
}

void print_evicted_message(int cycle, int* page_table, int pages) {
    printf("%u,EVICTED,evicted-frames=", cycle);
    f_print_mem_frames(page_table, pages);
}

// Calculate and display turnaround, overhead and makespan statisics for the ran processes
void find_and_print_statisics(Process_list_t* processes, int makespan) {
    int* turnaround = (int*) malloc(sizeof(int)*processes->total_processes);
    assert(turnaround);
    double avg_turnaround = 0, avg_overhead = 0;
    double max_overhead = 0;
    
    for (int i=0; i<processes->total_processes; i++) {
        // "turnaround time is the time elapsed between the arrival and the completion of a process"
        turnaround[i] = processes->array[i].finish_time - processes->array[i].start_time;
        avg_turnaround += turnaround[i];

        // "The time overhead of a process is defined as its turnaround time divided by its service time"
        double overhead = (double)turnaround[i] / processes->array[i].service_time;
        avg_overhead += overhead;

        if (DEBUG) printf("%s had turnaround %d and overhead %f\n", processes->array[i].PID, turnaround[i], overhead);
        if (overhead > max_overhead) {
            max_overhead = overhead;
        }
    }
    // always round up to nearest integer
    avg_turnaround = ceil(avg_turnaround/processes->total_processes);
    avg_overhead = round(avg_overhead/processes->total_processes*100)/100;

    free(turnaround);
    printf("Turnaround time %.f\nTime overhead %.2f %.2f\nMakespan %d\n", avg_turnaround, max_overhead, avg_overhead, makespan);
}