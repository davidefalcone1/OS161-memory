#ifndef VM_TLB_H
#define VM_TLB_H

int tlb_resident(vaddr_t vaddr);
void tlb_remove(int index);

#endif