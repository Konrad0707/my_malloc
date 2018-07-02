/*
 * CS 2110 Spring 2018
 * Author: Kai-Wei Wang
 */

/* we need this for uintptr_t */
#include <stdint.h>
/* we need this for memcpy/memset */
#include <string.h>
/* we need this to print out stuff*/
#include <stdio.h>
/* we need this for my_sbrk */
#include "my_sbrk.h"
/* we need this for the metadata_t struct and my_malloc_err enum definitions */
#include "my_malloc.h"

/* Our freelist structure - our freelist is represented as two doubly linked lists
 * the address_list orders the free blocks in ascending address
 * the size_list orders the free blocks by size
 */

metadata_t *address_list;
metadata_t *size_list;

/* Set on every invocation of my_malloc()/my_free()/my_realloc()/
 * my_calloc() to indicate success or the type of failure. See
 * the definition of the my_malloc_err enum in my_malloc.h for details.
 * Similar to errno(3).
 */
enum my_malloc_err my_malloc_errno;


static void addToAddr(metadata_t *addr);
static void addToSize(metadata_t *addr);
static void my_remove(metadata_t *remove_block);
static void *findSize(size_t size);
static void *blockAlloc(size_t size);
static void merge(metadata_t *addr1, metadata_t *addr2);

/* MALLOC
 * See my_malloc.h for documentation
 */
void *my_malloc(size_t size) {
	//set error to no error
    my_malloc_errno = NO_ERROR;

    if (size == 0) {
    	return NULL;
    }
    size_t nSize = size + TOTAL_METADATA_SIZE;
    if (nSize > SBRK_SIZE) {
    	my_malloc_errno = SINGLE_REQUEST_TOO_LARGE;
    	return NULL;
    }
	int moved = 1;
	while(moved){
        metadata_t *newblock = findSize(nSize);
        if(!newblock){
        	newblock = blockAlloc(nSize);
        }
        if(!newblock) {
        	metadata_t *thisblock = my_sbrk(SBRK_SIZE);
        	if (!thisblock) {
                my_malloc_errno = OUT_OF_MEMORY;
                return NULL;
            }
            thisblock->prev_size = NULL;
            thisblock->next_size = NULL;
            thisblock->prev_addr = NULL;
            thisblock->next_addr = NULL;
            thisblock->size = SBRK_SIZE;

            thisblock->canary = ((uintptr_t)thisblock^CANARY_MAGIC_NUMBER) + 1;
            unsigned long *my_ptr = (unsigned long*)((uint8_t*)thisblock + thisblock->size - sizeof(unsigned long));
			*my_ptr = ((uintptr_t)thisblock^CANARY_MAGIC_NUMBER) + 1;
			addToSize(thisblock);
			addToAddr(thisblock);
            
        } else {
    	moved = 0;
    	return newblock;
        }
    }    
    return NULL;
    
}

static void *findSize(size_t size) {
	metadata_t *this = size_list;
	while(this){
		if(this->size == size) {
			my_remove(this);
			this->canary = ((uintptr_t)this^CANARY_MAGIC_NUMBER) + 1;
			unsigned long *canary_ptr = (unsigned long*)((uint8_t*)this-sizeof(unsigned long)+ this->size);
			*canary_ptr = ((uintptr_t)this^CANARY_MAGIC_NUMBER) + 1;
			return this + 1;
		}
		this = this->next_size;
	}
	return NULL;
}

static void *blockAlloc(size_t size) {
	metadata_t* cur = size_list;

	while(cur!= NULL){
		if (cur->size >= size + MIN_BLOCK_SIZE){
			metadata_t *block = (metadata_t*)((uint8_t*)cur + cur->size -size);
			block->size = size;
			block->canary = ((uintptr_t)block ^ CANARY_MAGIC_NUMBER) + 1;
			block->prev_addr = NULL;
			block->prev_size = NULL;
			block->next_addr = NULL;
			block->next_size = NULL;
			// assign canary
			unsigned long *canary_ptr = (unsigned long*)((uint8_t*)block+block->size -sizeof(unsigned long));
			*canary_ptr = ((uintptr_t)block ^ CANARY_MAGIC_NUMBER) + 1;
			
			my_remove(cur);
			
			cur->size = cur->size -size;
			addToSize(cur);
			addToAddr(cur);
			
			my_malloc_errno = NO_ERROR;
			return block +1;
			
		}
		cur = cur->next_size;
	}
	return NULL;
}

/* REALLOC
 * See my_malloc.h for documentation
 */
void *my_realloc(void *ptr, size_t size) {
	if(!ptr) {
		return my_malloc(size);
	}
	if(size == 0){
		my_free(ptr);
		return NULL;
	}
	if (size + TOTAL_METADATA_SIZE > SBRK_SIZE) {
		my_malloc_errno = SINGLE_REQUEST_TOO_LARGE;
		return NULL;
	}
	metadata_t *new = (metadata_t*) ((char*)ptr - sizeof(metadata_t));

	unsigned long *my_ptr = (unsigned long*)((uint8_t*)new+ new->size - sizeof(unsigned long));
	
	if (new->canary != ((uintptr_t)new^CANARY_MAGIC_NUMBER) + 1) {
		my_malloc_errno = CANARY_CORRUPTED;
		return NULL;
	}
	if (*my_ptr != ((uintptr_t)new^CANARY_MAGIC_NUMBER) + 1) {
		my_malloc_errno = CANARY_CORRUPTED;
		return NULL;
	}
	metadata_t *thisblock = my_malloc(size);
	if(!thisblock) {
		return NULL;
	}
	if (size < new->size) {
		memcpy(thisblock, ptr, size);
	} else {
		memcpy(thisblock, ptr, new->size);
	}
	my_free(ptr);
	return thisblock;

}

/* CALLOC
 * See my_malloc.h for documentation
 */
void *my_calloc(size_t num, size_t size) {
	metadata_t* a = my_malloc(num * size);
	if(!a) {
		return NULL;
	}
	my_malloc_errno = NO_ERROR;
	a = memset(a, 0, num * size);
    return a;
}

/* FREE
 * See my_malloc.h for documentation
 */
void my_free(void *ptr) {
	my_malloc_errno = NO_ERROR;
	if(ptr == NULL) {
		return;
	}
	ptr = (char*) ptr - sizeof(metadata_t);
	unsigned long *end = (unsigned long*) ((uint8_t*)ptr -sizeof(unsigned long) + ((metadata_t*)ptr)->size);
	if(((metadata_t*) ptr)->canary != ((uintptr_t)ptr^CANARY_MAGIC_NUMBER) +1) {
		my_malloc_errno = CANARY_CORRUPTED;
		return;
	}

	if (*end != ((uintptr_t)ptr^CANARY_MAGIC_NUMBER) +1){
		my_malloc_errno = CANARY_CORRUPTED;
		return;
	}

	addToSize(ptr);
	addToAddr(ptr);
}


static void addToAddr(metadata_t *address) {
	if(!address_list) {
		address_list = address;
		return;
	}
	if((uintptr_t)address_list > (uintptr_t)address) {

		address->next_addr = address_list;
		address_list->prev_addr = address;
		address->prev_addr = NULL;
		address_list = address;
		if ((uintptr_t)((char*)address + address->size) == (uintptr_t)address->next_addr) {
			merge(address, address->next_addr);
		}
		return;
	}
	
	metadata_t *current = address_list;
	while(current->next_addr != NULL) {
		current = current->next_addr;
	}
	if(address <= current) {
		metadata_t *current = address_list;
		while ((uintptr_t)address>(uintptr_t)current) {
			current = current->next_addr;
		}
		current->prev_addr->next_addr = address;
		
		address->prev_addr = current->prev_addr;
		address->next_addr = current;
		current->prev_addr = address;

		if((uintptr_t)address == (uintptr_t)((char*)address->prev_addr + address->prev_addr->size)) {
			merge(address, address->prev_addr);
		} else if ((uintptr_t)address->next_addr == (uintptr_t)((char*)address+address->size)){
			merge(address, address->next_addr);
		}

	} else {
		address->next_addr = NULL;
		address->prev_addr = current;
		current->next_addr = address;
		if ((uintptr_t)address ==(uintptr_t)((char*)current +current->size)) {
			merge(current, address);
		}
	}
}

static void addToSize(metadata_t *addr) {
	if(!size_list){
		size_list = addr;
		return ;
	}
	if(size_list->size > addr->size) {
		addr->next_size = size_list;
		size_list->prev_size = addr;
		size_list = addr;
		size_list->prev_size = NULL;
		return;
	}
	metadata_t* curr = size_list;
	while (curr->next_size != NULL){
		curr = curr->next_size;
	}
	if(curr->size <= addr->size){
		addr->prev_size = curr;
		curr->next_size = addr;

		addr->next_size = NULL;
		return;
	}
	curr = size_list;
	while (curr->size <= addr->size) {
		curr = curr->next_size;
	}
	addr->prev_size = curr->prev_size;
	curr->prev_size->next_size = addr;
	addr->next_size = curr;
	curr->prev_size = addr;

}

static void merge(metadata_t *a, metadata_t *b) {
	metadata_t *temp = a;
	my_remove(a);
	my_remove(b);
	if((uintptr_t)a >= (uintptr_t)b) {
		temp = b;
	}
	temp->size = a->size + b->size;
	unsigned long *my_ptr = (unsigned long*)((uint8_t*)temp + temp->size - sizeof(unsigned long));
	*my_ptr = ((uintptr_t)temp^CANARY_MAGIC_NUMBER) + 1;
	addToSize(temp);
	addToAddr(temp);
}



static void my_remove(metadata_t *item) {
	if (item->prev_addr != NULL && item->next_addr != NULL) {
		metadata_t *newb = address_list;
		while(newb!=item) {
			newb = newb->next_addr;
		}
		// remove newb
		newb->next_addr->prev_addr = newb->prev_addr;
		newb->prev_addr->next_addr = newb->next_addr;
		newb->next_addr = NULL;
		newb->prev_addr = NULL;
	} else {
		if(item->prev_addr == NULL && item->next_addr != NULL){
			item->next_addr->prev_addr = NULL;
			address_list=item->next_addr;
			item->next_addr = NULL;
			
		} else if (item->prev_addr!=NULL) {
			item->prev_addr->next_addr = NULL;
			item->prev_addr = NULL;
		} else {
			address_list = NULL;
		}
	}

	//remove size list
	if(item->prev_size != NULL && item->next_size != NULL) {
		metadata_t *newb = size_list;
		while(newb!=item){
			newb = newb->next_size;
		}
		newb->prev_size->next_size=newb->next_size;
		newb->next_size->prev_size = newb->prev_size;
		newb->next_size = NULL;
		newb->prev_size = NULL;
	}else {
		if(item->prev_size ==NULL && item->next_size != NULL){
			item->next_size->prev_size = NULL;
			size_list = item->next_size;
			item->next_size = NULL;
		}else if(item->prev_size!= NULL) {
			item->prev_size->next_size = NULL;
			item->prev_size = NULL;
		}else {
			size_list = NULL;
		}
	}
}