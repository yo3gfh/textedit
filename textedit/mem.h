
#ifndef _MEM_H
#define _MEM_H

#include <windows.h>

extern void     * alloc_and_zero_mem    ( size_t nbytes );
extern BOOL     free_mem                ( void * buf );

#endif
