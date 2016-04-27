/*----------------------------------------------------------*/

/*
 * This is a work in progress.
 * To be updated accordingly.
 *
 * Use at your own discretion
 *
 * @author kevinwu52
 */

 /*----------------------------------------------------------*/
#include <string.h>
#include <stdbool.h>

#include "filesys/file.h"

#include "threads/interrupt.h" 
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/thread.h"
#include "threads/vaddr.h" 

#include "userprog/syscall.h"

#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

/*
 * Functions takes in elements to the suppl page table and places it in appropriate location
 *
 * @return location of an Unused Virtual Address
 */
static unsigned page_hash (const struct hash_elem *e, void *aux UNUSED){
	struct suppl_page_tbl_ent *spte = hash_entry(e, struct suppl_page_tbl_ent, element);

	return hash_int((int) spte -> unused_virtual_address);
}

/*
 * Functions to compare the hash entry of elements
 *
 * @return TRUE if first elem is less than second elem, FALSE otherwise
 */
 static bool page_compare (const struct hash_elem *a,
 								 const struct hash_elem *b,
 								 void *aux UNUSED)
 {
 	struct suppl_page_tbl_ent *sa = hash_entry(a, struct suppl_page_tbl_ent, element);
 	struct suppl_page_tbl_ent *sb = hash_entry(b, struct suppl_page_tbl_ent, element);

 	if(sa -> unused_virtual_address < sb -> unused_virtual_address)
 		{
 			return true;
 		}
 	return false;
 }

/*
 * Functions checks to see if the page that is being requested is loaded
 * If page is loaded, free the corresponding resources
 *
 * @return void
 */

static void page_function (struct hash_elem *e, void *aux UNUSED)
{
	struct suppl_page_tbl_ent *spte = hash_entry(e,struct suppl_page_tbl_ent, element);

	if(spte -> is_loaded)
	{
		free_frame(pagedir_get_page(thread_current() -> pagedir, spte -> unused_virtual_address));
		pagedir_clear_page(thread_current() -> pagedir, spte -> unused_virtual_address);
	}
	free(spte);
}

/*
 * Function initializes a page table based on...
 * a) current suppl page table
 * b) corresponding hash function
 * c) page less function
 *
 * Uses "hash_init", built in function located in <hash.h>
 *
 * @return void
 */
void page_table_init (struct hash *suppl_page_table){
	hash_init(suppl_page_table, page_hash, page_compare, NULL);
}

/*
 * Function to remove a page table
 * 
 * Uses hash_destroy, built in function located in <hash.h>
 *
 * @return void
 */
void page_table_rm (struct hash *suppl_page_table){
	hash_destroy (suppl_page_table, page_function);
}

/*
 * Function to remove a page table
 * 
 * Uses hash_destroy, built in function located in <hash.h>
 *
 * @return pointer to supplemental page table entry
 */
struct suppl_page_tbl_ent* get_spte (void * unused_virtual_address){

	struct suppl_page_tbl_ent spte;
	
	// pg_round_down is a built in function located in /src/threads/vaddr.h
	spte.unused_virtual_address = pg_round_down(unused_virtual_address);

	struct hash_elem *e = hash_find(&thread_current() ->spte, &spte.elem);
	if(!e)
	{
		return NULL;
	}
	return hash_entry(e, struct suppl_page_tbl_ent, element);
}

/*
 * Function to load the page given the supplemental page table entry pointer
 * First, check if the bool value "is_loaded" is true, if so @return false
 * If "is_loaded" isn't true, depending on the type the suppl_page_table_ent is
 * we perform different respective tasks
 *
 * @return success if "is_loaded is true"
 * otherwise @return depending on corresponding functionss
 */
bool load_page (struct suppl_page_tbl_ent *spte)
{
	bool success = false;
	spte -> pinned = true;

	if(spte -> is_loaded)
	{
		return success;
	}

	switch(spte -> type)
	{
		case FILE:
			success = load_file(spte);
			break;
		case SWAP:
			success = load_swap(spte);
			break;
		case MMAP:
			success = load_file(spte);
			break;
	}
	return success;
}

/*
 * Function used to swap from frame to virtual address
 *
 * @return FALSE if no space in the frame, TRUE if swap is able to be performed
 */
bool load_swap (struct suppl_page_tbl_ent *spte)
{
	uint8_t *frame = frame_alloc( PAL_USER, spte);
	if(!frame)
	{
		return false;
	}
	if(!install_page(spte -> unused_virtual_address, frame, spte -> writable))
	{
		free_frame(frame);
		return false;
	}

	swap_in(spte -> swap_index, spte -> unused_virtual_address);
	spte -> is_loaded = true;
	return true;
}

/*
 * Function to load file
 * Checks if the frame is of proper size. If it is...
 * acquires lock to ensure no interrupts and loads the file.
 * Will release lock and free the frame after success
 * If !install_page(...), free frame and return false
 *
 * @return TRUE If file is loaded, FALSE otherwise
 */
bool load_file (struct suppl_page_tbl_ent *spte)
{
	enum palloc_flags flags = PAL_USER;
	if(spte -> read_bytes == 0)
	{
		flags |= PAL_ZERO;
	}
	uint8_t *frame = frame_alloc(flags, spte);
	if(!frame)
	{
		return false;
	}
	if(spte -> read_bytes > 0)
	{
		lock_acquire(&file_lock);
		if((int) spte -> read_bytes != file_read_at(spte -> file, frame,
													spte -> read_bytes,
													spte -> offset))
		{
			lock_release(&file_lock);
			free_frame(frame);
			return false;
		}
		lock_release(&file_lock);
		memset(frame + spte -> read_bytes, 0, spte -> zero_bytes);
	}

	if(!install_page(spte -> unused_virtual_address, frame, spte -> writable))
	{
		free_frame(frame);
		return false;
	}

	spte -> is_loaded = true;
	return true;
}

/*
 * Function to add a file to a page tbale
 *
 * First, allocate space in memory corresponding to the size of the supplemental page table
 * Next, check if the we have a valid supplemental page table entry, if not @return false
 * If true, continue...
 *
 * @return TRUE if success, FALSE otherwise
 */

bool add_file_page_tbl (struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes,
			     bool writable)
{
  struct suppl_page_tbl_ent *spte = malloc(sizeof(struct suppl_page_tbl_ent));
  if (!spte)
    {
      return false;
    }
  spte -> file = file;
  spte -> offset = ofs;
  spte -> unused_virtual_address = upage;
  spte -> read_bytes = read_bytes;
  spte -> zero_bytes = zero_bytes;
  spte -> writable = writable;
  spte -> is_loaded = false;
  spte -> type = FILE;
  spte -> pinned = false;

  return (hash_insert(&thread_current() -> suppl_page_table, &spte -> elem) == NULL);
}

/*
 * Function to map memory to the page table
 *
 * Implementation almost identical to add_file_to_page_tbl()
 *
 * @return TRUE if success, FALSE otherwise
 */
bool add_mmap_page_tbl(struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes)
{
  struct suppl_page_tbl_ent *spte = malloc(sizeof(struct suppl_page_tbl_ent));
  if (!spte)
    {
      return false;
    }
  spte -> file = file;
  spte -> offset = ofs;
  spte -> unused_virtual_address = upage;
  spte -> read_bytes = read_bytes;
  spte -> zero_bytes = zero_bytes;
  spte -> is_loaded = false;
  spte -> type = MMAP;
  spte -> writable = true;
  spte -> pinned = false;

  if (!process_add_mmap(spte))
    {
      free(spte);
      return false;
    }

  if (hash_insert( &thread_current() -> suppl_page_table, &spte -> elem))
    {
      spte -> type = HASH_ERROR;
      return false;
    }
  return true;
}

/*
 * Function to ensure the stack hasn't exceeded it's boundaries plus add elements to the stack
 *
 * 
 *
 */
bool stack_growth (void *unused_virtual_address)
{
  if ( (size_t) (PHYS_BASE - pg_round_down(unused_virtual_address)) > MAX_STACK_SIZE)
    {
      return false;
    }
 struct suppl_page_tbl_ent *spte = malloc(sizeof(struct suppl_page_tbl_ent));
  if (!spte)
    {
      return false;
    }
  spte -> unused_virtual_address = pg_round_down(unused_virtual_address);		// pg_round_down is a built in PintOS function
  spte -> is_loaded = true;
  spte -> writable = true;
  spte -> type = SWAP;
  spte -> pinned = true;

  uint8_t *frame = frame_alloc (PAL_USER, spte);
  if (!frame)
    {
      free(spte);
      return false;
    }

  if (!install_page(spte->unused_virtual_address, frame, spte->writable))
    {
      free(spte);
      free_frame(frame);
      return false;
    }

  if (intr_context())
    {
      spte -> pinned = false;
    }

  return (hash_insert(&thread_current() -> suppl_page_table, &spte -> elem) == NULL);
}


