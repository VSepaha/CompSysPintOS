#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <bitmap.h>
#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)
#define SWAP_FREE 0
#define SWAP_IN_USE 1

struct block *swap_block;
struct bitmap *swap_map;
struct lock swap_lock;

//Prit change these names and comment the old names
void swap_init (void);
size_t swap_out (void *frame);
void swap_in (size_t used_index, void* frame);

#endif /* vm/swap.h */
