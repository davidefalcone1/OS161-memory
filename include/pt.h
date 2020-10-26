#ifndef PT_H
#define PT_H

struct pt_entry {
        vaddr_t vaddr;
	paddr_t paddr;
	int resident;
        vaddr_t swap_offset; /* NULL if not in swap file */
        struct pt_entry *next;
};
struct pt_entry *page_table_push(struct pt_entry *page_table);
struct pt_entry *page_table_get(struct pt_entry *page_table, vaddr_t vaddr);
void page_table_free(struct pt_entry *page_table);
#endif
