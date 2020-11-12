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
#include "coremap.h"
#include "swapfile.h"

/*
 * Wrap ram_stealmem in a spinlock.
 */
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

//spinlock used to ensure mutual exclusion when manipulating the couple of arrays
static struct spinlock freemem_lock = SPINLOCK_INITIALIZER;

//the initial part of the memory is used by the kernel and this won't never be freed, for this we could choose to not represent
//that initial part. 
static unsigned char *freeRamFrames = NULL; //array of char which contains info about if a frame is occupied/freed
static unsigned long *allocSize = NULL; //array to define the # of frames occupied starting from the i-th frame in memory 
static int nRamFrames = 0;

static int allocTableActive = 0; //used to define if this mechanism is active or not

static int isTableActive () {  //access allocTableActive ensuring the mutual exclusion
  int active;
  spinlock_acquire(&freemem_lock);
  active = allocTableActive;
  spinlock_release(&freemem_lock);
  return active;
}

void
vm_bootstrap(void)
{
  int i;
  nRamFrames = ((int)ram_getsize())/PAGE_SIZE;  //defines number of frames
  /* alloc freeRamFrame and allocSize. Done dynamically because RAM size could change*/  
  freeRamFrames = kmalloc(sizeof(unsigned char)*nRamFrames);
  if (freeRamFrames==NULL) return;  
  allocSize     = kmalloc(sizeof(unsigned long)*nRamFrames);
  if (allocSize==NULL) {    
    /* reset to disable this vm management */
    freeRamFrames = NULL; return;
  }
  for (i=0; i<nRamFrames; i++) {    //init the array at 0 values= all the frames are free but not freed by any process
    freeRamFrames[i] = (unsigned char)0;
    allocSize[i]     = 0;  
  }
  spinlock_acquire(&freemem_lock);
  allocTableActive = 1;
  spinlock_release(&freemem_lock);
  /* Init swapfile */
  swapfile_init();
}

static paddr_t 
getfreeppages(unsigned long npages) {
  paddr_t addr;	
  long i, first, found, np = (long)npages;

  if (!isTableActive()) return 0; 
  spinlock_acquire(&freemem_lock);
  //linear algorithm to search in freeRamFrames the long enough string of 1s 
  for (i=0,first=found=-1; i<nRamFrames; i++) {
    if (freeRamFrames[i]) {
      if (i==0 || !freeRamFrames[i-1]) 
        first = i; /* set first free in an interval */   
      if (i-first+1 >= np) {
        found = first;
        break;
      }
    }
  }
	
  if (found>=0) {  //if that sequence is found we set at 0 the correspondent elements in the array
    for (i=found; i<found+np; i++) {
      freeRamFrames[i] = (unsigned char)0;
    }
    allocSize[found] = np; //and set that np frames have been occupied starting from the found-th frame
    addr = (paddr_t) found*PAGE_SIZE;
  }
  else {
    addr = 0;
  }

  spinlock_release(&freemem_lock);

  return addr;
}

paddr_t
getppages(unsigned long npages) //high level function that calls getfreeppages 
{
  paddr_t addr;

  /* try (occupied and then)freed pages first */
  addr = getfreeppages(npages);
  if (addr == 0) { //there are not freed pages
    /* call stealmem */
    spinlock_acquire(&stealmem_lock);
    addr = ram_stealmem(npages);
    spinlock_release(&stealmem_lock);
  }
  if (addr!=0 && isTableActive()) { //otherwise we update allocSize vector (number of frames occupied from the i-th frame on) 
    spinlock_acquire(&freemem_lock);
    allocSize[addr/PAGE_SIZE] = npages;
    spinlock_release(&freemem_lock);
  } 

  return addr;
}

int 
freeppages(paddr_t addr, unsigned long npages){
  long i, first, np=(long)npages;	

  if (!isTableActive()) return 0; 
  first = addr/PAGE_SIZE; //get the index of the starting frame to free
  KASSERT(allocSize!=NULL);
  KASSERT(nRamFrames>first);

  spinlock_acquire(&freemem_lock);
  for (i=first; i<first+np; i++) {		//set in freeRamFrames np element at 1 to represent that np frames from first-th frame have been freed 
    freeRamFrames[i] = (unsigned char)1;
  }
  spinlock_release(&freemem_lock);

  return 1;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t pa;

	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void 
free_kpages(vaddr_t addr){
  if (isTableActive()) {
    paddr_t paddr = addr - MIPS_KSEG0;
    long first = paddr/PAGE_SIZE;	
    KASSERT(allocSize!=NULL);
    KASSERT(nRamFrames>first);
    freeppages(paddr, allocSize[first]);	
  }
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}