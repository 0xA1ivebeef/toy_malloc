
/*
 * vaguely following tsodings video: https://www.youtube.com/watch?v=sZ8GJ1TiMdk
 
    this program just uses one single heap of fixed size which can not be expanded
    its a global variable
    
    alignment is implemented as 8 byte aligned so standard for 64bit systems

    some edge cases are included:
        allocating a chunk of 0 bytes returns NULL, so is skipped essentially
        (malloc compared to this returns a unique ptr every call, just shifted 
        0x30 off another, so it does allocate chunks but the data size is 0, i think) 
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#define HEAP_CAPACITY 640000 // 64kb

typedef struct s_heap_chunk
{
    // s_heap_chunk*   prev;
    // s_heap_chunk*   next;
    void*           start;
    size_t          data_size;
    bool            is_freed;
} t_heap_chunk;

#define MAX_CHUNKS (int)(HEAP_CAPACITY / sizeof(t_heap_chunk))

char heap[HEAP_CAPACITY] = {0};
size_t heap_size = 0;

t_heap_chunk heap_alloced[MAX_CHUNKS] = {0}; // there are a max of HEAP_CAPACITY / sizeof(t_heap_chunk) 
size_t heap_alloced_count = 0;

void* heap_alloc(size_t requested_size)
{
    // 1 byte is the minimum size that can be allocated, resulting in an 8 byte chunk due to rounding it up for alignment
    if (requested_size < 1) 
        return NULL;

    size_t aligned_size = (requested_size + 7) & ~7; // 8-byte alignment
                                                     
    assert(heap_size + aligned_size <= HEAP_CAPACITY);
    assert(heap_alloced_count < MAX_CHUNKS);

    void* result = heap + heap_size; // current_heap_size
    heap_size += aligned_size; // expand by size

    // add metadata to the metadata struct array 
    t_heap_chunk chunk = 
    {
        .start = result, 
        .data_size = aligned_size,
        .is_freed = false
    };
    heap_alloced[heap_alloced_count++] = chunk;

    return result;
}   

void heap_dump_alloced_chunks()
{
    printf("Allocated Chunks: (%ld)\n", heap_alloced_count);
    for (size_t i = 0; i < heap_alloced_count; ++i)
    {
        printf("start: %p, size: %ldbytes, %b\n", 
                heap_alloced[i].start, 
                heap_alloced[i].data_size, 
                heap_alloced[i].is_freed);
    }
}

void heap_free(void* ptr)
{
    // how do we know how much space this pointer allocated?
    // find the struct with this ptr
    for (size_t i = 0; i < heap_alloced_count; ++i)
    {
        if (heap_alloced[i].start == ptr)
        {
            if (heap_alloced[i].is_freed)
            {
                fprintf(stderr, "heap_free: chunks is already freed\n");
                return;
            }

            // free this chunk
            heap_alloced[i].is_freed = 1;

            // carefull to not go negative ! maybe there is an edge case
            assert(heap_alloced_count > 0);
            heap_alloced_count -= 1; 

            // memset(heap_alloced[i].start, 1, heap_alloced[i].data_size); maybe reset is not necassary, just overwrite
            
            // remove the freed chunk from heap_alloced
            memset(&heap_alloced[i], 0, sizeof(t_heap_chunk)); 

            // remove the struct and move all remaining ones after that left by sizeof(t_heap_chunk)
            // NOTE: pointer arithmetic already scales addresses of arrays with element size so its i+1
            memmove(&heap_alloced[i], 
                    &heap_alloced[i + 1], 
                    (heap_alloced_count - (i+1)) * sizeof(t_heap_chunk));

            return;
        }
    }
    fprintf(stderr, "chunk is not in heap_alloced\n");
}

int main(void)
{
    for (size_t i = 0; i < 100; ++i)
    {
        void* c = heap_alloc(i);
        if (i % 2 == 0)
            heap_free(c);
    }

    heap_dump_alloced_chunks();

    return 0;
}

