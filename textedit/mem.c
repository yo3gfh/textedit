
// mem.c - memory alloc/free routines
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include <windows.h>

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
//                 and returns a pointer to the next DWORD as memory start.
/*--------------------------------------------------------------------@@-@@-*/
void * alloc_and_zero_mem ( size_t nbytes )
/*--------------------------------------------------------------------------*/
{
    HGLOBAL     hmem;
    void        * buf;

    // LPTR is LMEM_ZEROINIT|LMEM_FIXED
    hmem = LocalAlloc ( LPTR, nbytes + sizeof(HGLOBAL) );

    if ( hmem == NULL )
        return NULL;

    buf = hmem;
    *(( HGLOBAL * )buf ) = hmem;

    return ( void *)((( HGLOBAL *)( buf )) + 1 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: realloc_and_zero_mem 
/*--------------------------------------------------------------------------*/
//           Type: void * 
//    Param.    1: void * buf    : existing memory to enlarge :-)
//                               : !! NOTE !! this mem have to be prev.
//                               : alloc with alloc_and_zero_mem!
//    Param.    2: size_t mbytes : new size of the mem. block, in bytes
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 12.09.2022
//    DESCRIPTION: very much alike alloc_and_zero_mem above.
//                 thanks to Raymond Chen and his post on the url below:
//          https://stackoverflow.com/questions/16244133/localrealloc-failing 
/*--------------------------------------------------------------------@@-@@-*/
void * realloc_and_zero_mem ( void * buf, size_t mbytes )
/*--------------------------------------------------------------------------*/
{
    HGLOBAL     hmem, h_newmem;
    void        * new_buf;

    if ( buf == NULL || mbytes == 0 )
        return NULL;

    hmem = *((( HGLOBAL *) buf ) - 1 );

    // we alloc'd fixed mem. previously, but we pass LMEM_MOVEABLE
    // to allow mem relocation, otherwise realloc WILL fail sooner 
    // or later. Tnx. Raymond Chen, see function header
    h_newmem = LocalReAlloc ( hmem, mbytes + sizeof(HGLOBAL), 
                LMEM_MOVEABLE | LMEM_ZEROINIT );

    if ( h_newmem == NULL )
        return NULL;

    new_buf = h_newmem;
    *(( HGLOBAL * )new_buf ) = h_newmem;

    return ( void *)((( HGLOBAL *)( new_buf )) + 1 );
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

    hmem = *((( HGLOBAL *) buf ) - 1 );
    return ( LocalFree ( hmem ) == NULL );
}
