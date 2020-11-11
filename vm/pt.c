#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include "pt.h"
#include "vm_tlb.h"
#include "swapfile.h"

paddr_t page_is_resident(vaddr_t vaddr){
    int i;
    struct addrspace *as = proc_getas();
    for(i = 0; i < N_FRAME; i++){
        if(vaddr == as->page_table[i].vaddr)
            return as->page_table[i].paddr;
    }
    return 0;
}

static int get_victim(){
    static int victim = 0;
    return victim++ % N_FRAME;
}

paddr_t get_proc_frame(vaddr_t vaddr){
    struct addrspace *as = proc_getas();
    int i, tlb_index;
    for(i = 0; i < N_FRAME; i++){
        if(!as->page_table[i].resident){
            as->page_table[i].vaddr = vaddr;
            as->page_table[i].resident = 1;
            return as->page_table[i].paddr;
        }
    }
    /* Page replacement */
    i = get_victim();
    if(as->page_table[i].vaddr >= as->as_vbase2)
        swap_write(as->page_table[i].vaddr);
    /* Lookup TLB and remove if present */
    tlb_index = tlb_resident(as->page_table[i].vaddr);
    if(tlb_index >= 0)
        tlb_remove(tlb_index);
    /* Set page table entry */
    as->page_table[i].resident = 1;
    as->page_table[i].vaddr = vaddr;
    return as->page_table[i].paddr;
}

