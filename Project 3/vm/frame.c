#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "filesys/file.h"
#include "threads/synch.h"



// Initialization of the frame table
void frame_tbl_init(void) {
	lock_init(&frame_tbl_lock); // Lock for the frame table
	list_init(&frame_tbl_list); // List to keep track of the frames in the table
}

// function used to add a frame to the table
void add_frame_to_tbl(void *frame, struct suppl_page_tbl_ent *spte) {
  struct frame_tbl_ent *fte = malloc(sizeof(struct frame_tbl_ent)); // allocate memory the size of the frame table

  // set the parameters for the frmane table entry
  fte->frame = frame;
  fte->spte = spte;
  fte->thread = thread_current();

  // add frame table entry on the frame table list
  lock_acquire(&frame_tbl_lock);
  list_push_back(&frame_tbl_list, &fte->elem);
  lock_release(&frame_tbl_lock);
}

// allocate memory for the frame table
void* frame_alloc(enum palloc_flags flags, struct suppl_page_tbl_ent *spte) {

// safeguard to ensure that PAL_USER is called 
  if ( (flags & PAL_USER) == 0 ) {
      return NULL;
    }

// Since we ensured that PAL_USER is set, the page is obtained 
// from the user pool
  void *frame = palloc_get_page(flags);

// If the page is successfully obtained from the user pool
	if (frame) {
  		// add the frame to the table
    	add_frame_to_tbl(frame, spte);
    } else {
    // when no frame is free, a frame must be made free through eviction algo
     	while (!frame) {
			frame = frame_evict(flags);
		  	lock_release(&frame_tbl_lock);
		}

	// no frame can be evicted without allocating a swap slot,
	// but swap slot is full -> panic the kernel
     	if (!frame) {
	  		PANIC("Frame could not be evicted");
		}

	//add the frame to table which has been freed up
      	frame_add_to_table(frame, spte);
    }

    // return the frame
	return frame;
}


void free_frame(void *frame) {

	//acquire lock
	lock_acquire(&frame_tbl_lock);

	//iterate through the list (frame table), and for the frame which 
	// is called to be free, remove it from the list and free the page 
	struct list_elem *e;
  	for (e = list_begin(&frame_tbl_list); e != list_end(&frame_tbl_list);
	    e = list_next(e)) {

  		//get the current frame from the frame table list
	    struct frame_tbl_ent *curr_frame = list_entry(e, struct frame_tbl_ent, elem);

		//check if the current frame is the frame which is going to be removed
	    if (curr_frame->frame == frame) {
	    	// remove the element from the list and free the current frame
			list_remove(e);
			free(curr_frame);
			palloc_free_page(frame);

			break;
		}
    }
    //release the lock
	lock_release(&frame_tbl_lock);
}

// frame eviction policy (kevin you can work on this if you want,
// or we can go with what we have) 
void* evict_frame (enum palloc_flags flags) {

	//acquire the lock
	lock_acquire(&frame_tbl_lock);

	//get the first element of the list
	struct list_elem *e = list_begin(&frame_tbl_list);

	while (true) {
    	struct frame_tbl_ent *fte = list_entry(e, struct frame_tbl_ent, elem);

      	if (!fte->spte->pinned) {
	  		struct thread *t = fte->thread;

	  		if (pagedir_is_accessed(t->pagedir, fte->spte->uva)) {
	      		pagedir_set_accessed(t->pagedir, fte->spte->uva, false);
	    	} else {
	      		if (pagedir_is_dirty(t->pagedir, fte->spte->uva) ||
					fte->spte->type == SWAP) {

		  			if (fte->spte->type == MMAP) {
		      			lock_acquire(&filesys_lock);
		      			file_write_at(fte->spte->file, fte->frame,
				    	fte->spte->read_bytes,
				   		fte->spte->offset);
		      			lock_release(&filesys_lock);
		    		} else {
		      			fte->spte->type = SWAP;
		      			fte->spte->swap_index = swap_out(fte->frame);
		    		}

				}

		      fte->spte->is_loaded = false;
		      list_remove(&fte->elem);
		      pagedir_clear_page(t->pagedir, fte->spte->uva);
		      palloc_free_page(fte->frame);
		      free(fte);
		      return palloc_get_page(flags);
		    }

		}
     	e = list_next(e);
    	if (e == list_end(&frame_tbl_list)) {
      		e = list_begin(&frame_tbl_list);
		}
    }
}

