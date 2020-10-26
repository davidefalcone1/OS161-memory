#ifndef SEGMENTS_H
#define SEGMENTS_H
#include <elf.h>
#include <addrspace.h>
#include <vnode.h>
#include "opt-dumbvm.h"
int read_executable_header(struct vnode *v, Elf_Ehdr *eh);
int init_segments(struct vnode *v, Elf_Ehdr eh, struct addrspace *as);
int load_from_elf(struct vnode *v, vaddr_t vaddr, paddr_t paddr);
#endif
