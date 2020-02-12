#ifndef __MEMORY_H__
#define __MEMORY_H__

#define MEM_POOL_EXTENSIBLE (0x1 << 1)
#define MEM_POOL_LIMITED (0x1 << 2)


struct memory_blocks {
    volatile uint8_t *buffer;   /* block of memory or array of data */
    unsigned per_size;      /* how many bytes for each chunk */
    unsigned count;     /* number of chunks of data */
    volatile unsigned head;     /* where the writes go */
    volatile unsigned tail;     /* where the reads come from */
};

struct memory_pool {
	size_t total_size;
	size_t blk_size;
	uint32_t blk_cnt;
	uint32_t blk_used;
	uint32_t ref_count;
	uint32_t flag;
	struct memory_blocks memblk;
	uint8_t *start;
};

struct memory_pool * memory_pool_create(
	size_t total_size, size_t mem_blk_size, uint32_t flag);
void memory_pool_destroy(struct memory_pool *p);
void *memory_pool_malloc(struct memory_pool *p, size_t size);
void memory_pool_free(struct memory_pool *p, void *ptr);
void memory_pool_info(struct memory_pool *p);

#endif
