# Malloc
Wrote Malloc in C for CS2110 

## my_malloc()
1. Figure out what size block you need to satisfy the user’s request by adding TOTAL METADATA SIZE to the requested block body size to include the size of the metadata and the tail canary, that will be the real block size we need. (Note: if this size in bytes is over SBRK SIZE, set the error SINGLE REQUEST TOO LARGE and return NULL. If the request size is 0, then mark NO ERROR and return NULL).
2. Now that we have the size we care about, we need to iterate through our freelist to find a block that best fits. Best fit is defined as a block that is exactly the same size, or the smallest block big enough to split and house a new block (MIN BLOCK SIZE is defined for you). If the block is not big enough to split, it is not a valid block and cannot be used.
(a) If the block is exactly the same size, you can simply remove it from the both the address list and size list, set the canaries, and return a pointer to the body of the block.
(b) If the block is big enough to house a new block, we need to split off the portion we will use. Remember: pointer arithmetic can be tricky, make sure you are casting to a uint8 t * before adding the size (in bytes) to find the split pointer!
(c) If no suitable blocks are found at all, then call my sbrk() with SBRK SIZE to get more memory. You must use this macro; failure to do so will result in a lower grade. After setting up its metadata and merging it if possible (in this assignment, there must never be two different blocks in the freelist who are directly adjacent in memory), go through steps (a)-(c). In the event that my sbrk() returns failure (by returning NULL), you should set the error code OUT OF MEMORY and return NULL.
Remember that you want the address you return to be at the start of the block body, not the metadata. This is sizeof (metadata t) bytes away from the metadata pointer. Since pointer arithmetic is in multiples of the sizeof the data type, you can just add 1 to a pointer of type metadata t* pointing to the metadata to get a pointer to the body. If you have not specifically set the error code during this operation, set the error code to NO ERROR before returning.
3. The first call to my malloc() should call my sbrk(). Note that malloc should call my sbrk() when it doesn’t have a block to satisfy the user’s request anyway, so this isn’t a special case.
                       9
#my free()
You are also to write your own version of free that implements deallocation. This means:
1. Calculate the proper address of the block to be freed, keeping in mind that the pointer passed to any
call of my free() is a pointer to the block body and not to the block’s metadata.
2. Check the canaries of the block, starting with the head canary (so that if it is wrong you don’t try to use corrupted metadata to find the tail canary) to make sure they are still their original value. If the canary has been corrupted, set the CANARY CORRUPTED error and return.
3. Attempt to merge the block with blocks that are consecutive in address space with it if those blocks are free. That is, try to merge with the block to its left and its right in memory if they are in the freelist. Finally, place the resulting block in both the address list and size list.
Just like the free() in the C standard library, if the pointer is NULL, no operation should be performed. 1.8 my realloc()
You are to write your own version of realloc that will use your my malloc() and my free() functions. my realloc() should accept two parameters, void *ptr and size t size. If the block’s canaries are valid, it will attempt to effectively change the size of the memory block pointed to by ptr to size bytes, and return a pointer to the beginning of the new memory block. If the canaries are invalid, it returns NULL and sets my malloc errno to CANARY CORRUPTED.
Do not directly change the freelist or blocks in my realloc() — leave that to my malloc() and my free(). This means you don’t need to worry about shrinking or extending blocks in place1; if size is nonzero, just always call my malloc() to attempt to allocate a new block of the new size. Make sure to copy as much data as will fit in the new block from the old block to the new block. The rest of the data in the new block (if any) should be uninitialized.
Your my realloc() implementation must have the same features as the realloc() function in the standard library. For details on what realloc() does and edge cases involved in its implementation, read the realloc manual page by opening a terminal in Ubuntu and typing man realloc.
If my malloc() returns NULL, do not set any error codes (as my malloc will have taken care of that) and just return NULL directly.

## my_calloc()
You are to write your own version of calloc that will use your my malloc() function. my calloc() should accept two parameters, size t nmemb and size t size. It will allocate a region of memory for nmemb number of elements, each of size size, zero out the entire block, and return a pointer to that block.
If my malloc() returns NULL, do not set any error codes (as my malloc() will have taken care of that) and just return NULL directly.

## Error Codes
For this assignment, you will also need to handle cases where users of your malloc do improper things with their code. For instance, if a user asks for 12 gigabytes of memory, this will clearly be too much for your 8 kilobyte heap. It is important to let the user know what they are doing wrong. This is where the enum in
1Even though we don’t extend or shrink blocks in place in this homework, keep in mind that real-world implementations (which are not written in a panic right before finals) very well could.
                            10

the my malloc.h comes into play. You will see the four types of error codes for this assignment listed inside
 of it.
• • • •
They are as follows:
NO ERROR: set whenever my calloc(), my malloc(), my realloc(), and my free() complete suc- cessfully.
OUT OF MEMORY: set whenever the user’s request cannot be met because there’s not enough heap space.
SINGLE REQUEST TOO LARGE: set whenever the user’s requested size plus the total metadata size is beyond SBRK SIZE.
CANARY CORRUPTED: set whenever either canary is corrupted in a block passed to free() or realloc().

## Running Malloc 
You can run the provided tests
```
make run-tests
```
Run gdb
```
make run-gdb
```

