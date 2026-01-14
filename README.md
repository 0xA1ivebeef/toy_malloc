
this is a toy version of a memory allocator that uses a static stack array
as heap instead of virtual memory from the os with sbrk() or mmap()

it implements some cool features that actual memory allocators use like 
coalescence and 8 byte alignment so standard 64bit system and in-band metadata

the metadata for each chunk is just the size and the next_free chunk
which implements a linked list of free chunks. it lives before the user_data
ptr thats given by heap_alloc so directly in the heap

