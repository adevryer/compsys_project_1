#include <iso646.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include "process.h"
#include "memory.h"


Process_list_t* get_processes_from_input(char* filepath);

int main(int argc, char** argv) {
    char* filepath;
    int quantum, alloc_mode, c = 0;
    // read in all flags/options given from agrv
    while ((c = getopt (argc, argv, "f:q:m:")) != -1) {
        switch (c) {
            case 'f':
                filepath = strdup(optarg);
                break;

            case 'q':
                quantum = atoi(optarg);
                break;

            case 'm':
                if (strcasecmp(optarg, "infinite") == 0) {
                    alloc_mode = INFINITE;
                } else if (strcasecmp(optarg, "first-fit") == 0) {
                    alloc_mode = FIRSTFIT;
                } else if (strcasecmp(optarg, "paged") == 0) {
                    alloc_mode = PAGED;
                } else if (strcasecmp(optarg, "virtual") == 0) {
                    alloc_mode = VIRTUAL;
                }
                break;
        }
    }


    Process_list_t* processes = get_processes_from_input(filepath);
    // run the appropiate simulation mode
    switch (alloc_mode) {
        case INFINITE:
            simulate_infinite(processes, quantum, alloc_mode);
            break;
        case FIRSTFIT:
            simulate_firstfit(processes, quantum, alloc_mode);
            break;
        case PAGED:
            simulate_paged(processes, quantum, alloc_mode);
            break;
        case VIRTUAL:
            simulate_virtual(processes, quantum, alloc_mode);
            break;
    }
    
    /*  
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    IMPORTANT
    HAVE YOU FREED ALL MEMORY ALLOC'ED?
    ENSURE NO MEMORY LEAKS BEFORE SUBMISSION
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    */
    
    for (int i = 0; i < processes->total_processes; i++) {
        free(processes->array[i].page_table);
    }
    free(processes->array);
    free(processes);
    free(filepath);
    return 0;
}
// Load all processes for the simulation into a list with the correct details
Process_list_t* get_processes_from_input(char* filepath) {
    int INITIAL_CAPT = 2; 
    int GROWTH_FCT = 2;
    int capt = INITIAL_CAPT;
    
    Process_list_t* processes = (Process_list_t*) malloc(sizeof(Process_list_t)); // struct holding all our processes
    assert(processes);
    processes->array = (Process_t*) malloc(sizeof(Process_t)*capt);
    processes->total_processes = 0;
    assert(processes->array);
    
    FILE* fptr = fopen(filepath, "r"); // open our case file
    assert(fptr);

    int i=0;
    __uint32_t time, duration; 
    __uint16_t memory;     
    char PID[9];
    while(fscanf(fptr, "%u %8s %u %hu", &time, PID, &duration, &memory) == 4) {
        // loading attributes for each process
        processes->array[i].pages = ceil(memory/(double)PAGESIZE); // avoid int division 
        processes->array[i].page_table = (int*) malloc(sizeof(int)*processes->array[i].pages);
        for (int j=0; j<processes->array[i].pages; j++) {
            processes->array[i].page_table[j] = -1;
        }
        processes->array[i].start_time = time;
        processes->array[i].duration = duration;
        processes->array[i].service_time = duration;
        processes->array[i].memory = memory;
        processes->array[i].allocated = FALSE;
        processes->array[i].state = NOT_READY;
        strcpy(processes->array[i].PID, PID);
        
        i++;
        if (i == capt) { // might expand more than needed
            capt *= GROWTH_FCT;
            processes->array = (Process_t*) realloc(processes->array, sizeof(Process_t)*capt);
            assert(processes->array);
        }
    }
    processes->total_processes = i; 
    fclose(fptr);
    return processes;
}