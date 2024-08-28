#include "memory.h"
#include "process.h"
#include "processqueue.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define DEBUG 0

// Returns a pointer to a memory block for continuous memory
c_Memory_t* create_c_memory_block(char* PID, unsigned short size, unsigned short offset) {
    c_Memory_t* block = (c_Memory_t*) malloc(sizeof(c_Memory_t));
    assert(block);
    block->PID = PID;
    block->size = size;
    block->offset = offset;
    block->next = NULL;
    return block;
}

// Attempts to inject a process's memory into the system, may fail if not enough space
// Returns TRUE/FALSE if successful or not
int c_inject_mem(c_Memory_t* head, Process_t* process) {
    c_Memory_t* curr = head;
    // find the first big enough gap, first fit
    while (curr != NULL && (curr->PID != NULL || curr->size < process->memory) ) {
        curr = curr->next;
    }
    // no space for process
    if (curr == NULL) {
        return FALSE;
    }
    // need to make a left over block if not perfect fit
    if (curr->size - process->memory != 0) {
        c_Memory_t* freeslot = create_c_memory_block(NULL, curr->size - process->memory, curr->offset + process->memory);
        assert(freeslot);
        freeslot->next = curr->next;
        curr->next = freeslot;
    }     
    curr->PID = process->PID;
    curr->size = process->memory;
    process->allocated = TRUE;

    return TRUE;
}

// Ejects the given process out of system memory, merging nearby blocks if needed
void c_eject_mem(c_Memory_t* memory, Process_t* process) {
    c_Memory_t* curr = memory;
    // find the memory for the given process, if it exists 
    while (curr != NULL) {
        if (curr->PID != NULL && strcmp(curr->PID, process->PID) == 0) {
           break;
        }
        curr = curr->next;
    }
    if (curr == NULL) {
        return;
    }
    if (DEBUG) printf("Freeing block which is currently %s at 0x%hu\n", curr->PID, curr->offset);
    
    // free the memory
    curr->PID = NULL;
    process->allocated = FALSE;

    if (DEBUG) {
        printf("Before merge\t");
        c_display_memory(memory);
    }
    c_merge_blocks(memory);
    
    if (DEBUG) {
        printf("After merge\t");
        c_display_memory(memory);
    }
}

// Merges two memory blocks into one, b1 must come directly before b2 in sequential memory
void c_merge(c_Memory_t* b1, c_Memory_t* b2) {
    
    b1->next = b2->next;
    b1->size += b2->size;
    free(b2);    
}

// Goes over whole memory linked list, merging blocks if two free blocks are next to eachother
void c_merge_blocks(c_Memory_t* head) {
    c_Memory_t* prev = head;
    c_Memory_t* curr = head->next;
    if (curr == NULL){
        // only one block in memory
        return;
    }

    while (curr != NULL) {
        if (prev->PID == NULL && curr->PID == NULL) {
            c_merge(prev, curr);
            curr = prev->next;
        } else {
            prev = curr;
            curr = prev->next;
        }
    }
}

// Calculates system memory usage for continuous memory
double c_calc_mem_usage(c_Memory_t* head) {
    double used = 0.0;
    c_Memory_t* curr = head;
    
    while (curr != NULL) {
        if (curr->PID != NULL) {
            used += curr->size;
        }
        curr = curr->next;
    }

    return used / (double)SYSMEM;
}

// For a given process, finds the offset in memory where the process lays, or -1 if its not in memory
int c_get_offset(c_Memory_t* head, char* PID) {
    c_Memory_t* curr = head; 
    while (curr != NULL) {
        if (curr->PID != NULL && strcmp(PID, curr->PID) == 0) {
            break;
        }
        curr = curr->next;
    }
    if (curr != NULL) {
        return curr->offset;
    }
    return -1;
}

// Debug function that displays memory and all the blocks
void c_display_memory(c_Memory_t* head) {
    c_Memory_t* curr = head;
    while (curr != NULL) {
        printf("%s at 0x%hu, size=%hu-> ", curr->PID, curr->offset, curr->size);
        curr = curr->next;
    }
    printf("\n");
}

// Frees all remaining blocks in the linked list
void c_block_free(c_Memory_t* head) {
    c_Memory_t* curr = head;
    c_Memory_t* next;
    while (curr != NULL) {
        next = curr->next;
        free(curr);
        curr = next;
    }
}

// Creates the table used for paged/virtual memory
f_Memory_t* create_f_memory_table(unsigned short total, int frame_size) {
    f_Memory_t* memory = (f_Memory_t*) malloc(sizeof(f_Memory_t));
    assert(memory);
    memory->total_mem = total;
    memory->num_frame = total/frame_size;
    memory->frames = (char**) malloc(sizeof(char*)*memory->num_frame);
    // all intially free
    for (int i=0; i<memory->num_frame; i++) {
        memory->frames[i] = NULL;
    }
    return memory;
}

// Frees paged memory tables
void f_table_free(f_Memory_t* memory) {
    free(memory->frames);
    free(memory);
}   

// Returns the amount of free pages in memory
int f_cnt_free_pages(f_Memory_t* table) {
    int free = 0;
    for (int i=0; i<table->num_frame; i++) {
        if (table->frames[i] == NULL) {
            free += 1;
        }
    }
    return free;
}

// Injects the given processes into memory, freeing pages if required
void f_inject_mem(int cycle, f_Memory_t* table, Process_t* process, queue_t* queue) {
    // array to store all ejected pages so it can be accurately printed later
    int ejected_mem[SYSMEM/PAGESIZE + 1];
    int i;
    // -1 meaning not used, -2 meaning end of array
    for (i=0; i<SYSMEM/PAGESIZE; i++) {
        ejected_mem[i] = -1;
    }
    ejected_mem[i] = -2;
    
    // free pages if not enough for this process
    int space;
    while ((space = f_cnt_free_pages(table)) < process->pages) {
        f_free_mem(ejected_mem, table, queue); 
    }

    if (ejected_mem[0] != -1) print_evicted_message(cycle, ejected_mem, SYSMEM/PAGESIZE);

    // inserts process into memory
    int k=0;
    for (int i=0; i<table->num_frame; i++) {
        if (table->frames[i] == NULL) {
            table->frames[i] = process->PID; 
            process->page_table[k++] = i; // page 'k' is in index 'i' frame  
        }
        if (k >= process->pages) {
            // all process pages allocated
            process->allocated = TRUE;
            break;
        }
    }
    
}

// frees memory based on least recent process ideal 
void f_free_mem(int* ejected_mem, f_Memory_t* table, queue_t* queue) {
    node_t* curr = queue->head;
    Process_t* least_recent_process_allocated = NULL;
    // find process that has been ran the least recently that has memory allocated, which will be the process nearest to the top
    while (curr != NULL) {
        if (curr->process->allocated == TRUE) {
            least_recent_process_allocated = curr->process;
            break;
        }
        curr = curr->next;
    }
    
    f_eject_mem(ejected_mem, table, least_recent_process_allocated);
}

// Ejects the given process from paged memory
void f_eject_mem(int* ejected_mem, f_Memory_t* table, Process_t* process) {
    assert(process != NULL);
    int j = 0; // ejected mem offset
    int l = 0; // process mem offset

    // two cases, either ejecting after finishing or ejecting to make space 
    // ejecting after process, no need to have an ejected_mem array
    if (ejected_mem == NULL) {
        for (int i=0; i+l<process->pages; i++) {
            while (process->page_table[i+l] == -1) {
            l++;
            if (i+l >= process->pages) return; 
        }
            table->frames[process->page_table[i+l]] = NULL; // clear memory
            process->page_table[i+l] = -1;
        }
        process->allocated = FALSE;
        return;
    }
    // ejecting to make space, using j to maintain correct position in ejected_mem
    for (int i=0; i+l<process->pages; i++) {       
        while (process->page_table[i+l] == -1) {
            l++;
            if (i+l > process->pages) return; 
        }
        table->frames[process->page_table[i+l]] = NULL; // clear memory
        while (ejected_mem[i+j] != -1 && ejected_mem[i+j] != -2) {
            j++;
        }
        if (ejected_mem[i+j] == -2) {
            break;
        }
        ejected_mem[i+j] = process->page_table[i+l];
        process->page_table[i+l] = -1;
    }
    process->allocated = FALSE;
}

// Calculate paged memory usage
double f_mem_usage(f_Memory_t* table) {
    double used = 0;
    for (int i=0; i<table->num_frame; i++) {
        if (table->frames[i] != NULL) {
            used += 1;
        }
    }
    return used/(double)table->num_frame;
}

// Prints out a formatted array, used to print out memory frames
void f_print_mem_frames(int* page_table, int pages) {
    // skip all -1 values, they are not used frames
    int i=0;
    while (page_table[i] == -1) {
            i++;
    }
    
    printf("[%d", page_table[i++]);
    
    for (; i<pages; i++) {
        if (page_table[i] == -1) {
            continue;
        }
        printf(",%d", page_table[i]);
    }
    printf("]\n");
}

// Counts all allocated memory for a process, determine if its valid to run
int v_cnt_allocated(Process_t* process) {
    int cnt = 0;
    for (int i=0; i<process->pages; i++) {
        if (process->page_table[i] != -1) {
            cnt += 1;
        }
    }
    return cnt;
}

// Injects a process into virtual memory, freeing memory if required
void v_inject_mem(Process_t* process, f_Memory_t* table, queue_t* queue, int cycle) {
    // will ever only eject the minimum required pages (from here atleast)
    int ejected_mem[REQ_PAGES + 1] = {-1, -1, -1, -1, -2};

    // if we dont have enough free pages
    int c;
    int free;
    while ((c = REQ_PAGES - v_cnt_allocated(process)) > (free = f_cnt_free_pages(table))) {
        if (DEBUG) printf("required = %d | %d ", c, free);
        v_free_mem(ejected_mem, table, queue, c-free);
    } 
    if (ejected_mem[0] != -1) print_evicted_message(cycle, ejected_mem, REQ_PAGES);
    
    if (DEBUG) printf("DEBUG ");
    if (DEBUG) print_evicted_message(cycle, ejected_mem, REQ_PAGES);
  
    int k=0;
    for (int i=0; i<table->num_frame; i++) {
        // go until we have fill all memory or put all process memory in 
        if (table->frames[i] == NULL) {
            table->frames[i] = process->PID; // should work?
            process->page_table[k++] = i; // page 'k' is in index 'i' frame  
        }
        if (k >= process->pages) {
            // all process pages allocated
            break;
        }
    }
    process->allocated = TRUE;
}

// Finds the least recent allocated process to eject required memory
void v_free_mem(int* ejected_mem, f_Memory_t* table, queue_t* queue, int required_space) {
    // the last used process is actually the highest in the queue (most recent is at the bottom)
    node_t* curr = queue->head;
    Process_t* least_recent_process_allocated = NULL;
    
    if (DEBUG) printf("\n");
    if (DEBUG) print_queue(queue);
    if (DEBUG) printf("\n");
    // find process that has been ran the least recently that has memory allocated
    while (curr != NULL) {
        if (curr->process->allocated == TRUE) {
            least_recent_process_allocated = curr->process;
            break;
        }
        curr = curr->next;
    }
    
    v_eject_mem(ejected_mem, least_recent_process_allocated, table, required_space);
    
} 

// Ejecting the required amount of space in memory from the given process
void v_eject_mem(int* ejected_mem, Process_t* process, f_Memory_t* table, int required_space) {
    int j = 0; // ejected mem offset
    int l = 0; // process memory offset
    
    if (DEBUG) printf("ejecting from %s, requiring %d\n",process->PID, required_space);
    
    for (int i=0; i<required_space; i++) {
        for (int k=0; k<REQ_PAGES+1; k++) {
            if(DEBUG) printf("%d:%d ", k, ejected_mem[k]);
        }
        if(DEBUG) printf("\n");
        
        while (process->page_table[i+l] == -1) l++;
        
        if (DEBUG) printf("process->page_table[i+l] = %d table->frames[process->page_table[i+l]] = %s before ejection\n", process->page_table[i+l], table->frames[process->page_table[i+l]]);           

        table->frames[process->page_table[i+l]] = NULL; // clear memory
        while (ejected_mem[i+j] != -1 && ejected_mem[i+j] != -2) {
            j++;
        }
        if (ejected_mem[i+j] == -2) {
            break;
        }
        ejected_mem[i+j] = process->page_table[i+l];
        process->page_table[i+l] = -1;
    }

    if (v_cnt_allocated(process) == 0) process->allocated = FALSE;
}