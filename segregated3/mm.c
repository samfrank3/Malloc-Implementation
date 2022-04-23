/*
 
 
 
 
 REALLOC:
 Since we know that all size will be multiples of 8, we know that we can store the realloc in the second to last bit
 the same way we stored the free bit.
 
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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE      4 /* Word and header/footer size (bytes) */
#define DSIZE      (2 * WSIZE)    /* Doubleword size (bytes), also works for the overhead of the header and footer cuz its a byte */
#define CHUNKSIZE  (1<<9)      /* Initial heap size (bytes) */

#define MAX(x, y)  ((x) > (y) ? (x) : (y))
#define MIN(x, y)  ((x) < (y) ? (x) : (y))

/* Pack a size and 1 bit into a word */
/* Pack creates the top and bottom of the block */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))

/* Read the size and 1 fields from address p */
/* Read the size of a block*/
#define GET_SIZE(p)   (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)  (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define GET_NEXT(p)  ((char *)(p))
#define GET_PREV(p)  ((char *)(p + WSIZE))

/* GET SEGREGATED LIST NEXT AND PREV BLOCKS */
#define SEG_GET_NEXT(bp) GET(GET_NEXT(bp))
#define SEG_GET_PREV(bp) GET(GET_PREV(bp))

/* SET SEGREGATED LIST NEXT AND PREV BLOCKS for a block with block pointer bp */
#define SEG_SET_NEXT(bp, next_block_ptr) PUT(bp, next_block_ptr)
#define SEG_SET_PREV(bp, prev_block_ptr) PUT(GET_PREV(bp), prev_block_ptr)

/*SET AND GET REALLOC BIT*/
#define GET_RALLOC(p) (GET(p) & 0x2)
#define SET_RALLOC(p) (GET(p) | 0x2)

/* Global variables: */
static char *heap_listp; /* Pointer to first block */

/* Added Global variables: */
size_t num_buckets = 15;
static char *seg_p;


/* Give an index of the list from an array based on power of 2*/
static int get_index(size_t size){
    
    
    if(size > 262144){//2^18
        return 14;
    }else if(size > 131072){//2^17
        return 13;
    }else if(size > 65536){//2^16
        return 12;
    }else if(size > 32768){//2^15
        return 11;
    }else if(size > 16384){//2^14
        return 10;
    }else if(size > 8192){//2^13
        return 9;
    }else if(size > 4096){//2^12
        return 8;
    }else if(size > 2048){//2^11
        return 7;
    }else if(size > 1024){//2^10
        return 6;
    }else if(size > 512){//2^9
        return 5;
    }else if(size > 256){//2^8
        return 4;
    }else if(size > 128){ //2^7
        return 3;
    }else if(size > 64){//2^6
        return 2;
    }else if(size > 32){ //2^5
        return 1;
    }else{//2^4
        return 0;
    }
    
}

/* Add newly freed block pointer to the segregated list of appropriate size */
static void add_to_free_list(void *new)
{
    
    int index = get_index(GET_SIZE(HDRP(new)));
    void *head = seg_p + index * WSIZE;
//
//    if(SEG_GET_NEXT(head)){
//        void *next = (void*)SEG_GET_NEXT(head);
//        SEG_SET_NEXT(head, (size_t) new);
//        SEG_SET_NEXT(new,(size_t) next);
//        SEG_SET_PREV(new, (size_t) head);
//        SEG_SET_PREV(next, (size_t) new);
//    }else{
//        SEG_SET_NEXT(new, (size_t) NULL);
//        SEG_SET_PREV(new, (size_t) NULL);
//    }
    
    void *next = head;
    
    
    while(SEG_GET_NEXT(next)){
        next = (void *)SEG_GET_NEXT(next);
        if((size_t)next >= (size_t)new){

            void *tmp = next;
            next = (void *)SEG_GET_PREV(next);
            PUT(GET_NEXT(next), (size_t) new);
            PUT(GET_PREV(new), (size_t) next);
            PUT(GET_NEXT(new), (size_t) tmp);
            PUT(GET_PREV(tmp), (size_t) new);
            return;
        }
    }

    PUT(GET_NEXT(next), (size_t) new);
    PUT(GET_PREV(new), (size_t) next);
    PUT(GET_NEXT(new), (size_t) NULL);
    
}

/* Remove newly filled (free) block from the segregated list of free blocks*/
static void fill_block(void *current)
{
    if(SEG_GET_NEXT(current) && SEG_GET_PREV(current)){
        PUT(GET_NEXT(SEG_GET_PREV(current)), SEG_GET_NEXT(current));
        PUT(GET_PREV(SEG_GET_NEXT(current)), SEG_GET_PREV(current));
    }
    else if(SEG_GET_PREV(current)){
        PUT(GET_NEXT(SEG_GET_PREV(current)), (size_t) NULL);
    }

    PUT(GET_NEXT(current), (size_t) NULL);
    PUT(GET_PREV(current), (size_t) NULL);
}

/* Coalesce Function to help reduce fragmentation for segmentation implementation*/
static void *coalesce(void * bp)
{
    size_t prev_alloc =  GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc =  GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){
        add_to_free_list(bp);
        return bp;
    }else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        fill_block(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(GET_PREV(bp), (size_t) NULL);
        PUT(GET_NEXT(bp), (size_t) NULL);
    }else if(!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        fill_block(PREV_BLKP(bp));

        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        bp = PREV_BLKP(bp);
        PUT(GET_PREV(bp), (size_t) NULL);
        PUT(GET_NEXT(bp), (size_t) NULL);
    }else {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        fill_block(PREV_BLKP(bp));
        fill_block(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        PUT(GET_PREV(bp), (size_t) NULL);
        PUT(GET_NEXT(bp), (size_t) NULL);
    }
    add_to_free_list(bp);
    return bp;
}


/* Extends the heap with a free block and returns the pointer of that free block*/
static void *extend_heap(size_t asize)
{
    void *bp;

    if((bp = mem_sbrk(asize)) == (void*)-1){
        return NULL;
    }
    
    PUT(HDRP(bp), PACK(asize, 0));
    PUT(FTRP(bp), PACK(asize, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
   
    return coalesce(bp);
}


/* Places a block of size (asize) at the start of the free block bp */
static void *place(char *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if(!GET_ALLOC(HDRP(bp)))
        fill_block(bp);
    if((csize-asize) >= 2*DSIZE){
        if(asize > 64){
            PUT(HDRP(bp), PACK((csize-asize), 0));
            PUT(FTRP(bp), PACK((csize-asize), 0));
            PUT(HDRP(NEXT_BLKP(bp)), PACK(asize, 1));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(asize, 1));
            add_to_free_list(bp);
            return NEXT_BLKP(bp);
        }
        else{
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            PUT(HDRP(NEXT_BLKP(bp)), PACK((csize-asize), 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK((csize-asize), 0));

            coalesce(NEXT_BLKP(bp));
        }
    }
    else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));

        
    }
    return bp;
}

/* Adopted the best fit policy for finding a free block */
static void *find_fit(size_t asize, int index)
{
    size_t min = (size_t) 9999999;
    void *bestfit = NULL;
    while(index < num_buckets){
        void *head  = seg_p + index * WSIZE;
        void *bp = (void*)SEG_GET_NEXT(head);
        while(bp){
            size_t csize = GET_SIZE(HDRP(bp));
            if(csize >= asize){
                if(csize < min){
                    min = csize;
                    bestfit = bp;
                }
            }
            bp = (void*)SEG_GET_NEXT(bp);
        }
        index++;
    }
    return bestfit;
//    for(int i = index; i < num_buckets; i++){
//        void *head = seg_p + index * WSIZE;
//        void *bp = (void*)SEG_GET_NEXT(head);
//        while(bp != NULL){
//            size_t csize = GET_SIZE(HDRP(bp));
//            if(!GET_ALLOC(HDRP(bp)) && (asize <= csize)){
//                if(min == (size_t) NULL){
//                    min = csize;
//                    bestfit = bp;
//                }else if(csize < min){
//                    min = csize;
//                }
//            }
//            bp = (void*)SEG_GET_NEXT(bp);
//        }
//        if(bestfit != NULL){
//            return bestfit;
//        }
//    }
//    return bestfit;
//    while(index < num_buckets){
//        char *head = seg_p + index * WSIZE;
//        char *bp = (char *)SEG_GET_NEXT(head);
//        while(bp){
//            if((size_t)GET_SIZE(HDRP(bp)) >= asize)
//                return bp;
//
//            bp = (char *)SEG_GET_NEXT(bp);
//        }
//        index++;
//    }
//    return NULL;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{

    if((heap_listp = mem_sbrk((num_buckets + 3) * WSIZE)) == (void *)-1)
        
        return -1;
    int i;

    for(i = 0; i < num_buckets; ++i)
        PUT(heap_listp + i*WSIZE, (size_t)NULL);

    
    PUT(heap_listp + (i+0)*WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + (i+1)*WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + (i+2)*WSIZE, PACK(0, 1));

    seg_p = heap_listp;
    heap_listp += (i+1)*WSIZE;

    
    if(extend_heap(CHUNKSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if(size == 0)
        return NULL;
    
    size_t align_size;
    if(size > DSIZE){
        align_size = DSIZE * ((size + DSIZE + (DSIZE - 1))/DSIZE);
    }else{
        align_size = 2*DSIZE;
    }
    //size_t asize = align_size(size);
    size_t extendsize;
    void *bp;

    if((bp = find_fit(align_size, get_index(align_size))) != NULL)
        return place(bp, align_size);

    extendsize = MAX(align_size, CHUNKSIZE);
    
    if((bp = extend_heap(extendsize)) == NULL)
        return NULL;

    return place(bp, align_size);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    void *bp = ptr;
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * Requires:
 *   "ptr" is either the address of an 1 block or NULL.
 *
 * Effects:
 *   Reallocates the block "ptr" to a block with at least "size" bytes of
 *   payload, unless "size" is zero.  If "size" is zero, frees the block
 *   "ptr" and returns NULL.  If the block "ptr" is already a block with at
 *   least "size" bytes of payload, then "ptr" may optionally be returned.
 *   Otherwise, a new block is 1 and the contents of the old block
 *   "ptr" are copied to that new block.  Returns the address of this new
 *   block if the allocation was GET_NEXTessful and NULL otherwise.
 */
void *mm_realloc(void *ptr, size_t size)
{
    if(ptr == NULL)
        return mm_malloc(size);
    
    else if(size == 0){
        mm_free(ptr);
        return NULL;
    }
    size_t align_size;
    if(size > DSIZE){
        align_size = DSIZE * ((size + DSIZE + (DSIZE - 1))/DSIZE);
    }else{
        align_size = 2*DSIZE;
    }
    size_t old_size = GET_SIZE(HDRP(ptr));
    void *new_ptr;

    if(old_size == align_size)
        return ptr;
    
    size_t prev_alloc =  GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc =  GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    void *next = NEXT_BLKP(ptr);
    size_t total_size = old_size;

    if(prev_alloc && !next_alloc && (old_size + next_size >= align_size)){
        total_size += next_size;
        fill_block(next);
        PUT(HDRP(ptr), PACK(total_size, 1));
        PUT(FTRP(ptr), PACK(total_size, 1));
        place(ptr, total_size);
    }
    else if(!next_size && align_size >= old_size){
        size_t extend_size = align_size - old_size;
        if((mem_sbrk(extend_size)) == (void*)-1)
            return NULL;
        
        PUT(HDRP(ptr), PACK(total_size + extend_size, 1));
        PUT(FTRP(ptr), PACK(total_size + extend_size, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));
        place(ptr, align_size);
    }
    else{
        new_ptr = mm_malloc(align_size);
        if(new_ptr == NULL)
            return NULL;
        memcpy(new_ptr, ptr, MIN(old_size, size));
        mm_free(ptr);
        return new_ptr;
    }
    return ptr;
}
