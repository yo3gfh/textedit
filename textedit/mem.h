
#ifndef _MEM_H
#define _MEM_H

#include <windows.h>

// grow in 4k chunks
#define ALIGN_4K(x) ((((UINT_PTR)(x))+4096+1)&(~4095)) 

extern void     * alloc_and_zero_mem    ( size_t nbytes );
extern BOOL     free_mem                ( void * buf );

#endif
