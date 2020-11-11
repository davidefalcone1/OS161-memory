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
#include <uio.h>
#include <vfs.h>
#include <vnode.h>
#include <kern/fcntl.h>
#include "swapfile.h"

struct swap_table_entry{
    struct addrspace *as;
    vaddr_t vaddr;
    int free;
};

#define SWAPFILE_SIZE 9 * 1024 * 1024

static struct swap_table_entry swap_table[SWAPFILE_SIZE / PAGE_SIZE];
static struct vnode *SWAP_FILE;
char swapfile_name[] = "emu0:SWAPFILE"; 
static int swap_pages = SWAPFILE_SIZE / PAGE_SIZE;

void swapfile_init(void){
    int result, i;
    result = vfs_open(swapfile_name, O_RDWR|O_CREAT, 0, &SWAP_FILE);
    if(result)
        panic("Cannot open SWAPFILE");
    for(i = 0; i < swap_pages; i++)
        swap_table[i].free = 1;
}

int swapfile_resident(vaddr_t vaddr){
    int i;
    struct addrspace *as = proc_getas();
    for(i = 0; i < swap_pages; i++){
        if(as == swap_table[i].as && vaddr == swap_table[i].vaddr)
            return i;
    }
    return -1;
}

int swap_read(vaddr_t vaddr, int index){
    int result;
    struct iovec iov;
	struct uio u;
    uio_init(&iov, &u, (void *)vaddr, PAGE_SIZE, PAGE_SIZE * index, 
                UIO_READ, 0);
    result = VOP_READ(SWAP_FILE, &u);
    /* Clean swap page entry in swap map */
    swap_table[index].free = 1;
    swap_table[index].as = NULL;
    swap_table[index].vaddr = 0;
    if(result)
        return result;
    return 0;
}

void swap_write(vaddr_t vaddr){
    struct addrspace *as = proc_getas();
    int result, i;
    struct iovec iov;
	struct uio u;
    for(i = 0; i < swap_pages; i++){
        if(swap_table[i].free){
            /* Free swap page found */
            uio_init(&iov, &u, (void *)vaddr, PAGE_SIZE, PAGE_SIZE * i, 
                UIO_WRITE, 0);
            result = VOP_WRITE(SWAP_FILE, &u);
            if(result)
                panic("Cannot write in swap file.");
            swap_table[i].as = as;
            swap_table[i].vaddr = vaddr;
            swap_table[i].free = 0;
            return;
        }
    }
    panic("Out of swap space");
}





