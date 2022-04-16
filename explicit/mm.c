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
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
/*pg 857 of the textbook:*/
/* Basic constants and macros */
#define WSIZE       4       /* word size (bytes) */
#define DSIZE       8       /* doubleword size (bytes) */ /*also works for the overhead of the header and footer cuz its a byte */
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(size_t *)(p))
#define PUT(p, val)  (*(size_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/*Given block ptr bp, compute the address of the */
#define GET_NEXT(bp) (*(char **)(bp))
#define GET_PREV(bp) (*(char **)(bp + DSIZE))

#define SET_NEXT(bp, val) (*(char **)(bp) = (val))
#define SET_PREV(bp, val) (*(char **)(bp + DSIZE) = (val))


static char* heap_listp;
static char* free_listp;

static void free_add(void* new){
    /*
    new.next = free_listp;
    current.prev = new.next
    new.prev = root/NULL
    assign global pointer to new
    */
    /*
    GET_NEXT = (*(char **)(bp))
    GET_PREV = (*(char **)(bp + WSIZE))
    GET_NEXT(new) = free_listp;
    GET_PREV(new) = NULL;
    GET_PREV(free_listp) = GET_NEXT(new);
    free_listp = new;
    */
    
    (*(char **)(new)) = free_listp; //set new next point to free_listp
    (*(char **)(free_listp + WSIZE)) = new; //set free_listp prev to new
    (*(char **)(new + WSIZE)) = NULL; //set new prev to NULL (make it the head)
    free_listp = new; //assign the pointer of the list to the new head.
    
    
}


static void fill_block(void* current){
    /*
    if current is head,
    free_listp = current.next
    current.next.previous = current.previous
    else
    current.previous.next = current.next
    current.next.previous = current.previous
    GET_NEXT = (*(char **)(bp))
    GET_PREV = (*(char **)(bp + WSIZE))
    */
    if((*(char **)(current + WSIZE)) == NULL){ //if current is the head
        free_listp = GET_NEXT(current);
        GET_PREV(GET_NEXT(current)) = GET_PREV(current);
//         free_listp = (*(char **)(current));
//         (*(char **)(free_listp + WSIZE)) = NULL;
    }else{
        //current.previous.next = current.next <==> NEXT(PREV) = NEXT
        //current.next.previous = current.previous <==> PREV(NEXT) = PREV
        GET_NEXT(GET_PREV(current)) = GET_NEXT(current);
        GET_PREV(GET_NEXT(current)) = GET_PREV(current);
//         (*(char **)((*(char **)(current + WSIZE)))) = (*(char **)(current));
//         (*(char **)((*(char **)(current)) + WSIZE)) = (*(char **)(current + WSIZE));
    }
    
//     if(current == NULL){
//         free_listp = GET_NEXT(current);
//         SET_PREV(free_listp, NULL);
        
//     }else{
//         void *prev = GET_PREV(current);
//         void *next = GET_NEXT(current);
//         SET_NEXT(prev, next);
//         SET_PREV(next, prev);
//     }
}


static void *find_first_fit(size_t asize){
//     for(void *bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = (*(char **)(bp))){
//         if(asize <= GET_SIZE(HDRP(bp))){
//             return bp;
//         }
//     }
    
//     return NULL;
    
    void *bp;
    for(bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT(bp)){
        if(asize <= GET_SIZE(HDRP(bp))){
            return bp;
        }
    }
    
    return NULL;
    
    
//     void *bp = free_listp;
//     while(GET_ALLOC(HDRP(bp)) != 0){
//         if(asize <= GET_SIZE(HDRP(bp))){
//             return bp;
//         }
//         bp = GET_NEXT(free_listp);
//     }
   
    
//    return NULL; /*no fit found*/
    
    
    //or
    /*
    for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_PTR(bp)) {
        if (asize <= GET_SIZE(HDRP(bp))){ //block of required size is found
            return (bp);
        }
    }
    
    return NULL;
    */
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        free_add(bp);
        return bp;
    }
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        fill_block(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
        return(bp);
    }
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        fill_block(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return(PREV_BLKP(bp));
    }
    else { /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        fill_block(NEXT_BLKP(bp));
        fill_block(PREV_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        return(PREV_BLKP(bp));
    }
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    if((csize - asize) >= (2*DSIZE)){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        fill_block(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        coalesce(bp);
    }else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        fill_block(bp);
    }
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;
    
    /*Allocate an even number of words to maintain alignment*/
    size = (words % 2) ? (words + 1) * WSIZE : words*WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
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
    if ((heap_listp = mem_sbrk(4*WSIZE)) == NULL)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp+WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + DSIZE, PACK(DSIZE, 1));
    PUT(heap_listp+WSIZE+DSIZE, PACK(0, 1));
    heap_listp += DSIZE;
    
    free_listp = heap_listp;
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
    void *bp;
    size_t asize;
    size_t extendsize;
    
    if(size == 0)
        return NULL;
    
    if(size <= DSIZE){
        asize = 2*DSIZE;
    }else{
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }
    
    /*Search the free list for a fit*/
    if((bp = find_first_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }
    
    /*No fit found*/
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
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
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
