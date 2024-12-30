#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>  // for size_t

#include "mm.h"
#include "memlib.h"

team_t team = {
    "20201614",
    "Minseok Lee",
    "mslee1063@gmail.com",
};

#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define MAX(x, y) ((x) > (y)? (x) : (y))
#define MIN(x, y) ((x) < (y)? (x) : (y))

#define PACK(size, alloc)  ((size) | (alloc))

#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

#define NEXT_PTR(ptr) (*(char **)(ptr))
#define PREV_PTR(ptr) (*(char **)((char *)(ptr) + WSIZE))


static void *extend_heap(size_t words);
static void insert_node(void* ptr, size_t size);
static void delete_node(void *ptr);
static void *coalesce(void *bp);
static void *place(void *bp, size_t asize);
// static void mm_checker(void);

void *free_listp = 0; // free list

int mm_init(void)
{
    char *init_listp;

    free_listp = NULL;

    if ((init_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    PUT(init_listp, 0);                          //Align
    PUT(init_listp + (1*WSIZE), PACK(DSIZE, 1));  //Prologue header
    PUT(init_listp + (2*WSIZE), PACK(DSIZE, 1));  //Prologue footer
    PUT(init_listp + (3*WSIZE), PACK(0, 1));      //Epilogue header
    
    if (extend_heap(64/WSIZE) == NULL)    // WSIZE 나눌까말까
        return -1;
    return 0;
}

void *mm_malloc(size_t size)
{
    size_t asize = ALIGN(size + SIZE_T_SIZE);
    size_t extendsize;
    void *bp = NULL;
    
    if (size == 0)
        return NULL;
    
    if (size < DSIZE)
        asize = 2*DSIZE;
    else
        asize = ALIGN(size + DSIZE);
    
    for (bp = free_listp; bp != NULL && (asize > GET_SIZE(HDRP(bp))); bp = NEXT_PTR(bp));

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp == NULL) && ((bp = extend_heap(extendsize/WSIZE)) == NULL))
        return NULL;
    else bp = place(bp,asize);
    
    // mm_checker();

    return bp;
}

void mm_free(void *bp)
{
    // size_t size = GET_SIZE(HDRP(bp));
    
    if(bp == 0) return;
    
    PUT(HDRP(bp), PACK(GET_SIZE(HDRP(bp)), 0));
    PUT(FTRP(bp), PACK(GET_SIZE(HDRP(bp)), 0));
    
    insert_node(bp, GET_SIZE(HDRP(bp)));
    coalesce(bp);

    return;
}

void *mm_realloc(void *bp, size_t size)
{
    size_t old_size = GET_SIZE(HDRP(bp));
    size_t new_size = ALIGN(size + SIZE_T_SIZE);
    void *next_bp = NEXT_BLKP(bp);
    size_t next_alloc = GET_ALLOC(HDRP(next_bp));
    size_t csize;

    if(size == 0){
        mm_free(bp);
        return NULL;
    }

    if(bp == NULL){
        return mm_malloc(size);
    }
    
    if (new_size <= old_size) {
        return bp;
    }
    
    if (!next_alloc && ((csize = old_size + GET_SIZE(HDRP(next_bp)))) >= new_size) {
        delete_node(next_bp);
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        return bp;
    }

    void *new_bp = mm_malloc(size);
    if (new_bp == NULL) {
        return NULL;
    }

    memcpy(new_bp, bp, old_size - SIZE_T_SIZE);
    mm_free(bp);
    return new_bp;
}

static void *extend_heap(size_t words)
{
    void *bp;
    size_t size = words * WSIZE;
    void *new_bp;
    // void *bp = mem_sbrk(size);
    if ((bp = mem_sbrk(size)) == (void *)-1)
        return NULL;
    else{
        PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
        PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
        PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
        
        insert_node(bp, size);
        new_bp = coalesce(bp);
        return new_bp;
    }
    /* Initialize free block header/footer and the epilogue header */
}

static void insert_node(void* ptr, size_t size)
{
    SET_PTR(ptr, free_listp); // next_ptr(ptr) = free_listp
    
    if (free_listp != NULL) {
        SET_PTR((char *)free_listp + WSIZE, ptr); // prev_ptr(free_listp) = ptr
    }
    
    SET_PTR((char *)ptr + WSIZE, NULL); // prev_ptr(ptr) = NULL
    free_listp = ptr;
}

static void delete_node(void* ptr)
{
     if (PREV_PTR(ptr) != NULL) { // If prev_ptr(ptr) is not NULL, set next_ptr(prev_ptr(ptr)) to next_ptr(ptr)
        SET_PTR(PREV_PTR(ptr), NEXT_PTR(ptr));
    } else { // If prev_ptr(ptr) is NULL, update the head of the free list
        free_listp = NEXT_PTR(ptr);
    }
    if (NEXT_PTR(ptr) != NULL) { // If next_ptr(ptr) is not NULL, set prev_ptr(next_ptr(ptr)) to prev_ptr(ptr)
        SET_PTR((char *)NEXT_PTR(ptr) + WSIZE, PREV_PTR(ptr));
    }
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if (prev_alloc && next_alloc) { // both alloc
        return bp;
    } 
    else if (prev_alloc && !next_alloc) { // prev alloc next free
        delete_node(bp);
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) { // prev free next alloc
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else { // both free
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    insert_node(bp, size);
    return bp;
}

static void *place(void *bp, size_t asize)
{
    size_t bp_size = GET_SIZE(HDRP(bp));
    // size_t remainder = bp_size - asize;

    delete_node(bp);

    if ((bp_size - asize) >= 2 * DSIZE) {  // Split block only if the remainder is large enough
        if (asize >= 80) {
            PUT(HDRP(bp), PACK(bp_size - asize, 0));
            PUT(FTRP(bp), PACK(bp_size - asize, 0));
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            insert_node(PREV_BLKP(bp), bp_size - asize);
        } else {
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            void *next_bp = NEXT_BLKP(bp);
            PUT(HDRP(next_bp), PACK(bp_size - asize, 0));
            PUT(FTRP(next_bp), PACK(bp_size - asize, 0));
            insert_node(next_bp, bp_size - asize);
        }
    } else {
        PUT(HDRP(bp), PACK(bp_size, 1));
        PUT(FTRP(bp), PACK(bp_size, 1));
    }
    return bp;
}
