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
#include "segments.h"
#include "swapfile.h"
#include "vm_tlb.h"
#include "file_syscalls.h"
#include "vmstats.h"

#define STACKPAGES    18

typedef enum{
	CODE,
	DATA,
	STACK
}seg_type;

static int tlb_get_rr_victim(void){
	int victim;
	static unsigned int next_victim = 0;
	victim = next_victim;
	next_victim = (next_victim + 1) % NUM_TLB;
	return victim;
}

int tlb_resident(vaddr_t vaddr){
	uint32_t ehi, elo;
	ehi = vaddr;
	elo = 0;
	return tlb_probe(ehi, elo);
}
void tlb_remove(int index){
	tlb_write(TLBHI_INVALID(index), TLBLO_INVALID(), index);
}

int vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i, need_load, spl, result, inserted = 0, swap_index, tlb_index;
	uint32_t ehi, elo;
	struct addrspace *as;
	seg_type segment;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "vm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
			sys_exit(EPERM);
		/* We always create pages read-write, so we can't get this */
		//panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	inc_TLB_faults();

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_vbase1 != 0);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		segment = CODE;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		segment = DATA;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		segment = STACK;
	}
	else {
		return EFAULT;
	}

	paddr = page_is_resident(faultaddress);
	if(!paddr) {
		paddr = get_proc_frame(faultaddress);
		need_load = 1;
	}
	else {
		need_load = 0;
		inc_TLB_reload();
	}
    
	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		tlb_write(ehi, elo, i);
		splx(spl);
		inserted = 1;
		inc_TLB_faults_free();
		break;
	}

	if(!inserted) {
		/* TLB replacement */
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		tlb_write(ehi, elo, tlb_get_rr_victim());
		splx(spl);
		inc_TLB_faults_replace();
	}

    if(need_load) {
        /* Page fault */
		// Clear the frame
		bzero((void *)faultaddress, PAGE_SIZE); 
		switch (segment)
		{
		case CODE:
			result = load_page_from_elf(faultaddress, 1);
			//Set code page read only after loading it from the ELF
			spl = splhigh();
			tlb_index = tlb_resident(faultaddress);
			ehi = faultaddress;
			elo = paddr | TLBLO_VALID;
			tlb_write(ehi, elo, tlb_index);
			splx(spl);
			if(result)
				return result;
			inc_PF_disk();
			break;
		case DATA:
			// Check whether page is in swap file 
			swap_index = swapfile_resident(faultaddress);
			if(swap_index >= 0){
				// If so, load from swap file
				result = swap_read(faultaddress, swap_index);
				if(result)
					return result;
			}
			else{
				result = load_page_from_elf(faultaddress, 0);
				if(result)
					return result;
			}
			inc_PF_disk();
			break;
		case STACK:
			// Check wheter page is in swap file
			swap_index = swapfile_resident(faultaddress);
			if(swap_index >= 0){
				// If so, load from swap file
				result = swap_read(faultaddress, swap_index);
				if(result)
					return result;
				inc_PF_disk();
			}
			else
				inc_PF_zeroed();
			break;
		default:
			return EFAULT;
			break;
		}	
    }
	return 0;
}