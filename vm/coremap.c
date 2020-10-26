#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spinlock.h>
#include <vm.h>
#include <proc.h>
#include "coremap.h"

struct coremap_entry{
	struct addrspace *as; /* Used as ASID */
	unsigned char free;
	unsigned char dirty;
	unsigned char reference;
	vaddr_t vaddr;
};

struct fifo_entry {
	paddr_t paddr;
	struct fifo_entry *next;
	struct fifo_entry *prev;
};

static struct spinlock freemem_lock = SPINLOCK_INITIALIZER;
static struct coremap_entry *coremap = NULL;
static unsigned long *allocSize = NULL;
static int nRamFrames = 0;
static int coremap_active = 0;
static struct fifo_entry *fifo_head = NULL, *fifo_tail = NULL;

paddr_t get_paddr(vaddr_t vaddr){
	struct addrspace *as = proc_getas();
	int i;
	for(i = 0; i < nRamFrames; i++)
		if(coremap[i].as == as && coremap[i].vaddr == vaddr)
			return (paddr_t) i*PAGE_SIZE;
	return 0;
}

int isTableActive () {
	int active;
	spinlock_acquire(&freemem_lock); 
	active = coremap_active; 
	spinlock_release(&freemem_lock); 
	return active;
}

static void fifo_push_back(paddr_t addr){
	struct fifo_entry *new_node = kmalloc(sizeof(struct fifo_entry));
	new_node->next = NULL;
	new_node->paddr = addr;
	new_node->prev = fifo_tail;
	fifo_tail->next = new_node;
}

static paddr_t fifo_pop(void){
	struct fifo_entry *f = fifo_head;
	paddr_t addr = f->paddr;
	fifo_head = fifo_head->next;
	kfree(f);
	return addr;
}

paddr_t page_replacement(void){
	return fifo_pop();
}

void update_table(paddr_t addr, int npages, vaddr_t vaddr){
	spinlock_acquire(&freemem_lock); 
	allocSize[addr/PAGE_SIZE] = npages;
	coremap[addr/PAGE_SIZE].vaddr = vaddr;
	coremap[addr/PAGE_SIZE].as = proc_getas();
	coremap[addr/PAGE_SIZE].dirty = 0;
	coremap[addr/PAGE_SIZE].reference = 0;
	coremap[addr/PAGE_SIZE].free = 0;
	spinlock_release(&freemem_lock);
}

paddr_t getfreeppages(unsigned long npages){
	paddr_t addr;
	long first, found, i, np = (long) npages;
	if(!isTableActive()) return 0;

	spinlock_acquire(&freemem_lock);
	for(i=0, first=found=-1; i<nRamFrames; i++){
		if(coremap[i].free){
			if(i==0 || !coremap[i].free)
				first = i;
			if(i-first+1 >= np){
				found = first;
				break;
			}
		}
	}
	if(found>=0){
		for (i=found;i<found+np;i++){
			coremap[i].free=(unsigned char)0;
		}
		allocSize[found]=np;
		addr=(paddr_t)found*PAGE_SIZE;
		bzero((void *)PADDR_TO_KVADDR(addr), PAGE_SIZE * npages);
	}
	else addr=0;
	spinlock_release(&freemem_lock);
	return addr;
}

int freeppages(paddr_t addr){
	long first, i, np = (long) allocSize[addr/PAGE_SIZE];
	spinlock_acquire(&freemem_lock);
	first = addr / PAGE_SIZE;
	for(i=first; i<first+np; i++){
		coremap[i].free = (unsigned char)1;
		fifo_push_back(i * PAGE_SIZE);
	}
	spinlock_release(&freemem_lock);
	return 1;
}

void table_init(int nRamFrames){
	coremap = (struct coremap_entry *) kmalloc(nRamFrames * sizeof(struct coremap_entry));
	allocSize = kmalloc(nRamFrames * sizeof(long));
	if(coremap == NULL || allocSize == NULL) {
		coremap = NULL;
	 	allocSize = NULL;
		return;
	}
	int i;
	for(i=0; i<nRamFrames; i++) {
		coremap[i].free = (unsigned char) 0;
		allocSize[i] = 0;
	}
	spinlock_acquire(&freemem_lock);
	coremap_active = 1; 
	spinlock_release(&freemem_lock);
}
