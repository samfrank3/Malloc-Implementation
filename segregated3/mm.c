/*
 * mm.c
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * In this approach, a block is allocated by checking the segregated free list
 * for the best fit so as to reduce as much fragmentation as possible. Inside the block
 * we have a header and a footer that stores the size of the block and a bit that determines
 * if it's free or not. We decided to do a coalescing policy of every time free and/or extend_heap
 * is called, we call coalesce (not the defered coalescing policy). We did a best fit search
 * rather than the the first fit (but having a first fit with a segregated free list is basically
 * a having a best fit search)
 *
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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE       4             /* Word and header/footer size (bytes) */
#define DSIZE      (2 * WSIZE)    /* Doubleword size (bytes), also works for the overhead of the header and footer cuz its a byte */
#define CHUNKSIZE  (1<<11)      /* Initial heap size (bytes) */

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
static char *seg_p;
size_t num_buckets = 15;

/* Give an index of the list from an array based on power of 2*/
static int get_index(size_t size){
    if(size > 262144){ //size = 2^18
        return 14;
    }else if(size > 131072){//size = 2^17
        return 13;
    }else if(size > 65536){//size = 2^16
        return 12;
    }else if(size > 32768){//size = 2^15
        return 11;
    }else if(size > 16384){//size = 2^14
        return 10;
    }else if(size > 8192){//size = 2^13
        return 9;
    }else if(size > 4096){//size = 2^12
        return 8;
    }else if(size > 2048){//size = 2^11
        return 7;
    }else if(size > 1024){//size = 2^10
        return 6;
    }else if(size > 512){//size = 2^9
        return 5;
    }else if(size > 256){//size = 2^8
        return 4;
    }else if(size > 128){//size = 2^7
        return 3;
    }else if(size > 64){//size = 2^6
        return 2;
    }else if(size > 32){//size = 2^5
        return 1;
    }else{//size = 2^4
        return 0;
    }
    
}
/* Checks to see if the heap is stable*/
static int mm_check(){
    /* Checks to see if there are allocated blocks in the free list */
    for(int i = 0; i < num_buckets; i++){ //loops through free list
        void *head = seg_p + i *WSIZE;
        void *bp = (void*)SEG_GET_NEXT(head);
        while(bp != NULL){
            if(GET_ALLOC(HDRP(bp)) == 1){ //tests if the bp block is allocated
                printf("Allocated block in the free list\n");
                return 0;
            }
        }
    }
    /* Checks to see if there are better ways to coalesce, OPTIMIZATION here*/
    void *bp;
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){ //loops through heap and when size of block contains something
        size_t current = GET_ALLOC(HDRP(bp));
        size_t prev = (size_t) NULL;
        if(PREV_BLKP(bp) == NULL){ //continue when bp is head
            continue;
        }
        prev = GET_ALLOC(HDRP(bp));
        if(prev && current){ //checks to see if this and the previous blocks are free
            printf("contiguous free blocks");
            return 0;
        }
    }
    /* Checks to see if free block is in free list*/
    int found = 0; //variable to check if
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){ //loops through heaplist
        if(!GET_ALLOC(bp)){ //if the block is free
            int index = get_index(GET_SIZE(bp)); //index of free lists
            for(int i = index; i < num_buckets; i++){ //loops through segregated free lists
                void *current = seg_p + i * WSIZE;
                while(current != NULL){
                    if(current == bp){ //checks to see if the free block is actually in the segregated free lists
                        found = 1;
                    }
                }
            }
            if(found == 0){
                return 0;
            }
        }
    }
    if(found == 0){
        return 0;
    }
    /* Checls to see if free blocks in free list exist in heap*/
    for(int i = 0; i < num_buckets; i++){ //loops through segregated free lists
        void *bp = seg_p + i * WSIZE;
        while(bp != NULL){ //every free block pointer
            if(SEG_GET_NEXT(bp) != (size_t) NULL){
                if(SEG_GET_PREV(SEG_GET_NEXT(bp)) != bp){ //see if the pointers are pointing to themselves given not tail
                    return 0;
                }
            }else if(SEG_GET_PREV(bp) != (size_t) NULL){
                if(SEG_GET_NEXT(SEG_GET_PREV(bp)) != bp){ //see if the pointers are pointing to themselves given not head
                    return 0;
                }
            }
        }
    }
    
    return 1;
}

/* Add newly freed block pointer to the segregated list of appropriate size */
static void add_to_free_list(void *new)
{
    int index = get_index(GET_SIZE(HDRP(new))); // Get appropriate sized list for newly freed block to be placed in
    void *head = seg_p + index * WSIZE; //get the head of the correct segregated free bucket list
    void *next = head;
    
    /* Trys to find the best place to store the free block within the bucket, OPTIMIZATION*/
    while(SEG_GET_NEXT(next) != (size_t) NULL){ //loop through bucket list until reach tail
        next = (void *)SEG_GET_NEXT(next);
        if((size_t)next >= (size_t)new){
            void *holder = next;
            next = (void *)SEG_GET_PREV(next);
            SEG_SET_NEXT(next, (size_t) new);
            SEG_SET_PREV(new, (size_t) next);
            SEG_SET_NEXT(new, (size_t) holder);
            SEG_SET_PREV(holder, (size_t)new);
            return;
        }
    }
    /* Sets up the segregated free bucket's head*/
    SEG_SET_NEXT(next, (size_t) new);
    SEG_SET_PREV(new, (size_t) next);
    SEG_SET_NEXT(new, (size_t) NULL);
    
}

/* Remove newly filled (free) block from the segregated list of free blocks*/
static void fill_block(void *current)
{
    if(SEG_GET_NEXT(current) && SEG_GET_PREV(current)){ //if the current block is not head nor tail
        PUT(GET_NEXT(SEG_GET_PREV(current)), SEG_GET_NEXT(current));
        PUT(GET_PREV(SEG_GET_NEXT(current)), SEG_GET_PREV(current));
    }
    else if(SEG_GET_PREV(current)){ //if the current is not head
        PUT(GET_NEXT(SEG_GET_PREV(current)), (size_t) NULL);
    }
    /* Disconnect the block from the free list*/
    PUT(GET_NEXT(current), (size_t) NULL);
    PUT(GET_PREV(current), (size_t) NULL);
}

/* Coalesce Function to help reduce fragmentation for segmentation implementation*/
static void *coalesce(void * bp)
{
    size_t prev_alloc =  GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc =  GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){ //Case 1: previous and next blocks are allocated
        add_to_free_list(bp);
        return bp;
    }else if(prev_alloc && !next_alloc){ //Case 2: previous block is allocated and next block is free
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        fill_block(NEXT_BLKP(bp)); // remove 'next' free block pointer from its segregated list
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(GET_PREV(bp), (size_t) NULL);
        PUT(GET_NEXT(bp), (size_t) NULL);
    }else if(!prev_alloc && next_alloc) { //Case 3: Previous block is free and next block is allocated
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        fill_block(PREV_BLKP(bp)); //remove 'prev' free block pointer from its segregated list
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        PUT(GET_PREV(bp), (size_t) NULL);
        PUT(GET_NEXT(bp), (size_t) NULL);
    }else { //Case 4: Next and Previous blocks are free, combine
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
static void *extend_heap(size_t words)
{
    void *bp;
    /* Allocate an even number of words to maintain alignment*/
//    size_t size = (words % 2) ? (words + 1) * WSIZE : words*WSIZE; drops performance
    if((bp = mem_sbrk(words)) == (void*)-1){
        return NULL;
    }
    
    /* Initialize free block header/footer and the epilogue header*/
    PUT(HDRP(bp), PACK(words, 0));
    PUT(FTRP(bp), PACK(words, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
   
    return coalesce(bp);
}

/* Places a block of size (asize) at the start of the free block bp */
static void *place(char *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp)); //Gets the current block size

    if(!GET_ALLOC(HDRP(bp))){
        fill_block(bp);
    }
    /* If current block size is greater than the asize, splicing takes place */
    if((csize-asize) >= 2*DSIZE){
        if(asize > 64){ //played with numbers and tracefiles to see which number is most optimal
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
    else{ //no splicing, take whole block
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
    return bp;
}

/* Adopted the best fit policy for finding a free block */
static void *find_best_fit(size_t asize, int index)
{
    size_t min = (size_t) NULL;
    void *bestfit = NULL;
    while(index < num_buckets){ //loop through starting index bucket to bigger buckets
        void *head  = seg_p + index * WSIZE;
        void *bp = (void*)SEG_GET_NEXT(head);
        while(bp != NULL){ //available free block
            size_t csize = GET_SIZE(HDRP(bp));
            if(csize >= asize){ // if the current size of the bucket is greater than or equal to asize
                /* finds best fit from the list by finding some minimum difference*/
                if(min == (size_t) NULL){ //if null
                    min = csize;
                    bestfit = bp;
                }else if(csize == asize){ //if the free block size is the same as desired allocated size, return bp
                    return bp;
                }else if(csize < min){ //sees if there is a better fit
                    min = csize;
                    bestfit = bp;
                }
            }
            bp = (void*)SEG_GET_NEXT(bp); //iterate to next pointer within the segregated list
        }
        index++; //iterate to smaller bucket size if there is no best fit
    }
    return bestfit;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int heap_space = num_buckets + 3; //buckets + prologoue and epilogue space
    if((heap_listp = mem_sbrk(heap_space * WSIZE)) == (void *)-1){ //allocate space for segregated list in heap
        return -1;
    }
    /* initialize the segregated list to NULL*/
    int i;
    for(i = 0; i < num_buckets; ++i){
        PUT(heap_listp + i*WSIZE, (size_t)NULL);
    }
    /* Create the initial empty heap */
    PUT(heap_listp + (i)*WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + (i+1)*WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + (i+2)*WSIZE, PACK(0, 1));
    seg_p = heap_listp;
    heap_listp += (i+1)*WSIZE;

    if(extend_heap(1<<6) == NULL) //half the further extensionsons of the heap (11/2) rounded up to 6
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    /* Ignotre stupid calls*/
    if(size == 0)
        return NULL;
    
    size_t align_size; //Adjusted block size
    if(size > DSIZE){
        align_size = DSIZE * ((size + DSIZE + (DSIZE - 1))/DSIZE);
    }else{
        align_size = 2*DSIZE;
    }
    
    size_t extendsize;
    void *bp;
    /* Search the free list for a fit with the best fit policy*/
    if((bp = find_best_fit(align_size, get_index(align_size))) != NULL)
        return place(bp, align_size);

    /* If no fit found. Get more memory and place the block */
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
    /* Ignore stupid calls*/
    if (bp == NULL) {
        return;
    }
    /* Frees and coalesces the blcoks*/
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
    //allocates the memory if there is no pointer
    if(ptr == NULL)
        return mm_malloc(size);
    else if(size == 0){ //if the size ==0, free the block
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

    if(prev_alloc && !next_alloc && (old_size + next_size >= align_size)){
        fill_block(next);
        PUT(HDRP(ptr), PACK((old_size + next_size), 1));
        PUT(FTRP(ptr), PACK((old_size + next_size), 1));
        place(ptr, (old_size + next_size));
    }
    else if(!next_size && align_size >= old_size){
        size_t extend_size = align_size - old_size;
        if((mem_sbrk(extend_size)) == (void*)-1)
            return NULL;
        
        PUT(HDRP(ptr), PACK(old_size + extend_size, 1));
        PUT(FTRP(ptr), PACK(old_size + extend_size, 1));
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
