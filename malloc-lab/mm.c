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
#define CHUNKSIZE (1<<12) //기본 페이지 단위가 4kb라서 2^12

//최소 블록 크기 보장
#define MAX(x,y) ((x) > (y)? (x) : (y)) //요청 크기가 너무 작다면 최소 블록 크기로 할당함

//하나의 헤더/푸터 = 블록 크기 (size) + 할당여부 (alloc)
#define PACK(size, alloc) ((size) | (alloc))

//주소 p에서 4바이트 워드를 읽고 씀
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

//헤더/푸터에 저장된 정보에서 블록의 크기, 할당 여부를 분리해서 읽어오기
#define GET_SIZE(p) (GET(p) & ~0x7) //하위 3비트 제외한 블록의 크기만 가지고 옴
#define GET_ALLOC(p) (GET(p) & 0x1) //블록의 할당 여부만 가지고 옴->제일 마지막 비트

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

static char *heap_listp;

int mm_init(void)
{
    //16바이트로 초기 힙 구조 생성 
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) //실패시 -1, 성공하면 old_brk(이전 주소의 brk)을 반환 
        return -1;

    PUT(heap_listp, 0); //1.정렬 패딩
    PUT(heap_listp + (1*WSIZE), PACK (DSIZE, 1)); //2.프롤로그 헤더, 8바이트인 이유는 전체블록(16)-payload(8)
    PUT(heap_listp + (2*WSIZE), PACK (DSIZE, 1)); //3.프롤로그 푸터. 프롤로그 1인 이유는 할당되어 있다고 가정해야 실제 값을 알 수 있기때문
    PUT(heap_listp + (3*WSIZE), PACK (0, 1)); //4.에필로그 헤더
    heap_listp += (2*WSIZE);

    //초기 가용 블록으로 힙 확장
    if (extend_heap (CHUNKSIZE/WSIZE) == NULL) 
        return -1;
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    //%2가 1이면 홀수 -> 참 ->words + 1
    //%2가 0이라면 짝수 -> 거짓 -> words * wsize
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; 

    if ((long)(bp = mem_sbrk(size)) == -1) //힙을 size만큼 늘리기
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); //헤더에 (size | 0) 정보 쓰기
    PUT(FTRP(bp), PACK(size, 0)); //푸터에 (size | 0) 정보 쓰기
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //다음 블록의 헤더 = 에필로그 블록.

    //이전 블록이 free라면 병합
    //프롤로그 블록 덕분에 맨 앞 블록이어도 동작
    return coalesce(bp);
}

//단편화 방지하기 위해 free 블록들끼리 병합
static void *coalesce(void *bp)
{
    //앞,뒤 블록의 할당 상태 확인
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    size_t size = GET_SIZE(HDRP(bp)); //현재 블록 크기 확인. size = 현재블록의 size

    //Case 1 : 둘다 할당
    if (prev_alloc && next_alloc){
        return bp;
    }

    //Case 2 : 이전 블록 alloc 1, 다음블록 free 0
    //현재 블록 + 다음 블록 병합
    else if (prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); //다음 블록 크기만큼 증가
        PUT(HDRP(bp), PACK(size, 0)); //헤더 업데이트 : 현재 위치
        PUT(FTRP(bp), PACK(size, 0)); //푸터 업데이트 : 뒤 블록의 푸터 위치
}

    //Case 3 : 이전 블록 free 0, 다음 블록 alloc 1
    //이전 블록 + 현재 블록 병합
    else if (!prev_alloc && next_alloc){
        size += GET_SIZE(FTRP(PREV_BLKP(bp))); //이전 블록 크기만큼 증가
        PUT(FTRP(bp), PACK(size, 0)); //푸터 업데이트 : 현재 위치
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); //헤더 업데이트 : 이전 블록 시작
        bp = PREV_BLKP(bp); //bp를 이전 블록으로 이동
    }
    //Case 4 : 이전 블록 free 0, 다음 블록 free 0
    else{
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp))); //이전블록 + 다음 블록 크기만큼 증가
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); //헤더 업데이트
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); //푸터 업데이트
        bp = PREV_BLKP(bp); //bp 변경
    }
    return bp;
}

void *mm_malloc(size_t size)
{
    size_t asize; //실제로 할당할 블록 크기(payload+header/footer 포함)
    size_t extendsize; //힙 확장할 크기
    char *bp; 

    //예외처리
    if (size == 0)
        return NULL;
        
    if (size <= DSIZE) //최소 블록 크기 보장(header+footer+최소payload)
        asize = 2*DSIZE;
    else
        //DSIZE 초과면 8의 배수로 올림
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE); //(DSIZE - 1)을 해주는 이유 : 쓸데없는 메모리 낭비를 줄이기 위해

    //가용 리스트에서 asize를 만족하는 free블록 찾기
    //찾았으면 place()로 블록 할당
    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    //찾지 못했으면 heap확장
    extendsize = MAX(asize, CHUNKSIZE);

    //한번에 최소 CHUNKSIZE 이상 확장. extend는 무조건 실행됨
    if ((bp = extend_heap (extendsize/WSIZE)) == NULL)
        return NULL;

    //확장한 블록에 배정
    place(bp, asize);
    return bp;
}

//헤더와 푸터만 변경하고 내용을 변경하지는 않음
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr)); //free할 현재 블록의 크기를 가져옴

    PUT(HDRP(ptr), PACK(size, 0)); //header free
    PUT(FTRP(ptr), PACK(size, 0)); //footer free
    coalesce(ptr); //병합
}

static void *find_fit(size_t asize)
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