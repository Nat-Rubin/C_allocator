#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>

#include "alloc.h"

// GLOBALS //

void* mem_arena;

//////////////

#define ALIGNMENT_SIZE 32

void* malloc(size_t size) {
    int pagesize = getpagesize();
    int fd = open("/dev/zero", O_RDWR);

    int size_to_mmap = size + sizeof(node_t) + pagesize - ((size + sizeof(node_t)) % pagesize);

	int alloc_size = size + ALIGNMENT_SIZE - (size % ALIGNMENT_SIZE);  // return everything aligned with 32B

    node_t *chunk;

    if (mem_arena == NULL) {  // mem_arena is uninitialized
	    mem_arena = mmap(NULL, size_to_mmap, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);  // void *mmap(void addr[.length], size_t length, int prot, int flags, int fd, off_t offset)
		if (mem_arena == MAP_FAILED) return NULL;  // no space left for allocation


    	chunk = (node_t*)(mem_arena);
		chunk->size = alloc_size;
		chunk->prev = NULL;
		chunk->is_free = false;

    	int next_size = 2*sizeof(node_t) + chunk->size;
    	node_t *next = (void *)chunk + next_size;
    	next->size = pagesize - (2*sizeof(node_t) + chunk->size);
    	next->next = NULL;
    	next->prev = chunk;
		next->is_free = true;

    	chunk->next = next;
		return (void *)(chunk + 1);
    
    } else {

		chunk = (node_t *)(mem_arena);
		while (chunk != NULL) {
			if (chunk->is_free == true && chunk->size >= alloc_size) {
				if (chunk->size == size) {  // if only 32B or size Bytes left
					//should be at least size 32B
					chunk->is_free = false;
					return (void *)(chunk + 1);
				}

				node_t *next = (void *)chunk + 2*sizeof(node_t) + alloc_size;
				next->size = chunk->size - sizeof(node_t) - alloc_size;
				next->next = NULL;
				next->prev = chunk;
				next->is_free = true;

				chunk->size = alloc_size;
				chunk->next = next;
				chunk->is_free = false;
				return (void *)(chunk + 1);
			}
			chunk = chunk->next;
		}
		// if it reaches this point, means there was no chunk with enough room
		mmap(NULL, size_to_mmap, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		if (mem_arena == MAP_FAILED) return NULL;  // no space left for allocation

		chunk = (node_t *)(mem_arena);
		while (chunk->next != NULL) chunk = chunk->next;  // get last chunk in the list
		node_t *next = (void *)(chunk) + chunk->size + sizeof(node_t);
		next->size = alloc_size;
		next->next = NULL;
		next->prev = chunk;
		next->is_free = false;
		return (void *)(next + 1);
	}
}


void* calloc(size_t nelem, size_t elsize) {
	// TODO: fix/make sure this works with 32B alignment and the such of such
	if ((size_t)-1/nelem < elsize) return NULL;  // if overflow, return NULL
	size_t total_size = nelem * elsize;

	void *ptr = alloc(total_size);
	if (ptr == NULL) return NULL;  // no space left for allocation

	node_t *chunk = ptr-sizeof(node_t);

	memset(ptr, 0, chunk->size);  // pretty lazy but oh well. In reality should probably also mmap and such instead of just calling alloc. No need to memset if it was mummapped

	return ptr;
}


void* realloc(void *ptr, size_t size) {
	if (ptr == NULL) return alloc(size);
	node_t *chunk = (node_t *)(ptr - sizeof(node_t));
	if (chunk->is_free == true) return NULL;

	void *new_ptr = alloc(size);  // allocate mem for new size
	if (new_ptr == NULL) return NULL;  // no space left for allocation

	memcpy(new_ptr, ptr, chunk->size);  // copy everything from ptr to new_ptr
	free(ptr);

	return new_ptr;
}


/**
 * Checks if there is degragmentation and coalasces if there is
 * params:
 * 	chunk: the newly freed/created chunk
 * returns addr of available chunk.
 * 	This means that if the prev chunk is free, it returns the addr of the prev chunk
**/
node_t* check_for_defrag(node_t *chunk) {
	if (chunk == mem_arena) return chunk;  // lol stupid tryna coalasce the first chunk in the list with a chunk at -1 lol, so do nothing

	node_t *prev = (node_t *)chunk->prev;
	node_t *next = (node_t *)chunk->next;
	if (prev->is_free == true) {
		prev->size += sizeof(node_t) + chunk->size;
		prev->next = chunk->next;
		chunk = (void *)chunk;  // is this needed because there is nothing linked to it
		return prev;
	} else if (next != NULL && next->is_free == true) {
		chunk->size = sizeof(node_t) + next->size;
		chunk->next = next->next;
		next = (void *)next;
		return chunk;
	}
	return chunk;  // reach here if no coalasce
}

void free(void *ptr) {
	// TODO: maybe return memory to the OS with munmap
	node_t *node_to_free = ptr - sizeof(node_t);
	node_to_free->is_free = true;
	check_for_defrag(node_to_free);
}


int main(void) {
    printf("Hello, World!\n");
    size_t size = 10;
    void* all = alloc(size);
    all = realloc(all, 50);
    //void* call = calloc(10, sizeof(int));
    return 0;
}

