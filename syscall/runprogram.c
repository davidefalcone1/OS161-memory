/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than runprogram() does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <vnode.h>
#include <elf.h>
#include <uio.h>
#include "coremap.h"

#if OPT_PAGING
static int read_elf_header(struct vnode *v, vaddr_t *entrypoint){
	struct iovec iov;
	struct uio ku;
	Elf_Ehdr eh; /* Executable header */
	Elf_Phdr ph; /* Program header */
	struct addrspace *as = proc_getas();
	int result, i;
	
	uio_kinit(&iov, &ku, &eh, sizeof(eh), 0, UIO_READ);
	result = VOP_READ(v, &ku);

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
			kprintf("vm: unknown segment type %d\n",
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
		if(as->offset_text_elf == -1)
			as->offset_text_elf = ph.p_offset;
		else if(as->offset_data_elf == -1)
			as->offset_data_elf = ph.p_offset;
	}

	/* Set entrypoint */
	*entrypoint = eh.e_entry;
	return result;
}
static int alloc_process_frames(){
	int i;
	paddr_t paddr;
	struct addrspace *as = proc_getas();
	if(as == NULL)
		return 1;
	for(i = 0; i < N_FRAME; i++){
		paddr = getppages(1);
		if(!paddr)
			return 2;
		as->page_table[i].paddr = paddr;
		as->page_table[i].vaddr = 0;
		as->page_table[i].resident = 0;
	}
	return 0;
}
#endif

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname)
{
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* We should be a new process. */
	KASSERT(proc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	proc_setas(as);
	as_activate();

	#if OPT_PAGING
	/* Set elf vnode in as struct */
	as->v = v;
	/* Read executable header and segment headers */
	read_elf_header(v, &entrypoint);
	
	/* Alloc N_FRAME frames to process */
	alloc_process_frames();
	#else
	result = load_elf(v, &entrypoint);
	/* Done with the file now. */
	vfs_close(v);
	#endif

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

	/* Warp to user mode. */
	enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

