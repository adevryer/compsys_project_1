#ifndef MEMORY_H
#define MEMORY_H

#include "process.h"
#include "processqueue.h"

#define INFINITE 2
#define FIRSTFIT 3
#define PAGED 4
#define VIRTUAL 5

#define SYSMEM (unsigned short)2048
#define PAGESIZE 4
#define REQ_PAGES 4

// Struct for continuous (firstfit) memory
typedef struct c_Memory {
    struct c_Memory* next; // linked list structure
    char* PID;             // ID of process if block is allocated
    unsigned short size;   // in KBs
    unsigned short offset; // position in memory
} c_Memory_t;

// Struct for paged and virtual memory
typedef struct f_Memory {
    char** frames;      // array showing what process in is a given frame (or free)
    int num_frame;      // total num of frames
    int total_mem;      // how much memory in total, in KBs
} f_Memory_t;

c_Memory_t* create_c_memory_block(char* PID, unsigned short size, unsigned short offset);
int c_inject_mem(c_Memory_t* head, Process_t* process);
void c_eject_mem(c_Memory_t* memory, Process_t* process);
void c_merge(c_Memory_t* b1, c_Memory_t* b2);
void c_merge_blocks(c_Memory_t* head);
int c_get_offset(c_Memory_t* head, char* PID);
double c_calc_mem_usage(c_Memory_t* head);
void c_display_memory(c_Memory_t* head);
void c_block_free(c_Memory_t* head);

f_Memory_t* create_f_memory_table(unsigned short total, int frame_size);
void f_inject_mem(int cycle, f_Memory_t* table, Process_t* process, queue_t* queue);
void f_free_mem(int* ejected_mem, f_Memory_t* table, queue_t* queue);
void f_eject_mem(int* ejected_mem, f_Memory_t* table, Process_t* process);
void f_print_mem_frames(int* page_table, int pages);
double f_mem_usage(f_Memory_t* table);
void f_table_free(f_Memory_t* memory);

int v_cnt_allocated(Process_t* process);
void v_inject_mem(Process_t* process, f_Memory_t* table, queue_t* queue, int cycle);
void v_free_mem(int* ejected_mem, f_Memory_t* table, queue_t* queue, int required_space);
void v_eject_mem(int* ejected_mem, Process_t* process, f_Memory_t* table, int required_space);


#endif