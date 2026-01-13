
/*
 * vaguely following tsodings video: https://www.youtube.com/watch?v=sZ8GJ1TiMdk
 
    this program just uses one single heap of fixed size which can not be expanded
    its kept as a global variable
    
    alignment is implemented as 8 byte aligned so standard for 64bit systems

    some edge cases are included:
        allocating a chunk of 0 bytes returns NULL, so is skipped essentially
        (malloc compared to this returns a unique ptr every call, just shifted 
        0x30 off another, so it does allocate chunks but the data size is 0, i think)
*/

/*
    free inserts chunk into free list
    free list is implicit in t_chunk (next free chunk) (singly linked list)
    malloc scans only free blocks
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#define HEAP_SIZE 640000 // 64kb
#define D_SPLIT 100 // if 

typedef struct s_chunk
{
    size_t          size;
    bool            is_freed;
    s_chunk*        next_chunk;
} t_chunk;

#define MAX_CHUNKS (int)(HEAP_SIZE / sizeof(t_chunk))
#define ALIGN 8

_Alignas(8) static uint8_t heap[HEAP_SIZE];
t_chunk* free_list_head = NULL;

void heap_init(void)
{
    chunk_t* initial = (chunk_t*)heap;

    initial->size = HEAP_SIZE - sizeof(t_chunk),
    initial->is_freed = true,
    initial->next_free_chunk = NULL;

    free_list_head = initial;
}

void* heap_alloc(size_t size)
{
    if (size < 1)
        return NULL;

    t_chunk* prev = NULL;
    t_chunk* curr = free_list_head;

    size_t aligned_size = (size + ALIGN-1) & ~(ALIGN-1);

    while (curr)
    {
        if (curr->size >= aligned_size) // header in not included in curr-size its already allocated 
                                        // so only check if splitting
        {
            // chunk is large enough, use this chunk
            size_t alloc_size = aligned_size + sizeof(t_chunk) + ALIGN;
            printf("heap_alloc: free chunk has size: %d, alloc_size: %d\n", 
                    curr->size, alloc_size);

            if (curr->size >= alloc_size)
            {
                // split
                printf("SPLITTING\n");

                // remove from free list
                // need prev ptr
                // next ptr

                // add new chunk
                // set head if there is no head

                return (void*)(curr + 1); // user data address
            }        
            else
            {
                // allocate normally (not split)
                printf("ALLOCATING NORMALLY\n");

                // remove from free list
                if (!prev)
                {
                    // set new head 
                       
                }
                else
                {
                    // insert remove from free list

                }

                return (void*)(curr + 1); // user data address
            }
        }
        else
        {
            // ask next chunk
            prev = curr;
            curr = curr->next_chunk;
            if (curr == NULL)
            {
                fprintf(stderr, "error, heap_alloc: no space left for chunk of: 
                        %d, aligned: %d\n", size, aligned_size);
                return NULL;
            }
        }
    }

    return NULL;
}

void free(void* ptr)
{
    
}

int main(void)
{
    heap_init();

    void* root = heap_alloc(69);

    return 0;
}

