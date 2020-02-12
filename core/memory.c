#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"

struct memblk_desc {
	union {
		uint32_t blk_cnt;
		uint8_t data[16];
	};
};

static unsigned memory_block_count(
    struct memory_blocks const *m)
{
    unsigned head, tail;        /* used to avoid volatile decision */

    if (m) {
        head = m->head;
        tail = m->tail;
        return head - tail;
    }

    return 0;
}

static bool memory_block_empty(
	struct memory_blocks const *m)
{
	return (m ? (memory_block_count(m) == 0) : true);
}

static uint8_t *memory_block_get(
	struct memory_blocks *m, int cnt)
{
	volatile uint8_t *entry = NULL;
	
	if (!memory_block_empty(m)) {
		entry = m->buffer;
		entry += ((m->tail % m->count) * m->per_size);
		m->tail += cnt;
	}
	
	return (uint8_t *)entry;
}

static void memory_block_put(
	struct memory_blocks *m, int cnt)
{
	if (m)
		m->head += cnt;
}

static void memory_block_init(
	struct memory_blocks *m, volatile uint8_t *start,
	unsigned per_size, unsigned cnt)
{
	if (m) {
		m->head = 0;
		m->tail = 0;
		m->buffer = start;
		m->per_size = per_size;
		m->count = cnt;
		memory_block_put(m, cnt);
	}
}

bool mem_pool_extensible(uint32_t flag)
{
	return flag & MEM_POOL_EXTENSIBLE;
}

void mem_pool_set_extensible(uint32_t *flag)
{
	*flag |= MEM_POOL_EXTENSIBLE;
}

bool mem_pool_limited(uint32_t flag)
{
	return flag & MEM_POOL_LIMITED;
}

void mem_pool_set_limited(uint32_t *flag)
{
	*flag |= MEM_POOL_LIMITED;
}

static size_t align(size_t size, size_t align)
{
	size_t _size = size;
	
	if (size <= align)
		return align;
	
	if (size % align)
		_size = size + (align - (size % align));	

	return _size;
}

struct memory_pool * memory_pool_create(
	size_t total_size, size_t mem_blk_size, uint32_t flag)
{
	struct memory_pool *p = NULL;
	int _align = sizeof(struct memblk_desc);

	p = (struct memory_pool *)malloc(sizeof *p);
	if (!p) {
		perror("Error while allocating memory for memory pool\n");
		return NULL;
	}
	memset(p, 0x0, sizeof *p);

	p->blk_size = align(mem_blk_size, _align);
	p->total_size = align(total_size, mem_blk_size);
	p->blk_cnt = p->total_size / p->blk_size;
	p->ref_count = 0;
	p->flag = flag;
	
	p->start = malloc(p->total_size);
	if (!p->start) {
		printf("Error while allocating memory for memory pool start\n");
		free(p);
		return NULL;
	}
	memset(p->start, 0x0, p->total_size);
	memory_block_init(&p->memblk, p->start, p->blk_size, p->blk_cnt);

	return p;
}

void memory_pool_destroy(struct memory_pool *p)
{
	if (!p)
		return;
	if (p->ref_count)
		printf("Warning:memory pool in use, force exit\n");
	if (p->start)
		free(p->start);
	free(p);
}

void *memory_pool_malloc(struct memory_pool *p, size_t size)
{
	size_t real_size = 0;
	uint32_t cnt = 0;
	uint8_t *pref;
	uint32_t extra = 1;

	if (!p) {
		errno = EINVAL;
		return NULL;
	}

	if ((size % p->blk_size) > sizeof(struct memblk_desc))
		extra = 0;

	real_size = align(size, p->blk_size);
	cnt = real_size / p->blk_size;

	if (cnt <= (memory_block_count(&p->memblk) - extra)) {
		p->ref_count++;
		p->blk_used += (cnt + extra);
		pref = memory_block_get(&p->memblk, cnt + extra);
		((struct memblk_desc *)pref)->blk_cnt = cnt + extra;
		return (void *)(pref + sizeof(struct memblk_desc));
	} else {
		errno = ENOMEM;
		return NULL;
	}

	return NULL;
}

void memory_pool_free(struct memory_pool *p, void *ptr)
{
	if (!p || !ptr)
		return;

	if (ptr < (void *)p->start)
		return;

	uint8_t *_ptr = (uint8_t *)ptr;
	_ptr -= sizeof(struct memblk_desc);
	
	memory_block_put(&p->memblk, ((struct memblk_desc *)_ptr)->blk_cnt);
	p->blk_used -= ((struct memblk_desc *)_ptr)->blk_cnt;
	p->ref_count--;
}

void memory_pool_info(struct memory_pool *p)
{
	if (!p)
		return;

	printf("Memory base    : %p\n", p->start);
	printf("Memory total   : %lu\n", p->total_size);
	printf("Memory free    : %lu\n", p->total_size - (p->blk_used * p->blk_size));
	printf("Block size     : %lu\n", p->blk_size);
	printf("Block count    : %u\n", p->blk_cnt);
	printf("Block used     : %u\n", p->blk_used);
	printf("Block free     : %u\n", p->blk_cnt - p->blk_used);
	printf("Memory refcount: %u\n", p->ref_count);
}

int main(void)
{
	char *p,*p1,*p2, *p3;
	struct memory_pool *pool = memory_pool_create(4096, 512, 0);
	if (!pool) {
		printf("Create memory pool failed:");
		perror("");
		return -1;
	}
	memory_pool_info(pool);

	printf("======================================\n");
	p = (char *)memory_pool_malloc(pool, 1024);
	if (!p) {
		printf("memory_pool_malloc\n");
		memory_pool_destroy(pool);
	}
	printf("Addr of p:%p\n", p);
	memory_pool_info(pool);
	memcpy(p, "hello world", 12);
	printf("the  value of p:%s\n", p);

	printf("======================================\n");
	p1 = (char *)memory_pool_malloc(pool, 512);
	if (!p1) {
		printf("memory_pool_malloc\n");
		memory_pool_destroy(pool);
	}
	printf("Addr of p1:%p\n", p1);
	memory_pool_info(pool);
	memcpy(p1, "hello world", 12);
	printf("the  value of p1:%s\n", p1);

	printf("======================================\n");
	p2 = (char *)memory_pool_malloc(pool, 55);
	if (!p2) {
		printf("memory_pool_malloc\n");
		memory_pool_destroy(pool);
	}
	printf("Addr of p2:%p\n", p2);
	memory_pool_info(pool);
	memcpy(p2, "hello world", 12);
	printf("the  value of p2:%s\n", p2);

	printf("======================================\n");
	p3 = (char *)memory_pool_malloc(pool, 3000);
	if (!p3) {
		printf("memory_pool_malloc\n");
		memory_pool_destroy(pool);
		return -1;
	}
	printf("Addr of p3:%p\n", p3);
	memory_pool_info(pool);
	memcpy(p3, "hello world", 12);
	printf("the  value of p3:%s\n", p3);
	printf("======================================\n");
	printf("free p\n");
	memory_pool_free(pool, p);
	memory_pool_info(pool);
	printf("======================================\n");
	printf("free p1\n");
	memory_pool_free(pool, p1);
	memory_pool_info(pool);
	printf("======================================\n");
	printf("free p2\n");
	memory_pool_free(pool, p2);
	memory_pool_info(pool);

	memory_pool_destroy(pool);
}


