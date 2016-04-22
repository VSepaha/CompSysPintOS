#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdbool.h>
#include <stdint.h>
#include <list.h>
#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/synch.h"


struct list frame_tbl_list;
struct lock frame_tbl_lock;

// struct for the frame table entry
struct frame_tbl_ent {
  	void *frame; // frame in the entry
  	struct thread *thread; //thread for the frame table entry
  	struct suppl_page_table_ent *spte; // supplemental page table enrty
  	struct list_elem elem; // list element
};


// functions defined in frame.c //
// Used to initialize the frame table
void frame_tbl_init(void); 
// Used to add a framme to the table
void add_frame_to_tbl (void *frame, struct sup_page_entry *spte);
// allocate memory for the frame
void* frame_alloc(enum palloc_flags flags, struct sup_page_entry *spte);
// free tbe frame passed as argument
void free_frame(void *frame);
// frame eviction policy
void* evict_frame(enum palloc_flags flags);

#endif /* vm/frame.h */