
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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};
static void * segregated_free_list[16];
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define MAX_SIZE_LIST 16
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<9) //Extend head by this amount
#define MAX(x,y) ((x)>(y)?(x):(y))
#define PACK(size,alloc) ((size)|alloc) //Pack a size and alloced bit
#define GET(p) (*(unsigned int *)(p)) //Read a word
#define PUT(p,val) (*(unsigned int *)(p)=(val))//Write a word
#define GET_SIZE(p) (GET(p)&~0x7)
#define GET_ALLOC(p) (GET(p)&0x1)
/*Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)
/*Given block ptr bp, compute address of next and previos blocks*/
#define NEXT_BLKP(bp) ((char *)(bp)+GET_SIZE(((char*)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char*)(bp)-DSIZE)))
/*Given free block ptr fbp, compute ptr of its pred and succ*/
#define PRED_P(fbp) ((char *)(fbp))
#define SUCC_P(fbp) ((char *)(fbp+WSIZE))
/*Given free block fbp, compute add of its pred and succ???*/
#define PRED_BLKP(fbp) (*(char **)(fbp))
#define SUCC_BLKP(fbp) (*(char **)(fbp+WSIZE))
// test (char *) *(fbp)
#define PTR(p,ptr) (*(unsigned int *)(p)=(unsigned int)(ptr))
static void *coalesce(void *bp);
static void * head_listsp;
//helper function

static void insert_node(void *fbp, size_t size){
    int lineindex=0;
    void *target_ptr=NULL;
    void *pred_of_target_ptr=NULL;
    // int old_size = size;
    while((lineindex<(MAX_SIZE_LIST-1))&&(size>1)){
        size = size >> 1;
        lineindex+=1;
    }
    // printf("insert node %d, %d \n", old_size, lineindex);

    target_ptr=segregated_free_list[lineindex];
    while ((target_ptr!=NULL)&&(size>GET_SIZE(HDRP(target_ptr))))
    {
        pred_of_target_ptr = target_ptr;
        target_ptr=SUCC_BLKP(target_ptr);//???
    }
    if(target_ptr!=NULL){
        if(pred_of_target_ptr!=NULL){
            PTR(PRED_P(fbp),pred_of_target_ptr);
            PTR(SUCC_P(pred_of_target_ptr),fbp);
            PTR(SUCC_P(fbp),target_ptr);
            PTR(PRED_P(target_ptr),(char *)fbp);
        }else
        {
            PTR(SUCC_P(fbp),target_ptr);
            PTR(PRED_P(target_ptr),fbp);
            PTR(PRED_P(fbp),NULL);
            segregated_free_list[lineindex]=fbp;
        }
        
    }
    else{
        // target_ptr == NULL. 节点是结尾或者链表为空
        if(pred_of_target_ptr != NULL){
            PTR(SUCC_P(pred_of_target_ptr),fbp);
            PTR(PRED_P(fbp),pred_of_target_ptr);
            PTR(SUCC_P(fbp),NULL);
            
        }else{
           PTR(SUCC_P(fbp),NULL); 
           PTR(PRED_P(fbp),NULL); 
           segregated_free_list[lineindex]=fbp;
        }
    }
    // node inserted
}

static void *extend_heap(size_t size){
    char *fbp;
    size_t asize;
    asize = ALIGN(size);
    if((long)(fbp=mem_sbrk(asize))==-1){
        return NULL;
    }
    //Initialize free block header/footer
    PUT(HDRP(fbp),PACK(asize,0));
    PUT(FTRP(fbp),PACK(asize,0));
    PUT((HDRP(NEXT_BLKP(fbp))),PACK(0,1)); //ending block
    // PUT((FTRP(NEXT_BLKP(fbp))),PACK(0,1));
    insert_node(fbp,asize);
    // printf("heap extended %p, %p, %d , end in %p \n", fbp,FTRP(fbp), asize,(HDRP(NEXT_BLKP(fbp)))+3);
    return coalesce(fbp);
}



static void delete_node(void *fbp){
    if (fbp==NULL) return;
    int lineindex=0;
    size_t size=GET_SIZE(HDRP(fbp));
    while((lineindex<(MAX_SIZE_LIST-1))&&(size>1)){
        size = size >> 1;
        lineindex+=1;
    }
    if(SUCC_BLKP(fbp)!=NULL){
        // not the last
        if(PRED_BLKP(fbp)!=NULL){
            PTR(SUCC_P((PRED_BLKP(fbp))),SUCC_BLKP(fbp));
            PTR(PRED_P((SUCC_BLKP(fbp))),PRED_BLKP(fbp));
        }else{
            segregated_free_list[lineindex] = SUCC_BLKP(fbp);
            PTR(PRED_P((SUCC_BLKP(fbp))),NULL);
        }
    }else{
        if(PRED_BLKP(fbp)!=NULL){
            // the last
            PTR(SUCC_P((PRED_BLKP(fbp))),NULL);
        }else{
            // will be empty
            segregated_free_list[lineindex] = NULL;
        }
    }

}

static void *coalesce(void *bp){
    size_t prev_alloc=GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){
        return bp;
    }
    else if(prev_alloc && !next_alloc){
        delete_node(bp);
        delete_node(NEXT_BLKP(bp));
        size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp),PACK(size,0));
        PUT(FTRP(bp),PACK(size,0));
    }else if(!prev_alloc && next_alloc){
        size+=GET_SIZE(HDRP(PREV_BLKP(bp)));
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp),PACK(size,0));
        PUT(FTRP(bp),PACK(size,0));
    }else if(!prev_alloc && !next_alloc){
        size+=GET_SIZE(HDRP(PREV_BLKP(bp)));
        size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp),PACK(size,0));
        PUT(FTRP(bp),PACK(size,0));
    }
    insert_node(bp,size);
    return bp;
}

static void *place(void *fbp, size_t size){
    size_t block_size = GET_SIZE(HDRP(fbp));
    size_t left_size = block_size-size;
    delete_node(fbp);
    if(left_size<2*DSIZE){
        // 如果剩余块小于最小块，就不分离原块
        PUT(HDRP(fbp),PACK(block_size,1));
        PUT(FTRP(fbp),PACK(block_size,1));
        // printf("malloc %p %p %d \n",fbp,FTRP(fbp)-1,GET_SIZE(FTRP(fbp)));
        return fbp;
    }else{
        // 分离原块，且小块靠左，大块靠右，思路参考李秋豪：https://www.cnblogs.com/liqiuhao/p/8252373.html
        if (size >= 96){
            PUT(HDRP(fbp),PACK(left_size,0));
            PUT(FTRP(fbp),PACK(left_size,0));
            PUT(HDRP(NEXT_BLKP(fbp)),PACK(size,1));
            PUT(FTRP(NEXT_BLKP(fbp)),PACK(size,1));
            // char * left = NEXT_BLKP(fbp);
            // printf("block size %d ",block_size);
            // printf("malloc %p %p %d ",left,FTRP(left)-1,GET_SIZE(FTRP(left)));
            // printf("left %p %p %d \n",fbp,FTRP(fbp)-1,GET_SIZE(FTRP(fbp)));
            insert_node(fbp,left_size);

            
            return NEXT_BLKP(fbp);
        }else{
            PUT(HDRP(fbp),PACK(size,1));
            PUT(FTRP(fbp),PACK(size,1));
            PUT(HDRP(NEXT_BLKP(fbp)),PACK(left_size,0));
            PUT(FTRP(NEXT_BLKP(fbp)),PACK(left_size,0));
            insert_node(NEXT_BLKP(fbp),left_size);
            

            return fbp;
        }
    }

}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int i;
    for(i=0;i<MAX_SIZE_LIST;i++){
        segregated_free_list[i] = NULL;
    }
    if ((head_listsp = mem_sbrk(4 * WSIZE))==(void *)-1)
        return -1;
    PUT(head_listsp,0);
    PUT(head_listsp+(1*WSIZE),PACK(DSIZE,1));
    PUT(head_listsp+(2*WSIZE),PACK(DSIZE,1));
    PUT(head_listsp+(3*WSIZE),PACK(0,1));
    head_listsp += (2*WSIZE);
    if(extend_heap(CHUNKSIZE)==NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (size==0)
        return NULL;
    int asize = ALIGN(size + DSIZE);//TODO
    int lineindex = 0;
    size_t target_size = asize;
    void *fbp =NULL;
    while (lineindex < MAX_SIZE_LIST)
    {
        if((target_size<=1)&&(segregated_free_list[lineindex]!=NULL)){
            fbp = (void *)segregated_free_list[lineindex];
            while ((fbp!=NULL)&&(asize>(GET_SIZE(HDRP(fbp)))))
            {
                fbp=SUCC_BLKP(fbp);  
            }
            if(fbp!=NULL){
                break;
            }
        }
        target_size>>=1;
        lineindex+=1;
    }
    if(fbp == NULL){
        if((fbp=extend_heap(MAX(asize,CHUNKSIZE)))==NULL)
            return NULL;
    }
    // printf("malloc %p \n",fbp);
    fbp=place(fbp,asize);
    // printf("malloc %p %p %d\n",fbp,FTRP(fbp)-1,GET_SIZE(FTRP(fbp)));
    return fbp;
    
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE((HDRP(ptr)));

    PUT((HDRP(ptr)),PACK(size,0));
    PUT((FTRP(ptr)),PACK(size,0));

    insert_node(ptr,size);
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */

void *mm_realloc(void *ptr, size_t size)
{
    // printf("realloc %p %d\n",ptr,size);

    void *new_block = ptr;
    // int remainder=0;
    if (size == 0){
        // mm_free(ptr);
        return NULL;
    }
    int require_size = size;
    int asize = ALIGN(size + DSIZE);//TODO
    int old_size = GET_SIZE(HDRP(ptr));
    //如果size小于原本的大小，就直接返回
    if(asize <= GET_SIZE(HDRP(ptr))){
        // printf("no need realloc %p %p %d %d\n",ptr,FTRP(ptr)-1,size,GET_SIZE(HDRP(ptr)));
        return ptr;
    }
    
    
    // printf("realloc %p %p %d\n",ptr,FTRP(ptr)-1,GET_SIZE(FTRP(ptr)));

    //检查地址的下一个块是否为free块，或者是堆的结束块，要尽可能利用相邻的块，以减少外部碎片
    if(!(GET_ALLOC(HDRP(NEXT_BLKP(ptr)))) || !(GET_SIZE(HDRP(NEXT_BLKP(ptr))))){
        // next is free or next is the end
        int avil_size = ((GET_SIZE(HDRP(NEXT_BLKP(ptr)))) + GET_SIZE(HDRP(ptr)));
        int add_size = asize - avil_size;
        if(add_size>0){
            if (extend_heap(MAX(add_size,CHUNKSIZE))==NULL)
                return NULL;
            add_size = MAX(add_size,CHUNKSIZE);
        }
        delete_node(NEXT_BLKP(ptr));
        PUT(HDRP(ptr),PACK(avil_size+add_size,1));
        PUT(FTRP(ptr),PACK(avil_size+add_size,1));
    }else
    {
        // 需要删除ptr，再把内容复制到新申请的blk中
        new_block = mm_malloc(size);
        memcpy(new_block,ptr,old_size);
        mm_free(ptr);
    }
    
    // printf("realloc %p %p %d\n",ptr,FTRP(ptr)-1,GET_SIZE(FTRP(ptr)));
    return new_block;
}














