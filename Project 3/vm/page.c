/*----------------------------------------------------------*/

/*
 * This is a work in progress.
 * To be updated accordingly.
 *
 * Use at your own discretion
 *
 * @author kevin.wu52
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

#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

/*
 * Functions takes in elements to the suppl page table and places it in appropriate location
 *
 * @return location of an Unused Virtual Address
 */
static unsigned page_hash_function (const struct hash_element *e, void *aux UNUSED){
	struct suppl_page_tbl_ent *spte = hash_entry(e, struct suppl_page_tbl_ent, element);

	return hash_int((int) spte -> unused_virtual_address);
}

/*
 * Functions to compare the hash entry of elements
 *
 * @return TRUE if first elem is less than second elem, FALSE otherwise
 */
 static bool page_less_function (const struct hash_element *a,
 								 const struct hash_element *b,
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

static void page_action_function (struct hash_element *e, void *aux UNUSED)
{
	struct suppl_page_tbl_ent *spte = hash_entry(e,struct suppl_page_tbl_ent, elem);

	if(spte -> is_loaded)
	{
		frame_free(pagedir_get_page(thread_current() -> pagedir, spte -> unused_virtual_address));
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
	hash_init(suppl_page_table, page_hash_function, page_less_function, NULL);
}

/*
 * Function to remove a page table
 * 
 * Uses hash_destroy, built in function located in <hash.h>
 *
 * @return void
 */
void page_table_rm (struct hash *suppl_page_table){
	hash_destroy (suppl_page_table, page_action_function);
}

/*
 * Function to remove a page table
 * 
 * Uses hash_destroy, built in function located in <hash.h>
 *
 * @return pointer to supplemental page table entry
 */
struct suppl_page_tbl_ent* get_spte (void * unused_virtual_address){

	struct suppl_page_tbl_entry spte;
	
	// pg_round_down is a built in function located in /src/threads/vaddr.h
	spte.unused_virtual_address = pg_round_down(unused_virtual_address);

	struct hash_element *e = hash_find(&thread_curren() ->spte, &spte.elem);
	if(!e)
	{
		return NULL;
	}
	return hash_entry(e, struct suppl_page_tbl_ent, elem);
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
		frame_free(frame);
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
		lock_acquire(&filesys_lock);
		if((int) spte -> read_bytes != file_read_at(spte -> file, frame,
													spte -> read_bytes,
													spte -> offset))
		{
			lock_release(&filesys_lock);
			frame_free(frame);
			return false;
		}
		lock_release(&filesys_lock);
		memset(frame + spte -> read_bytes, 0, spte -> zero_bytes);
	}

	if(!install_page(spte -> unused_virtual_address, frame, spte -> writable))
	{
		frame_free(frame);
		return false;
	}

	spte -> is_loaded = true;
	return true;
}

/*
 * Function to add a file to a page tbale
 *
 * @return TRUE if success, FALSE otherwise
 */

bool add_file_to_page_table (struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes,
			     bool writable)
{
  struct sup_page_entry *spte = malloc(sizeof(struct sup_page_entry));
  if (!spte)
    {
      return false;
    }
  spte->file = file;
  spte->offset = ofs;
  spte->uva = upage;
  spte->read_bytes = read_bytes;
  spte->zero_bytes = zero_bytes;
  spte->writable = writable;
  spte->is_loaded = false;
  spte->type = FILE;
  spte->pinned = false;

  return (hash_insert(&thread_current()->spt, &spte->elem) == NULL);
}

bool add_mmap_to_page_table(struct file *file, int32_t ofs, uint8_t *upage,
			     uint32_t read_bytes, uint32_t zero_bytes)
{
  struct sup_page_entry *spte = malloc(sizeof(struct suppl_page_tbl_ent));
  if (!spte)
    {
      return false;
    }
  spte->file = file;
  spte->offset = ofs;
  spte->uva = upage;
  spte->read_bytes = read_bytes;
  spte->zero_bytes = zero_bytes;
  spte->is_loaded = false;
  spte->type = MMAP;
  spte->writable = true;
  spte->pinned = false;

  if (!process_add_mmap(spte))
    {
      free(spte);
      return false;
    }

  if (hash_insert(&thread_current()->spt, &spte->elem))
    {
      spte->type = HASH_ERROR;
      return false;
    }
  return true;
}

bool stack_growth (void *uva)
{
  if ( (size_t) (PHYS_BASE - pg_round_down(uva)) > MAX_STACK_SIZE)
    {
      return false;
    }
 struct sup_page_entry *spte = malloc(sizeof(struct sup_page_entry));
  if (!spte)
    {
      return false;
    }
  spte->uva = pg_round_down(uva);
  spte->is_loaded = true;
  spte->writable = true;
  spte->type = SWAP;
  spte->pinned = true;

  uint8_t *frame = frame_alloc (PAL_USER, spte);
  if (!frame)
    {
      free(spte);
      return false;
    }

  if (!install_page(spte->uva, frame, spte->writable))
    {
      free(spte);
      frame_free(frame);
      return false;
    }

  if (intr_context())
    {
      spte->pinned = false;
    }

  return (hash_insert(&thread_current()->spt, &spte->elem) == NULL);
}






 // HEY DIPSHIT
 // grow_stack ==> stack_growth
 // I didn't mean it when i said dipshit
 // srry please no cryerino

