/*----------------------------------------------------------*/

/*
 * This is a work in progress...
 * To be updated accordingly.
 *
 */

/*----------------------------------------------------------*/
 
#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "vm.frame.h"


struct suppl_page_table_ent{

	void *unused_virtual_address;

	struct hash_elem element;

};

void page_table_init (struct hash *suppl_page_table);
void page_table_rm (struct hash *suppl_page_table);
