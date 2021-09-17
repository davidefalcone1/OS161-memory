# OS161 memory management system
This work is the result of System and Device Programming course project, at Polytechnic University of Turin. It has been developed by Mattia Bencivenga, Aurelio Cirella and Davide Falcone using pair programming approach. 
This is an on-demand paging memory management system for the teaching operating system OS161. \
On-demand paging has been implemented via a local fixed-allocation policy, this means that a set of physical frames is allocated for each process and victim pages are chosen among those of the faulting process.
## TLB Management
TLB is managed entirely by means of functions defined in vm_tlb.c. Here, the system handles the cases in which the process references pages that are not present in TLB (“TLB miss”). 
The main function for this task is vm_fault in which the following steps are performed: 
- Check if the fault is triggered by a VM_FAULT_READONLY exception, if so the current process exits without generating a kernel panic.
- Determine the area of the process image the faultaddress belongs to.
- Retrieve the physical page according to the faulting logical page (see On-demand page loading).
- Scan the entire TLB looking for an INVALID entry:
  - If an entry is found, then the translation is inserted in that entry.
  - If an entry is not found, then is invoked the TLB entry replacement algorithm(tlb_get_rr_victim), the victim is selected and used to store the translation.
- Manage page loading, if needed (see On-demand page loading).

The replacement algorithm is a simple round robin.
vm_tlb.c includes also two support functions used during a Page Replacement: tlb_resident and tlb_remove. The first takes as parameter a virtual address and returns the index of the TLB entry that contains the translation related to the address, otherwise it returns a negative number. The second instead receives an entry index and removes that entry. They are used to remove an entry from the TLB when swapping out a page.

## On-Demand Page Loading
The system implements for paging management a Fixed-allocation policy: it allocates N_FRAMES frames for each process at process start and loads a page only when it’s referenced. When all these frames are occupied, a victim is searched among them using a round robin algorithm. This means that the system implements a local page replacement strategy. 
When a user process is run, the function runprogram is invoked. First, it opens the ELF file and sets its vnode in struct addrspace of the process. Then, runprogram calls read_elf_header that retrieves information about each segment. Subsequently, alloc_process_frames allocates N_FRAME (by using getppages) to the process and sets up the page table accordingly. Finally, enter_new_process is invoked and the process starts its execution.
Regarding point 3 of TLB management, function page_is_resident returns the physical page coupled with the fault address in case the page is in memory, otherwise it returns 0. If the page is not resident, get_proc_frame scans the entire page table looking for a free frame. If a free frame is found, its physical address is returned. Otherwise page replacement is performed. 
Regarding point 5 of TLB management, if the page was not in memory (page fault), the system zeroes the entire frame and behaves according to the area the fault address belongs to. 
- Code: load the page from ELF file with load_page_from_elf and mark its TLB entry as read-only (DIRTY bit unset).
- Data: load the page from the swapfile (see Page replacement). If it’s not on the swapfile, load it from ELF file with load_page_from_elf.
- Stack: load the page from the swapfile if present.

## Page Replacement
Page replacement comes in place in get_proc_frame, in particular when no free frames are available. After a victim is found, the kernel writes the frame in the swapfile (swap_write) if it does not belong to the code area. Then, checks if that page is present in TLB using tlb_resident and if so, it removes that entry using tlb_remove.
Logically, the swapfile is divided in pages, identified by an index. A swap table is used to keep track of used pages and to facilitate read and write operations.
For the swapfile management, the system uses the following functions: 
- swapfile_init is used to create the swapfile file 
- swap_free is used to mark as free all the pages belonging to a given addrspace when terminating a process. 
- swapfile_resident is used to retrieve the index of a page of the swapfile starting from a virtual address
- swap_read is used to read a page from the swapfile
- swap_write is used to write a page in the swapfile.
