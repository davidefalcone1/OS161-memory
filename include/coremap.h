#ifndef COREMAP_H
#define COREMAP_H

void table_init(int nRamFrames);
int freeppages(paddr_t addr);
paddr_t getfreeppages(unsigned long npages);
int isTableActive (void);
void update_table(paddr_t addr, int npages, vaddr_t vaddr);
paddr_t page_replacement(void);
paddr_t get_paddr(vaddr_t vaddr);
#endif
