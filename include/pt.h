#ifndef PT_H
#define PT_H

#include "opt-paging.h"

struct pt_entry {
        vaddr_t vaddr;
        paddr_t paddr; 
        int resident;
};

paddr_t page_is_resident(vaddr_t vaddr);
paddr_t get_proc_frame(vaddr_t vaddr);
#endif

