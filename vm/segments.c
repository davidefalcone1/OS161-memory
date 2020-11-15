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
#include <uio.h>
#include <vnode.h>
#include "vmstats.h"

int load_page_from_elf(vaddr_t vaddr, int is_code){
    struct iovec iov;
	struct uio u;
    struct addrspace *as = proc_getas();
    struct vnode *v = as->v;
    int result;
    off_t base = is_code ? as->offset_text_elf : as->offset_data_elf;
    off_t offset = is_code ? (vaddr - as->as_vbase1) : (vaddr - as->as_vbase2);

    uio_init(&iov, &u, (void *)vaddr, PAGE_SIZE, base + offset, UIO_READ, is_code);
    result = VOP_READ(v, &u);
    if(result)
        return result;
    inc_PF_elf();
    return 0;
}