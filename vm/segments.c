#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <elf.h>
#include "segments.h"
#include "pt.h"

int read_executable_header(struct vnode *v, Elf_Ehdr *eh){
	struct iovec iov;
       	struct uio ku;
	int result;
	/*
	 * Read the executable header from offset 0 in the file.
	 */
	uio_kinit(&iov, &ku, eh, sizeof(Elf_Ehdr), 0, UIO_READ);
	result = VOP_READ(v, &ku);
	if (result) {
		return result;
	}
	return 0;
}

int init_segments(struct vnode *v, Elf_Ehdr eh, struct addrspace *as){
	int result, i;
	Elf_Phdr ph;   /* "Program header" = segment header */
	struct iovec iov;
       	struct uio ku;
	for (i=0; i<eh.e_phnum; i++) {
		off_t offset = eh.e_phoff + i*eh.e_phentsize;
		uio_kinit(&iov, &ku, &ph, sizeof(ph), offset, UIO_READ);

		result = VOP_READ(v, &ku);
		if (result) {
			return result;
		}

		if (ku.uio_resid != 0) {
			/* short read; problem with executable? */
			kprintf("ELF: short read on phdr - file truncated?\n");
			return ENOEXEC;
		}

		switch (ph.p_type) {
		    case PT_NULL: /* skip */ continue;
		    case PT_PHDR: /* skip */ continue;
		    case PT_MIPS_REGINFO: /* skip */ continue;
		    case PT_LOAD: break;
		    default:
			kprintf("loadelf: unknown segment type %d\n",
				ph.p_type);
			return ENOEXEC;
		}

		result = as_define_region(as,
					  ph.p_vaddr, ph.p_memsz,
					  ph.p_flags & PF_R,
					  ph.p_flags & PF_W,
					  ph.p_flags & PF_X);
		if (result) {
			return result;
		}
	}
	return 0;
}

int load_from_elf(struct vnode *v, vaddr_t vaddr, paddr_t paddr){
	int result;
	struct iovec iov;
       	struct uio ku;
	uio_kinit(&iov, &ku, (void *)PADDR_TO_KVADDR(paddr), PAGE_SIZE, PAGE_SIZE * vaddr, UIO_READ);
	result = VOP_READ(v, &ku);
	if(result)
		return result;
	return 0;
}







