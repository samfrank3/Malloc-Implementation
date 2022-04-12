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
static char *heap_listp;

static void *coalesce(void *bp){
size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
size_t size = GET_SIZE(HDRP(bp));
15 
	.	16  if (prev_alloc && next_alloc) {  
	.	17  return bp;  
	.	18  }  
19 
	.	20  else if (prev_alloc && !next_alloc) {  
	.	21  size += GET_SIZE(HDRP(NEXT_BLKP(bp)));  
	.	22  PUT(HDRP(bp), PACK(size, 0));  
	.	23  PUT(FTRP(bp), PACK(size,0));  
	.	24  return(bp);  
	.	25  }  
26 
	.	27  else if (!prev_alloc && next_alloc) {  
	.	28  size += GET_SIZE(HDRP(PREV_BLKP(bp)));  
	.	29  PUT(FTRP(bp), PACK(size, 0));  
	.	30  PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));  
	.	31  return(PREV_BLKP(bp));  
	.	32  }  
33 
	.	34  else { /* Case 4 */  
	.	35  size += GET_SIZE(HDRP(PREV_BLKP(bp))) +  
	.	36  GET_SIZE(FTRP(NEXT_BLKP(bp)));  
	.	37  PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));  
	.	38  PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));  
	.	39  return(PREV_BLKP(bp));  
	.	40  }  
	.	41  }  


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
    PUT(HDRP(NEXT_BLK(bp)), PACK(0,1)); /*New epilogue header*/
    
    /*Coalesce if the previous block was free*/
    return coalesce(bp);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /*Create the intial empty heap*/
    if ((heap_listp = mem_sbrk(4*WSIZE)) == NULL)
        return -1;
    PUT(heap_listp, 0); /*Alignment Padding*/
    PUT(heap_listp+WSIZE, PACK(DSIZE, 1)); /*Prologue Header*/
    PUT(heap_listp + DSIZE, PACK(DSIZE, 1));/*Prologue Footer*/
    PUT(heap_listp+WSIZE+DSIZE, PACK(0, 1));/*Epliogue Header*/
    heap_listp += DSIZE;
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
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
    return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
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














