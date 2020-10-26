#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <elf.h>
#include "swapfile.h"

#define SWAP_SIZE 9 * 1024 * 1024

struct vnode *swapfile;
char *swapfile_table;

static int get_swap_page(){
    int i;
    for(i = 0; i < SWAP_SIZE / PAGE_SIZE; i++)
        if(!swapfile_table)
            return i;
    return -1;
}

int swap_in(struct pt_entry *page){
    struct iovec iov;
    struct uio u;
    int result, swap_page;
    /* Get a free page of the swap file */
    swap_page = get_swap_page();
    if(swap_page == -1)
        panic("Out of swap space!"); 
    uio_kinit(&iov, &u, PADDR_TO_KVADDR(p->paddr), PAGE_SIZE, swap_page * PAGE_SIZE, UIO_WRITE, 0);
    result = VOP_READ(swapfile, &u);
    if(result)
        return result;
    /* Update swap file table */
    swapfile_table[swap_page] = 1;
    /* Update page table */
    page->swap_offset = swap_page * PAGE_SIZE;
    return 0;
}

int swap_out(struct pt_entry *page){
    struct iovec iov;
    struct uio u;
    int result;
    uio_kinit(&iov, &u, PADDR_TO_KVADDR(p->paddr), PAGE_SIZE, page->swap_offset, UIO_READ, 0);
    result = VOP_WRITE(swapfile, &u);
    if(result)
        return result;
    /* Update swap file table */
    swapfile_table[page->swap_offset / PAGE_SIZE] = 0;
    /* Update page table */
    page->swap_offset = NULL;
    return 0;
}

int init_swapfile(){
    int result;
    /* Open swap file */
    result = vfs_open("SWAPFILE", O_RDWR, 0, swapfile);
    if(result)
        return result;
    /* Init table */
    swapfile_table = (char *)kmalloc(sizeof(int) * SWAP_SIZE / PAGE_SIZE);
    if(swapfile_table == NULL)
        return 1;
    return result;
}