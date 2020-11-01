#include <types.h>
#include <kern/unistd.h>
#include <stdarg.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <mainbus.h>
#include <vfs.h>          // for vfs_sync()
#include <lamebus/ltrace.h>
#include "file_syscalls.h"
#include "opt-file.h"
#if OPT_FILE
#include <vnode.h>
#include <proc.h>
#include <limits.h>
#include <kern/errno.h>
struct openfile{
    unsigned int refCount;
    off_t offset;
    struct vnode* vn;  
};

struct openfile systemFileTable[10*OPEN_MAX]; 
#endif

int sys_read(int fd, char *buf, size_t count){
    if(fd!= STDIN_FILENO)
	{
	    return -1;
	} 
    int i; 
    char *b = buf; 
    
    for(i= 0; i<(int)count; i++)
	{
	    *b = getch();
	    if(*buf<0)
		{
		    return i; 
		}
	    b++;
	}
       

    return i; 
}

int sys_write(int fd, const char *buf, size_t count){
 

    if(fd!=STDOUT_FILENO)
	{
	    return -1; 
	} 
  
    int i; 
    for (i = 0; i<(int)count; i++)
	{
	    putch(*buf);
	    buf++; 
	}
   

    return (int)count;

}

int sys_open(userptr_t path, int openflags, mode_t mode, int* errp){
#if OPT_FILE
    struct vnode *vn;
    struct openfile *f=NULL;
   
    int res = vfs_open((char*)path, openflags, mode, &vn);
    if(res<0)
	{
	    return -1;
	}
    for(int i = 0; i<10*OPEN_MAX; i++){
	if(systemFileTable[i].vn==NULL){
	    f = &systemFileTable[i];
	    f->vn = vn;
	    f->refCount=1;
	    f->offset=0;
	    break;
	    
	}
    }
    if(f!=NULL){
	for(int i =STDERR_FILENO+1; i<OPEN_MAX; i++){
	    if(curproc->fileTable[i]==NULL){
		curproc->fileTable[i] = f;
		return i; 
	    }
	}
    }else{
	*errp = EMFILE;
    }
    vfs_close(vn);
    return -1;
    
    
#endif

}
