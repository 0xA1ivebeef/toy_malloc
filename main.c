
/*
 * vaguely following tsodings video: https://www.youtube.com/watch?v=sZ8GJ1TiMdk
 
    this program just uses one single heap of fixed size which can not be expanded
    its kept as a global variable
    
    alignment is implemented as 8 byte aligned so standard for 64bit systems

    some edge cases are included but its completely unsafe of course just for learning purposes
        allocating a chunk of 0 bytes returns NULL, so is skipped essentially
        (malloc compared to this returns a unique ptr every call, just shifted 
        0x30 off another, so it does allocate chunks but the data size is 0, i think)

    coalescence is implemented in free for prev and next

    interesting obervation:
    if we free every second chunk we allocate, then coalescence will merge the chunk to the heap
    and split it off again this is basically work we dont have to do because this removes 
    the sense of reusing chunks we had before coalescence (only if the chunk could be reused by
    the next allocation)

    so if we were allocating a chunk and then freeing this chunk and allocating another
    chunk of size <= first chunk the freed space could be reused without merging and splitting
    but merging is implicit in heap_free and its not known what going to allocate next
*/

/*
    free inserts chunk into free list
    free list is implicit in t_chunk (next free chunk) (singly linked list)
    malloc scans only free blocks
*/

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define HEAP_SIZE 64000 // 64kb
#define TOLERATE_DIFF 50 // upbound difference, size in bytes to determine if a 
                         // free chunk should be used normally or split

// size_t size: size of the user data 
// s_chunk* next_free: NULL if chunk is allocated linked list of free chunks if chunk is free
typedef struct s_chunk
{
    size_t          	size;
    struct s_chunk*     next_free; 
} t_chunk;

#define MAX_CHUNKS (int)(HEAP_SIZE / sizeof(t_chunk))
#define ALIGN 8

_Alignas(8) static uint8_t heap[HEAP_SIZE];
t_chunk* free_list_head = NULL;

int round_count = 0;

void heap_init(void)
{
    t_chunk* initial = (t_chunk*)heap;

    initial->size = HEAP_SIZE - sizeof(t_chunk),
    initial->next_free = NULL;

    free_list_head = initial;
}

void print_free_list() 
{
    t_chunk* c = free_list_head;
    printf("Free list:\n");
    while (c) 
	{
        printf("  chunk %p size %ld\n", c, c->size);
        c = c->next_free;
    }
}

void* heap_alloc(size_t size)
{
    if (size == 0)
        return NULL;

    t_chunk* prev = NULL;
    t_chunk* curr = free_list_head;

    size_t aligned_size = (size + ALIGN-1) & ~(ALIGN-1);
    if (aligned_size != size)
        round_count++;

    while (curr)
    {
		if (curr->size < aligned_size) // chunk too small, ask next chunk
        {
			printf("heap_alloc: asking for next chunk current size: %ld, aligned size: %ld\n", curr->size, aligned_size);
            prev = curr;
            curr = curr->next_free;
			continue;
		} 

        size_t alloc_size = aligned_size + sizeof(t_chunk) + ALIGN;
        printf("heap_alloc: free chunk has size: %ld, alloc_size: %ld\n", curr->size, alloc_size);

        if (curr->size < alloc_size + TOLERATE_DIFF)
        {
			// allocate normally (not split)
            printf("heap_alloc: ALLOCATING NORMALLY\n");

			// update free list
            if (!prev)
 				free_list_head = curr->next_free; // becomes NULL if no chunks left
            else
				prev->next_free = curr->next_free; // insert and remove from free list
				
			curr->next_free = NULL; // remove from free list 

            return (void*)(curr + 1); // user data address
		}
        
        printf("heap_alloc: SPLITTING\n");

		size_t remaining = curr->size - aligned_size - sizeof(t_chunk); // other half of the chunk that is split

		t_chunk* split = (t_chunk*)((uint8_t*)(curr + 1) + aligned_size);
    	split->size = remaining;
    	split->next_free = curr->next_free;

		printf("heap_alloc: split new free chunk: size %ld\n", split->size);
		if (!prev)
		{
    		free_list_head = split;
			printf("heap_alloc: updated free list head to: %p\n", free_list_head);
		}
		else
		{
			prev->next_free = split; // insert
		}

		curr->size = aligned_size;
		curr->next_free = NULL;

        return (void*)(curr + 1); // user data address
	}

	fprintf(stderr, "ERROR heap_alloc: no free chunks left\n");
	return NULL;
}

void heap_free(void* ptr)
{
    // TODO what if free is given an unallocated chunk? / a random address?
    // thats undefined behaviour rn
	if (!ptr)
	{
		fprintf(stderr, "ERROR: heap_free called with NULL\n");
		return;
	}

	t_chunk* to_free = (t_chunk*)(ptr) - 1;
    if (!free_list_head)
    {
        // there are no free_chunks
        free_list_head = to_free;
        printf("heap_free: inserting only free chunk\n");
        return;
    }
    
    if (free_list_head > to_free)
    {
        // merge
        t_chunk* right_neighbor = (t_chunk*)((uint8_t*)to_free + sizeof(t_chunk) + to_free->size);
        if (right_neighbor == free_list_head)
        {
            // merge
            printf("heap_free: merging with free_list_head, as first index\n");
            to_free->size += sizeof(t_chunk) + free_list_head->size;
            to_free->next_free = free_list_head->next_free;
            free_list_head = to_free;
            return; 
        }
        to_free->next_free = free_list_head;
        free_list_head = to_free;
        printf("heap_free: inserting as new head\n");

        return; 
    }

    t_chunk* prev = NULL;
    t_chunk* curr = free_list_head;
    while (curr)
    {
        // traverse free list
        if (curr > to_free) // curr is the free chunk after the one we need to insert
        {
            // TODO: see if neighbors are next chunks left and right of to_free, then merge otherwise dont
            printf("heap_free: inserting free chunk: left_free: %p, this: %p, right_free: %p\n", prev, to_free, curr); 
 
            // basically check if left and right chunks of to_free are prev and curr

            // if normal next of prev == to_free
            bool left_merged = false;
            bool right_merged = false;
            if (prev)
            {   
                t_chunk* right_neighbor_of_prev = (t_chunk*)((uint8_t*)prev + sizeof(t_chunk) + prev->size);
                if (right_neighbor_of_prev == to_free)
                {
                    printf("heap_free: merging with left neighbor\n");
                    prev->size += sizeof(t_chunk) + to_free->size;
                    left_merged = true;
                    return; // no ptr updating needed, prev still points to curr and curr is unchanged
                }   
            }

            // ! this can be called when already having merged with left and also not having merged
            t_chunk* right_neighbor = (t_chunk*)((uint8_t*)to_free + sizeof(t_chunk) + to_free->size);
            if (curr == right_neighbor)
            {
                printf("heap_free: merging with right neighbor\n");
                if (left_merged)
                {
                    // merge prev with curr             
                    prev->size += sizeof(t_chunk) + curr->size;
                    prev->next_free = curr->next_free; // necessary in right merge ofc
                    printf("heap_free: merged all three chunks\n");
                    return;
                }
                else
                {
                    // only merge to_free and curr
                    // remove curr from the free_list
                    to_free->size += sizeof(t_chunk) + curr->size;
                    to_free->next_free = curr->next_free; // necessary in right merge ofc
                    printf("heap_free: merged only with right neighbor\n");
                    return; 
                }
                right_merged = true;
            }
            prev->next_free = to_free;
            to_free->next_free = curr;
            printf("heap_free: no merging done at all\n");
            return;
        }
        prev = curr;
        curr = curr->next_free;
    }

    // can at most merge with left chunk
    t_chunk* right_neighbor = (t_chunk*)((uint8_t*)prev + sizeof(t_chunk) + prev->size); 
    if (right_neighbor == to_free)
    {
        // merge
        printf("heap_free: merging last two chunks in free list\n");
        prev->size += sizeof(t_chunk) + to_free->size;
        return; // no ptr adjustments
    }

    printf("heap_free: no merging, inserting at end of free_list: %p\n", to_free); 
    prev->next_free = to_free;
    to_free->next_free = NULL;
}

int main(void)
{
    heap_init();
	
    size_t alloced_bytes = 0;

    // fragmenting the heap
    
    for (int i = 0; i < 100; i++)
    // for (int i = 0; i < 256; i+=8)
    {
        void* curr = heap_alloc(i);
        alloced_bytes += i;
        if (i % 2 == 0) // free every second chunk
        {
            heap_free(curr);
            alloced_bytes -= i;
        }
    }

	print_free_list();

    // free list has some small chunk when coalescence is not used and the one 
    // big chunk which is split during allocations

    printf("initial size: %d remaining free space should be around: %ld, we rounded %d times\n", HEAP_SIZE, HEAP_SIZE-alloced_bytes, round_count);

    return 0;
}

