#include "vm/swap.h"
#include <stdio.h>
#include <string.h>
#include <bitmap.h>
#include "devices/block.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"

//Initializes swap table
void start_swap (void)
{
  swap_block = block_get_role (BLOCK_SWAP); //Sets the swap role to block
  if (!swap_block)
    {
      return; //Returns if swap is not blocked
    }
  swap_map = bitmap_create( block_size(swap_block) / SECTORS_PER_PAGE ); //Makes a bitmap for swap
  if (!swap_map)
    {
      return; //Returns if there is no swap map
    }
  bitmap_set_all(swap_map, SWAP_FREE); //Calls bitmap with the map and free arguments for swap
  lock_init(&swap_lock); //Calls the lock intialization with the help of swap lock
}

//Loads from the swap to main memory
void swap_in (size_t used_index, void* frame)
{
  if (!swap_block || !swap_map)
    {
      return; //Returns if there is no swap block or frame
    }
  lock_acquire(&swap_lock); //Acquires the lock status for swap
  if (bitmap_test(swap_map, used_index) == SWAP_FREE)
    {
      PANIC ("Trying to swap in a free block! Kernel panicking."); //Goes into panic mode if the swap block is free 									     and swap in is in process
    }
  bitmap_flip(swap_map, used_index); //Flips the bitmap

  size_t i;
  for (i = 0; i < SECTORS_PER_PAGE; i++) //Block reader
    {
      block_read(swap_block, used_index * SECTORS_PER_PAGE + i,
     (uint8_t *) frame + i * BLOCK_SECTOR_SIZE);
    }
  lock_release(&swap_lock); //Releases the swap lock
}

//switches from main memory to swap disk
size_t swap_out (void *frame)
{
  if (!swap_block || !swap_map)
    {
      PANIC("Need swap partition but no swap partition present!"); //Goes into panic mode if there is no swap block or 									     map basically the partition does not exist
    }
  lock_acquire(&swap_lock); //Acquires the lock status
  size_t free_index = bitmap_scan_and_flip(swap_map, 0, 1, SWAP_FREE);

  if (free_index == BITMAP_ERROR)
    {
      PANIC("Swap partition is full!"); //Goes into panic mode when there is a bitmap error for the particular index 						  which basically means the partition is now full 
    }

  size_t i;
  for (i = 0; i < SECTORS_PER_PAGE; i++) //Block writer
    { 
      block_write(swap_block, free_index * SECTORS_PER_PAGE + i,
      (uint8_t *) frame + i * BLOCK_SECTOR_SIZE);
    }
  lock_release(&swap_lock); //Releases swap lock status
  return free_index; //Returns the free index
}
