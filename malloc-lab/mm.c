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

//상수/매크로 정의
#define WSIZE 4 
#define DSIZE 8
#define CHUNKSIZE (1<<12)

//최소 블록 크기 보장
#define MAX(x,y) ((x) > (y)? (x) : (y)) //요청 크기가 너무 작다면 최소 블록 크기로 할당함

//하나의 헤더/푸터 = 블록 크기 (size) + 할당여부 (alloc)
#define PACK(size, alloc) ((size) | (alloc))

//주소 p에서 4바이트 워드를 읽고 씀
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

//헤더/푸터에 저장된 정보에서 블록의 크기, 할당 여부를 분리해서 읽어오기
#define GET_SIZE(p) (GET(p) & ~0x7) //블록의 크기만 가지고 옴
#define GET_ALLOC(p) (GET(p) & 0x1) //블록의 할당 여부만 가지고 옴

//헤더와 푸터의 위치를 가리키는 포인터
#define HDRP(bp) ((char *)(bp) - WSIZE) //헤더는 푸터보다 4바이트 앞
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

//다음 블록의 시작주소, 이전 블록의 시작 주소 계산
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //bp에서 현재 블록 크기만큼 더함
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //bp에서 이전 블록의 크기만큼 뺌

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "고민지",
    /* First member's email address */
    "kmj6386@gmail.com",
    /* Second member's full name (leave blank if none) */
    "이지윤",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
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
    else
    {
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