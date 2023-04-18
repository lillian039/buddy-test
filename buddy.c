#include "buddy.h"
#include <stdlib.h>
#include <stdio.h>

#define NULL ((void *)0)

#define SIZE_OF_4K 4096
struct PageBlock{
    int number;
    int order;
    void  *ptr;
    struct PageBlock *nxt;
};

struct PageBlock **free_area,**alloc_area;
int max_rank = 1, **page_bitmap, *page_sum, *page_rank;
void *start_ptr;

void init_new(struct PageBlock *self, int number,int order, void *ptr, struct PageBlock *nxt){
    self->number = number;
    self->nxt = nxt;
    self->order = order;
    self->ptr = ptr;
}
void insert_page(struct PageBlock *new_insert){
    new_insert->nxt  = free_area[new_insert->order];
    free_area[new_insert->order] = new_insert;
    page_sum[new_insert->order]++;
    page_bitmap[new_insert->order][(new_insert->number >> new_insert->order)] ^= 1;
    page_rank[(new_insert->ptr - start_ptr)/SIZE_OF_4K] = new_insert->order;
}

void erase_page(struct PageBlock *delete_block){
    if(free_area[delete_block->order] == delete_block)free_area[delete_block->order] = delete_block->nxt;
    else{
        struct PageBlock *prev = free_area[delete_block->order];
        while(prev->nxt != delete_block)prev = prev->nxt;
        prev->nxt = delete_block->nxt;
    }
    page_rank[(delete_block->ptr - start_ptr)/SIZE_OF_4K] = -1;
    page_sum[delete_block->order]--;
    page_bitmap[delete_block->order][(delete_block->number >> delete_block->order)] ^= 1;
}

int init_page(void *p, int pgcount){
    int tmp_cout = pgcount;
    while(tmp_cout>>=1)max_rank++;
    start_ptr = p;

    page_bitmap = (int **)malloc(sizeof(int*)*(max_rank + 1));
    for(int i = 0; i <= max_rank; i++){
        page_bitmap[i] = (int*)malloc(sizeof(int)*pgcount);
        for(int j = 0; j < pgcount; j++)page_bitmap[i][j] = 0;
    }

    page_sum = (int *)malloc(sizeof(int)*(max_rank + 1));
    for(int i = 0; i <= max_rank; i++)page_sum[i] = 0;

    page_rank = (int *)malloc(sizeof(int)*pgcount);
    for(int i = 0; i < pgcount; i++)page_rank[i] = -1;

    free_area = (struct PageBlock **)malloc(sizeof(struct PageBlock*)*(max_rank + 1));
    for(int i = 0; i <= max_rank; i++)free_area[i] = NULL;

    alloc_area = (struct PageBlock**)malloc(sizeof(struct PageBlock*)*pgcount);
    for(int i = 0; i < pgcount; i++)alloc_area[i] = NULL;

    struct PageBlock *maxn_block = (struct PageBlock *)malloc(sizeof(struct PageBlock));
    init_new(maxn_block,0,max_rank,p,NULL);
    insert_page(maxn_block);

    return OK;
}
void *alloc_pages(int rank){
    if(rank > max_rank)return -EINVAL;
    int rank_available = rank;
    while(free_area[rank_available] == NULL){
        rank_available ++;
        if(rank_available > max_rank)return -ENOSPC;
    }
    struct PageBlock *retVal = free_area[rank_available];
    erase_page(retVal);

    while(rank_available != rank){
        struct PageBlock *newLs =  (struct PageBlock *)malloc(sizeof(struct PageBlock));
        struct PageBlock *newRs =  (struct PageBlock *)malloc(sizeof(struct PageBlock));

        rank_available --;

        init_new(newLs,retVal->number,rank_available,retVal->ptr,NULL);
        init_new(newRs,retVal->number + (1 << rank_available - 1), rank_available,retVal->ptr + (1 << rank_available - 1) * SIZE_OF_4K,NULL);        
        insert_page(newRs);
        free(retVal);
        retVal = newLs;
    }

    alloc_area[(retVal->ptr - start_ptr)/SIZE_OF_4K] = retVal;
    page_rank[(retVal->ptr - start_ptr)/SIZE_OF_4K] = retVal->order;
    return retVal->ptr;
}

int return_pages(void *p){
    if(p < start_ptr || p > start_ptr + (1 << max_rank-1)*SIZE_OF_4K)return -EINVAL;
    struct PageBlock *find_ret = alloc_area[(p-start_ptr)/SIZE_OF_4K];
    if(find_ret == NULL)return -EINVAL;
    alloc_area[(p-start_ptr)/SIZE_OF_4K] = NULL;

    while(page_bitmap[find_ret->order][(find_ret->number >> find_ret->order)] == 1){//merge
        struct PageBlock *buddy = free_area[find_ret->order];

        while((buddy->number >> find_ret->order) != (find_ret->number >> find_ret->order))buddy = buddy->nxt;
        erase_page(buddy);

        struct PageBlock *mergeBlock =  (struct PageBlock *)malloc(sizeof(struct PageBlock));
        if(buddy->number < find_ret->number){
            mergeBlock->number = buddy->number;
            mergeBlock->ptr = buddy->ptr;
        }
        else {
            mergeBlock->number = find_ret->number;
            mergeBlock->ptr = find_ret->ptr;
        }
        mergeBlock->order = find_ret->order + 1;
        free(find_ret);
        free(buddy);
        find_ret = mergeBlock;
    }
    insert_page(find_ret);
    return OK;
}

int query_ranks(void *p){
    if(p < start_ptr || p > start_ptr + (1 << max_rank-1)*SIZE_OF_4K)return -EINVAL;
    int index = (p-start_ptr)/SIZE_OF_4K;
    if(page_rank[index] == -1)return -EINVAL;
    return page_rank[index];
}

int query_page_counts(int rank){
    if(rank > max_rank)return -EINVAL;
    else if(rank < 1)return -EINVAL;
    return page_sum[rank];
}