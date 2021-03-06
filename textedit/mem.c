
// mem.c - memory alloc/free routines
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include "main.h"

/*-@@+@@--------------------------------------------------------------------*/
//       Function: alloc_and_zero_mem 
/*--------------------------------------------------------------------------*/
//           Type: void * 
//    Param.    1: size_t nbytes : how much to alloc
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Allocates nbytes of fixed memory and zero init.
//                 Saves mem. handle returned by LocalAlloc in the first DWORD
//                 and returns a pointer  to the next DWORD as memory start.
/*--------------------------------------------------------------------@@-@@-*/
void * alloc_and_zero_mem ( size_t nbytes )
/*--------------------------------------------------------------------------*/
{
    HGLOBAL     hmem;
    void        * buf;

    hmem = LocalAlloc ( LPTR, nbytes + sizeof(hmem) );

    if ( hmem == NULL )
        return NULL;

    buf = hmem;
    *( ( HGLOBAL * )buf ) = hmem;

    return ( void *)( ( ( HGLOBAL *)( buf ) ) + 1 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: free_mem 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: void * buf : address to prev. allocated mem.
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Frees the memory prev. allocated with alloc_and_zero_mem.
//                 Goes back a DWORD from the given address, to retrieve the
//                 stored handle from there.
/*--------------------------------------------------------------------@@-@@-*/
BOOL free_mem ( void * buf )
/*--------------------------------------------------------------------------*/
{
    HGLOBAL     hmem;

    if ( buf == NULL )
        return FALSE;

    hmem = *( ( ( HGLOBAL *) buf ) - 1 );
    return ( LocalFree ( hmem ) == NULL );
}
