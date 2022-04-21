/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "URmom",
    /* First member's full name */
    "Alexander Martin",
    /* First member's email address */
    "amart50@u.rochester.edu",
    /* Second member's full name (leave blank if none) */
    "Samuel Frank",
    /* Second member's email address (leave blank if none) */
    "sfrank8@u.rochester.edu"
};
/* pg 857 of the textbook */
/* Basic constants and macros */
#define WSIZE      sizeof(void *) /* Word and header/footer size (bytes) */
#define DSIZE      (2 * WSIZE)    /* Doubleword size (bytes), also works for the overhead of the header and footer cuz its a byte */
#define CHUNKSIZE  (1 << 12)      /* Initial heap size (bytes) */

#define MAX(x, y)  ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
/* Pack creates the top and bottom of the block */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
/* Read the size of a block*/
#define GET_SIZE(p)   (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)  (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


#define GET_NEXT(p)  (*(char **)(p))
#define GET_PREV(p)  ((char *)(p + WSIZE))

#define SEG_GET_NEXT(bp) GET(bp)
#define SEG_GET_PREV(bp) GET(GET_PREV(bp))

#define SEG_SET_NEXT(bp, next_block_ptr) PUT(bp, next_block_ptr)
#define SEG_SET_PREV(bp, prev_block_ptr) PUT(GET_PREV(bp), prev_block_ptr)

/* Global variables: */
static char *heap_listp; /* Pointer to first block */

/* Added Global variables: */
size_t seg_count = 8;
static char** tiny_p;
static char** big_p;
static char** seg_p;






/* Give an index of the list from an array based on power of 2*/
static int list_number(size_t size){
    if(size > 16384){
        return 7;
    }else if(size > 8192){
        return 6;
    }else if(size > 4096){
        return 5;
    }else if(size > 2048){
        return 4;
    }else if(size > 1024){
        return 3;
    }else if(size > 512){
        return 2;
    }else if(size > 256){
        return 1;
    }else if(size > 128){
        return 0;
    }else if(size > 64){
        return 7;
    }else if(size > 32){
        return 6;
    }else if(size > 16){
        return 5;
    }else if(size > 8){
        return 4;
    }else if(size > 4){
        return 3;
    }else if(size > 3){
        return 2;
    }else if(size > 2){
        return 1;
    }else{
        return 0;
    }
}

/* Add newly freed block pointer to the segregated list of appropriate size */
static void add_to_list(void *new){
    size_t size = GET_SIZE(HDRP(new));
    size_t index = list_number(size);
    
    if(size > 128){
        SEG_SET_NEXT(new, (size_t) big_p[index]);
        SEG_SET_PREV(new, (size_t) NULL);
        
        if(big_p[index] != NULL){
            SEG_SET_PREV(big_p[index], (size_t) new);   
        }
        big_p[index] = new;
    }else{
        SEG_SET_NEXT(new, (size_t) tiny_p[index]);
        SEG_SET_PREV(new, (size_t) NULL);
        
        if(tiny_p[index] != NULL){
            SEG_SET_PREV(tiny_p[index], (size_t) new);   
        }
        tiny_p[index] = new;
    }
    
    
//     size_t listnum = list_number(GET_SIZE(HDRP(new)));// Get appropriate sized list for newly freed block to be placed in
    
//     /* Organizes the newly freed block and 'sets' as head */
//     SEG_SET_NEXT_BLKP(new, (size_t) seg_p[listnum]);     // Places newly freed block in the beginning of the list
//     SEG_SET_PREV_BLKP(new, (size_t) NULL);              // Sets the previous pointer to NULL

//     /* Adds the newly freed block into a non-empty list*/
//     if (seg_p[listnum] != NULL){
//         SEG_SET_PREV_BLKP(seg_p[listnum], (size_t) new);
//     }
//     seg_p[listnum] = new; //places the newly freed block pointer as head of list
//     return;
}

/* Remove newly filled (free) block from the segregated list of free blocks*/
static void fill_block(void *current){
    size_t size = GET_SIZE(HDRP(current));
    size_t index = list_number(size);
    
    if(size > 128){
        if(SEG_GET_PREV(current) != (size_t) NULL){
            SEG_SET_NEXT(SEG_GET_PREV(current), SEG_GET_NEXT(current));   
        }else{
            big_p[index] = (char*) SEG_GET_NEXT(current);
        }
        
        if(SEG_GET_NEXT(current) != (size_t) NULL){
            SEG_SET_PREV(SEG_GET_NEXT(current), SEG_GET_PREV(current));   
        }
    }else{
        if(SEG_GET_PREV(current) != (size_t) NULL){
            SEG_SET_NEXT(SEG_GET_PREV(current), SEG_GET_NEXT(current));   
        }else{
            tiny_p[index] = (char*) SEG_GET_NEXT(current);
        }
        
        if(SEG_GET_NEXT(current) != (size_t) NULL){
            SEG_SET_PREV(SEG_GET_NEXT(current), SEG_GET_PREV(current));   
        }  
    }

//     int id = list_number(GET_SIZE(HDRP(current)));   // Get appropriate sized list for newly freed block to be removed

//     /* Checks to see if the current free block is not the head of its segregated list*/
//     if (SEG_GET_PREV_BLKP(current) != (size_t) NULL){ //If not head
//             SEG_SET_NEXT_BLKP(SEG_GET_PREV_BLKP(current),SEG_GET_NEXT_BLKP(current)); //Sets the previous block's next pointer to current's next block
//     }
//     else{
//         seg_p[id] = (char*) SEG_GET_NEXT_BLKP(current); //If head
//     }

//     /* Checks to see if the current free block is not the tail of its segregated lists*/
//     if (SEG_GET_NEXT_BLKP(current) != (size_t) NULL){ //If not tail
//             SEG_SET_PREV_BLKP(SEG_GET_NEXT_BLKP(current), SEG_GET_PREV_BLKP(current)); //Sets the next block's previous pointer to current's previous block
//     }
//     return;
}

/* Coalesce Function to help reduce fragmentation for segmentation implementation*/
static void *coalesce_seg(void *bp){
    /* Case 1 : previous and next allocated*/
    if (GET_ALLOC(FTRP(PREV_BLKP(bp))) && GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
        add_to_list(bp);                               // Add to free list
    }

    /* Case 2 : Next Block is Free*/
    if (GET_ALLOC(FTRP(PREV_BLKP(bp))) && !GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
        void *next = NEXT_BLKP(bp);
        fill_block(next); //remove 'next' free block pointer from its segregated list

        /* Handles the different cases of combining the next's free block and the current's free block*/
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t size = GET_SIZE(HDRP(bp));
        if (prev_alloc && !next_alloc) {                // Case 2 : next block free, combine
            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));
        } else if (!prev_alloc && next_alloc) {         // Case 3 : previous block free, combine
            size += GET_SIZE(HDRP(PREV_BLKP(bp)));
            PUT(FTRP(bp), PACK(size, 0));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);
        } else {                                        // Case 4 : both next and previous blocks free, combine
            size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);
        }
        add_to_list(bp); //add combined block to free list
    }
    /* Case 3: Previous Block is Free*/
    if (!GET_ALLOC(FTRP(PREV_BLKP(bp))) && GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
        void *prev = PREV_BLKP(bp);
        fill_block(prev);
        
        /* Handles the different cases of combining the next's free block and the current's free block*/
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t size = GET_SIZE(HDRP(bp));
        if (prev_alloc && !next_alloc) {                // Case 2 : next block free, combine
            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));
        } else if (!prev_alloc && next_alloc) {         // Case 3 : previous block free, combine
            size += GET_SIZE(HDRP(PREV_BLKP(bp)));
            PUT(FTRP(bp), PACK(size, 0));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);
        } else {                                        // Case 4 : both next and previous blocks free, combine
            size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);
        }
        add_to_list(bp); //add combined free block to segmented free list
    }

    /* Case 4: Next and Previous Blocks are Free*/
    if (!GET_ALLOC(FTRP(PREV_BLKP(bp))) && !GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
        void *prev = PREV_BLKP(bp);
        void *next = NEXT_BLKP(bp);
        fill_block(prev);
        fill_block(next);
        
        /* Handles the different cases of combining the next's free block and the current's free block*/
        size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t size = GET_SIZE(HDRP(bp));
        if (prev_alloc && !next_alloc) {                // Case 2 : next block free, combine
            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));
        } else if (!prev_alloc && next_alloc) {         // Case 3 : previous block free, combine
            size += GET_SIZE(HDRP(PREV_BLKP(bp)));
            PUT(FTRP(bp), PACK(size, 0));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);
        } else {                                        // Case 4 : both next and previous blocks free, combine
            size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);
        }
        add_to_list(bp); //add combined free block to segmented free lists
    }
    return bp;
}

/* Extends the heap with a free block and returns the pointer of that free block*/
static void *extend_heap(size_t words){
    void *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment. */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((bp = mem_sbrk(size)) == (void *)-1) {
        return (NULL);
    }

    /* Initialize free block header/footer and the epilogue header. */
    PUT(HDRP(bp), PACK(size, 0));         // Free block header
    PUT(FTRP(bp), PACK(size, 0));         // Free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // New epilogue header

    return bp;
}

/* Places a block of size (asize) at the start of the free block bp */
static void place(void *bp, size_t asize, int heapExtended)
{
    size_t csize = GET_SIZE(HDRP(bp)); //Gets the current block size

    if (heapExtended == 1) {                       //when heap is extended
        /*if current block size is greater than the asize, splicing takes place */
        if ((csize - asize) >= (2 * DSIZE)) {
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            /*splice block*/
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp), PACK(csize - asize, 0));
            PUT(FTRP(bp), PACK(csize - asize, 0));
            add_to_list(bp); //Put the newly spliced free block at front of the free list
        } else {                    /*else no splicing, getting the whole block*/
            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));
        }
    }
    else {
        /*if current block size is greater than the asize, splicing takes place */
        if ((csize - asize) >= (2 * DSIZE)) {
            fill_block(bp); //removes the recently filled block from free list
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            /*splice block*/
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp), PACK(csize - asize, 0));
            PUT(FTRP(bp), PACK(csize - asize, 0));
            add_to_list(bp); //Put the newly spliced free block at front of the free list
        } else {
            /*else no splicing necessary, they are getting the whole block*/
            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));
            fill_block(bp); //removes the recently filled block from free list
        }
    }
}

/* Adopted the best fit policy for finding a free block */
static void *segregated_best_fit(size_t asize){
    size_t min_diff = 9999999; //different between the sizes, init really large
    void *bestfit = NULL; //pointer for best fit block
        
    /* Searching segregated lists whose block size is greater than or equal to the asize */
    int index = list_number(asize);
    void *bp  
    
    if(asize <= 128){
        for(int i = index; i < seg_count; i++){
            bp = tiny_p[i];
            while(bp != NULL){
                size_t csize = GET_SIZE(HDRP(bp));
                if(!GET_ALLOC(HDRP(bp)) && (asize <= csize)){
                    size_t diff = csize;
                    if(diff < min_diff){
                        min_diff = diff;
                        bestfit = bp;
                    }
                }
                bp = (void*) SEG_GET_NEXT(bp)
            }
            if(bestfit != NULL){
                return bestfit;   
            }
        }
    }
    if(bestfit != NULL){
        return bestfit;
    }
    for(int i = index; i < seg_count; i++){
        bp = big_p[i];
        while(bp != NULL){
            size_t csize = GET_SIZE(HDRP(bp));
            if(!GET_ALLOC(HDRP(bp)) && (asize <= csize)){
                size_t diff = csize;
                if(diff < min_diff){
                    min_diff = diff;
                    bestfit = bp;
                }
            }
            bp = (void*) SEG_GET_NEXT(bp);
        }
        if(bestfit != NULL){
            return bestfit;   
        }
    }
    
    return bestfit;
    
    
//     if(asize > 128){
//         for (i = id; i < seg_count; i++) {
//             void *bp = big_p[i];
//             while (bp != NULL) {                          //goes through the linked list of each segregated free lists
//                 size_t curr_size = GET_SIZE(HDRP(bp));
//                 if (!GET_ALLOC(HDRP(bp)) && (asize <= curr_size)) {
//                     size_t diff = curr_size;
//                     /*finds best fit from the list by finding the minimum difference*/
//                     if (diff < min_diff) {
//                         min_diff = diff;
//                         bestfit = bp;
//                     }
//                 }
//                 bp = (void*) SEG_GET_NEXT_BLKP(bp); //iterate to next pointer within the segregated list
//             }
        
//             /*if we get a fit in the list, no need to check other lists of higher sizes.*/
//             if(bestfit != NULL)
//                 return bestfit;
//             }
//         }
//     }else{
//         for (i = id; i < seg_count; i++) {
//             void *bp = tiny_p[i];
//             while (bp != NULL) {                          //goes through the linked list of each segregated free lists
//                 size_t curr_size = GET_SIZE(HDRP(bp));
//                 if (!GET_ALLOC(HDRP(bp)) && (asize <= curr_size)) {
//                     size_t diff = curr_size;
//                     /*finds best fit from the list by finding the minimum difference*/
//                     if (diff < min_diff) {
//                         min_diff = diff;
//                         bestfit = bp;
//                     }
//                 }
//                 bp = (void*) SEG_GET_NEXT_BLKP(bp); //iterate to next pointer within the segregated list
//             }
        
//             /*if we get a fit in the list, no need to check other lists of higher sizes.*/
//             if(bestfit != NULL)
//                 return bestfit;
//             }
//         }
//     }
    
//     for (i = id; i < seg_count; i++) {
//         void *bp = big_p[i];
//             while (bp != NULL) {                          //goes through the linked list of each segregated free lists
//                 size_t curr_size = GET_SIZE(HDRP(bp));
//                 if (!GET_ALLOC(HDRP(bp)) && (asize <= curr_size)) {
//                     size_t diff = curr_size;
//                     /*finds best fit from the list by finding the minimum difference*/
//                     if (diff < min_diff) {
//                         min_diff = diff;
//                         bestfit = bp;
//                     }
//                 }
//                 bp = (void*) SEG_GET_NEXT_BLKP(bp); //iterate to next pointer within the segregated list
//             }
        
//         /*if we get a fit in the list, no need to check other lists of higher sizes.*/
//         if(bestfit!= NULL)
//             return bestfit;
//     }
//     return bestfit;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void){
    if((tiny_p = mem_sbrk(seg_count*WSIZE)) == (void *)-1)  /*allocate space for segregated list in heap*/
        return -1;
    if((big_p = mem_sbrk(seg_count*WSIZE)) == (void *)-1)  /*allocate space for segregated list in heap*/
        return -1;
    
    /* Create the initial empty heap. */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2 * WSIZE);
    /* Initializes the the segregated list to NULL*/
//     int i;
    for(int i = 0; i < seg_count; i++){
        tiny_p[i] = NULL;
        big_p[i] = NULL;
    }
//     for (i = 0; i < seg_count; i++)
//         seg_p[i] = NULL;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void * mm_malloc(size_t size){
    size_t asize;                           /* Adjusted block size */
    size_t extendsize;                      /* Amount to extend heap if no fit */
    void *bp;

    /* Ignore stupid calls */
    if (size == 0)
        return (NULL);

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit with the best fit policy */
    if ((bp = segregated_best_fit(asize)) != NULL) {
        place(bp, asize,0);
        return (bp);
    }

    /* No fit found.  Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return (NULL);
    place(bp, asize,1);
    
    return (bp);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp){
    size_t size;

    /* Ignore stupid calls */
    if (bp == NULL)
        return;

    /* Frees and coalesce blocks */
    size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce_seg(bp);
}

/*
 * Requires:
 *   "ptr" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Reallocates the block "ptr" to a block with at least "size" bytes of
 *   payload, unless "size" is zero.  If "size" is zero, frees the block
 *   "ptr" and returns NULL.  If the block "ptr" is already a block with at
 *   least "size" bytes of payload, then "ptr" may optionally be returned.
 *   Otherwise, a new block is allocated and the contents of the old block
 *   "ptr" are copied to that new block.  Returns the address of this new
 *   block if the allocation was successful and NULL otherwise.
 */
void *mm_realloc(void *ptr, size_t size){
    size_t oldsize;
    void *newptr;

    /* If size == 0 , free the block. */
    if (size == 0){
        mm_free(ptr);
        return NULL;
    }

    /*If the realloc'd block has previously been given more size than it needs, perhaps
        this realloc request can be serviced within the same block:*/
    size_t curSize = GET_SIZE(HDRP(ptr));
    if (size < curSize-2*WSIZE) {
        return ptr;
    }

        
    /*If next block is not allocated, realloc request can be serviced by merging both blocks*/
    void *next = NEXT_BLKP(ptr);
    int next_alloc = GET_ALLOC(HDRP(next));
    
    size_t coalesce_size = (GET_SIZE(HDRP(next)) + GET_SIZE(HDRP(ptr)));
    if (!next_alloc && size <= coalesce_size-2*WSIZE){
        fill_block(next);
        PUT(HDRP(ptr), PACK(coalesce_size, 1));
        PUT(FTRP(ptr), PACK(coalesce_size, 1));
        return ptr;
    }

    /* If old ptr is NULL, then this is just malloc. */
    if (ptr == NULL){
        return (mm_malloc(size));
    }
    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if (newptr == NULL){
        return (NULL);
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if (size < oldsize)
        oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return (newptr);
}
