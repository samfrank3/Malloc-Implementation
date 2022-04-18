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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT DSIZE

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
/*pg 857 of the textbook:*/
/* Basic constants and macros */
#define WSIZE       sizeof(void *)       /* word size (bytes) */
#define DSIZE       (2*WSIZE)            /* doubleword size (bytes) */ /*also works for the overhead of the header and footer cuz its a byte */
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
/*pack creates the top and bottom of the block*/
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
/*Writing to a block*/
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
/*Read the size of a block*/
#define GET_SIZE(p)  (GET(p) & ~(DSIZE -1))
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)  ((void *)(bp) - GET_SIZE((void *)(bp) - DSIZE))

#define GET_NEXT(p)  (*(char **)(p))
#define GET_PREV(p)  (*(char **)(p + WSIZE))

static char *heap_listp;
static char  *free_listp;
// static char* free_listp1_2;
// static char* free_listp3_4;
// static char* free_listp5_8;
// static char* free_listp9_16;
// static char* free_listp17_32;
// static char* free_listp33_inf;


static void add_to_list(void* new, size_t size){
    /*if (size <= 2) {
        GET_NEXT(new) = free_listp1_2;
        GET_PREV(free_listp1_2) = new;
        GET_PREV(new) = NULL;
        free_listp1_2 = new;
    }
    else if (size <=4){
        GET_NEXT(new) = free_listp3_4;
        GET_PREV(free_listp3_4) = new;
        GET_PREV(new) = NULL;
        free_listp3_4 = new;
    }
    else if (size <=8){
        GET_NEXT(new) = free_listp5_8;
        GET_PREV(free_listp5_8) = new;
        GET_PREV(new) = NULL;
        free_listp5_8 = new;
    }
    else if (size <=16){
        GET_NEXT(new) = free_listp9_16;
        GET_PREV(free_listp9_16) = new;
        GET_PREV(new) = NULL;
        free_listp9_16 = new;
    }
    else if (size <=32){
        GET_NEXT(new) = free_listp17_32;
        GET_PREV(free_listp17_32) = new;
        GET_PREV(new) = NULL;
        free_listp17_32 = new;
    }
    else {
        GET_NEXT(new) = free_listp33_inf;
        GET_PREV(free_listp33_inf) = new;
        GET_PREV(new) = NULL;
        free_listp33_inf = new;
    }*/
}

static void fill_block(void* current,size_t size){
    /*if(GET_PREV(current)==NULL){
        
        if (size <= 2) {
            free_listp1_2 = GET_NEXT(current);
        }
        else if (size <=4){
            free_listp3_4 = GET_NEXT(current);
        }
        else if (size <=8){
            free_listp5_8 = GET_NEXT(current);
        }
        else if (size <=16){
            free_listp9_16 = GET_NEXT(current);
        }
        else if (size <=32){
            free_listp17_32 = GET_NEXT(current);
        }
        else {
            free_listp33_inf = GET_NEXT(current);
        }
    }else{
        GET_NEXT(GET_PREV(current))=GET_NEXT(current);
    }
    GET_PREV(GET_NEXT(current))=GET_PREV(current);*/
}

static void *find_first_fit(size_t asize)
{
    /*void *bp;
    if (asize <= 2) {
        for (bp = free_listp1_2; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT(bp)) {
            if (asize <= GET_SIZE(HDRP(bp)))
                return bp;
        }
    }
    else if (asize <= 4) {
        for (bp = free_listp3_4; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT(bp)) {
            if (asize <= GET_SIZE(HDRP(bp)))
                return bp;
        }
    }
    else if (asize <= 8) {
        for (bp = free_listp5_8; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT(bp)) {
            if (asize <= GET_SIZE(HDRP(bp)))
                return bp;
        }
    }
    else if (asize <= 16) {
        for (bp = free_listp9_16; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT(bp)) {
            if (asize <= GET_SIZE(HDRP(bp)))
                return bp;
        }
    }
    else if (asize <= 32) {
        for (bp = free_listp17_32; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT(bp)) {
            if (asize <= GET_SIZE(HDRP(bp)))
                return bp;
        }
    }
    else {
        for (bp = free_listp33_inf; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT(bp)) {
            if (asize <= GET_SIZE(HDRP(bp)))
                return bp;
        }
    }
    */
    return NULL;
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if (prev_alloc && next_alloc) {
        add_to_list(bp, size);
        return bp;
    }
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        fill_block(NEXT_BLKP(bp),size);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        fill_block(PREV_BLKP(bp), size);
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else { /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        fill_block(NEXT_BLKP(bp),size);
        fill_block(PREV_BLKP(bp),size);
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    add_to_list(bp, size);
    return bp;
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        fill_block(bp, asize);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        coalesce(bp);
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        fill_block(bp, asize);
    }
}

static void *extend_heap(size_t words){
    void *bp;
    size_t size;

    /*Allocate an even number of words to maintain alignment*/
    size = (words % 2) ? (words + 1) * WSIZE : words*WSIZE;
    
    if ((bp = mem_sbrk(size)) == (void *)-1)
        return NULL;

    /*Initialize free block header/footer and the epilogue header*/
    PUT(HDRP(bp), PACK(size,0)); /*Free block header*/
    PUT(FTRP(bp), PACK(size,0)); /*Free block footer*/
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); /*New epilogue header*/
    
    /*Coalesce if the previous block was free*/
    return coalesce(bp);
}


/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{

    /*Creat the seglist pointers for da heap*/
    if((free_listp = mem_sbrk(7 * WSIZE)) == NULL){
        return -1;   
    }
    /* Create the initial empty heap. */
    if ((heap_listp = mem_sbrk(8 * WSIZE)) == NULL)
        return -1;
    
    
    PUT(heap_listp, 0); /*Alignment Padding*/
    PUT(heap_listp+WSIZE, PACK(DSIZE, 1)); /*Prologue Header*/
    PUT(heap_listp + DSIZE, PACK(DSIZE, 1));/*Prologue Footer*/
    PUT(heap_listp+WSIZE+DSIZE, PACK(0, 1));/*Epliogue Header*/
    heap_listp += DSIZE;
    
    /*
    Seg List:
    0-2
    2-4
    4-8
    8-16
    16-32
    32-64
    64-inf
    */
    
    PUT(free_listp + 0, NULL);
    PUT(free_listp + 8, NULL);
    PUT(free_listp + 16, NULL);
    PUT(free_listp + 32, NULL);
    PUT(free_listp + 40, NULL);
    PUT(free_listp + 48, NULL);
    PUT(free_listp + 56, NULL);
    
//     free_listp1_2 = heap_listp;
//     free_listp3_4 = heap_listp;
//     free_listp5_8 = heap_listp;
//     free_listp9_16 = heap_listp;
//     free_listp17_32 = heap_listp;
//     free_listp33_inf = heap_listp;

    /* Extend the empty heap with a free block */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    void *bp;

    /* Ignore stupid calls. */
    if (size == 0)
        return NULL;

    if (size <= DSIZE){
        asize = 2 * DSIZE;
    }else{
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    }

    /* Search the free list for a fit*/
    if ((bp = find_first_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found*/
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
    
    //     ORIGINAL MALLOC GIVEN WORKS
    //     int newsize = ALIGN(size + SIZE_T_SIZE);
    //     void *p = mem_sbrk(newsize);
    //     if (p == (void *)-1)
    //     return NULL;
    //     else {
    //         *(size_t *)p = size;
    //         return (void *)((char *)p + SIZE_T_SIZE);
    //     }
    //     END OF ORIGINAL MALLOC GIVEN
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL)
        return;
    
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
//    void *oldptr = ptr;
//    void *newptr;
//    size_t copySize;
//
//    newptr = mm_malloc(size);
//    if (newptr == NULL)
//      return NULL;
//    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
//    if (size < copySize)
//      copySize = size;
//    memcpy(newptr, oldptr, copySize);
//    mm_free(oldptr);
//    return newptr;
    
    size_t oldsize,newsize;
    void *newptr;
    if((int)size < 0)
        return NULL;
    if (size == 0) {
        mm_free(ptr);
        return (NULL);
    }
    if (ptr == NULL)
        return (mm_malloc(size));

    oldsize=GET_SIZE(HDRP(ptr));
    newsize = size + (2 * WSIZE);

    if (newsize <= oldsize){
        return ptr;
    }
    else{
        size_t if_next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
        size_t next_blk_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        size_t total_free_size = oldsize + next_blk_size;

        if(!if_next_alloc && total_free_size>= newsize){
            fill_block(NEXT_BLKP(ptr),size);
            PUT(HDRP(ptr),PACK(total_free_size,1));
            PUT(FTRP(ptr),PACK(total_free_size,1));
            return ptr;
        }
        
        else{
            newptr=mm_malloc(newsize);
                
            if (newptr == NULL)
                return (NULL);
            place(newptr,newsize);
            memcpy(newptr,ptr,oldsize);
            mm_free(ptr);
            return newptr;
        }
    }
}
