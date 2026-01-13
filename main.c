
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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define HEAP_SIZE 64000 // 64kb
#define TOLERATE_DIFF 100 // upbound size in bytes to determine if a free chunk should be used
// (chunks that are <100 bytes larger than the requested size + metadata will be used normally and not split)

typedef struct s_chunk
{
    size_t          	size;
    bool            	is_freed;
    struct s_chunk*     next_free;
} t_chunk;

#define MAX_CHUNKS (int)(HEAP_SIZE / sizeof(t_chunk))
#define ALIGN 8

_Alignas(8) static uint8_t heap[HEAP_SIZE];
t_chunk* free_list_head = NULL;

void heap_init(void)
{
    t_chunk* initial = (t_chunk*)heap;

    initial->size = HEAP_SIZE - sizeof(t_chunk),
    initial->is_freed = true,
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

    while (curr)
    {
		if (curr->size < aligned_size) // chunk too small, ask next chunk
        {
			printf("asking for next chunk current size: %ld, aligned size: %ld\n", curr->size, aligned_size);
            prev = curr;
            curr = curr->next_free;
			continue;
		} 

        size_t alloc_size = aligned_size + sizeof(t_chunk) + ALIGN;
        printf("heap_alloc: free chunk has size: %ld, alloc_size: %ld\n", curr->size, alloc_size);

        if (curr->size < alloc_size + TOLERATE_DIFF)
        {
			// allocate normally (not split)
            printf("ALLOCATING NORMALLY\n");
			curr->is_freed = false;

			// update free list
            if (!prev)
 				free_list_head = curr->next_free; // becomes NULL if no chunks left
            else
				prev->next_free = curr->next_free; // insert and remove from free list
				
			curr->next_free = NULL; // remove from free list 

            return (void*)(curr + 1); // user data address
		}
        
        printf("SPLITTING\n");

		curr->is_freed = false; // remove from free list

		size_t remaining = curr->size - aligned_size - sizeof(t_chunk); // other half of the chunk that is split

		t_chunk* split = (t_chunk*)((uint8_t*)(curr + 1) + aligned_size);
    	split->size = remaining;
    	split->is_freed = true;
    	split->next_free = curr->next_free;

		printf("split new free chunk: size %ld\n", split->size);
		if (!prev)
		{
    		free_list_head = split;
			printf("updated free list head to: %p\n", free_list_head);
		}
		else
		{
			prev->next_free = split; // insert
		}

		curr->size = aligned_size;
		curr->next_free = NULL;

        return (void*)(curr + 1); // user data address
	}

	fprintf(stderr, "ERROR: heap_alloc no free chunks left\n");
	return NULL;
}

void heap_free(void* ptr)
{
	if (!ptr)
	{
		fprintf(stderr, "ERROR: heap_free called with NULL\n");
		return;
	}

    // append to free list (maybe sorted by addresses?)
	// free ptr user data
	t_chunk* this_chunk = (t_chunk*)(ptr) - 1;
	this_chunk->is_freed = true;

	// insert to free_list
	this_chunk->next_free = free_list_head;
	free_list_head = this_chunk;
}

int main(void)
{
    heap_init();
	/*
    char* root = (char*)heap_alloc(64000-100);

	for (int i = 0; i < 26; ++i)
	{
		root[i] = 'A' + i;
	}
	printf("root: %s\n", root);
	heap_free(root);
	
	printf("root after first free: %s\n", root);
    root = heap_alloc(64000-100);
	
	for (int i = 0; i < 26; ++i)
	{
		root[i] = 'A' + i;
	}
	printf("root: %s\n", root);
	heap_free(root);

	printf("root after second free: %s\n", root);
	*/
	print_free_list();
	char* root1 = heap_alloc(64000-100);
	heap_free(root1);

	print_free_list();
	char* root2 = heap_alloc(64000-100);

	// Assert the allocator returned a **valid pointer inside heap**
	assert(root2 >= (char*)heap && root2 < (char*)heap + HEAP_SIZE);
	heap_free(root2);
	print_free_list();
	
    return 0;
}

