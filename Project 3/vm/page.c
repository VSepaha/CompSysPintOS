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

#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

/*
 * Functions takes in elements to the suppl page table and places it un appropriate location
 *
 * @return location of an Unused Virtual Address
 */
static unsigned page_hash_function (const struct hash_elem *e, void *aux UNUSED){
	struct suppl_page_tbl_ent *spte = hash_entry(e, struct suppl_page_tbl_ent, elem);

	return hash_int((int) spte -> unused_virtual_address);
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
 *
 *
 * @return void
 *
 * TO DO:
 * check if pg_round_down is a built in function or need to implement
 *
 */
struct suppl_page_tbl_ent* get_spte (void * unused_virtual_address){

	struct suppl_page_tbl_entry spte;
	
	// see commment above
	spte.unused_virtual_address = pg_round_down(unused_virtual_address);

	struct hash_element *e = hash_find(&thread_curren() ->spte, &spte.elem);
	if(!e)
	{
		return NULL;
	}
	return hash_entry(e, struct suppl_page_tbl_ent, elem);

}