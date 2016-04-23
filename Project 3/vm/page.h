/*----------------------------------------------------------*/

/*
 * This is a work in progress...
 * To be updated accordingly.
 *
 * @author kevinwu52
 */

/*----------------------------------------------------------*/
 
#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "vm/frame.h"


#define FILE 0
#define SWAP 1
#define MMAP 2
#define HASH_ERROR 3

#define MAX_STACK_SIZE (1 << 23) 

struct suppl_page_tbl_ent{

	void *unused_virtual_address;
	uint8_t type;
	bool writable;

	bool is_loaded;
	bool pinned;

	struct file *file;

	// These values are initalized to "0"
	size_t offset;
	size_t read_bytes;
	size_t zero_bytes;

	size_t swap_index;

	struct hash_elem element;

};

struct suppl_page_tbl_ent* get_spte (void *unused_virtual_address);

/* ATTENTION
 * 
 * There is a difference between "suppl_page_tbl_ent" and "suppl_page_table"
 */

void page_table_init (struct hash *suppl_page_table);
void page_table_rm (struct hash *suppl_page_table);

bool load_page (struct suppl_page_tbl_ent *spte);
bool load_mmap (struct suppl_page_tbl_ent *spte);
bool load_swap (struct suppl_page_tbl_ent *spte);
bool add_file_page_tbl (struct file *file, int32_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable);
bool add_mmap_page_tbl (struct file *file, int32_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable);
bool stack_growth (void *unused_virtual_address);


#endif 