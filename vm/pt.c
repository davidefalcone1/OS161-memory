#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include "pt.h"

struct pt_entry *page_table_push(struct pt_entry *page_table){
	struct pt_entry *new_node;
	/* Max size of a process in pages is 2GB / PAGE_SIZE */
	new_node = (struct pt_entry *) kmalloc(sizeof(struct pt_entry));
	if(new_node == NULL)
		return NULL;
	new_node->next = page_table;
	return new_node;
}

struct pt_entry *page_table_get(struct pt_entry *page_table, vaddr_t vaddr){
	struct pt_entry *p = page_table;
	while(p != NULL){
		if(p->vaddr == vaddr)
			return p;
		p = p->next;
	}
	return NULL;
}
void page_table_free(struct pt_entry *page_table){
	struct pt_entry *p = page_table, *n;
	while(p != NULL){
		n = p->next;
		kfree(p);
		p = n;
	}
}











