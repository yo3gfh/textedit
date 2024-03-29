
// pathutils.c - some file path handling routines

//enum not used in switch, = used in conditional, deprecated
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include    "main.h"
#include    <strsafe.h>

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FILE_CommandLineToArgv
/*--------------------------------------------------------------------------*/
//           Type: TCHAR **
//    Param.    1: TCHAR * CmdLine: pointer to app commandline
//    Param.    2: int * _argc    : pointer to var to receive arg. count
/*--------------------------------------------------------------------------*/
//         AUTHOR: Thanks to Alexander A. Telyatnikov,
//                 http://alter.org.ua/en/docs/win/args/
//           DATE: 28.09.2020
//    DESCRIPTION: Nice little function! Takes a command line and breaks it
//                 into null term. strings (_argv) indexed by an array of
//                 pointers (argv). All necessary mem. is allocated
//                 dynamically, _argv and argv being stored one after the
//                 other. It returns the pointers table, which you must free
//                 with GlobalFree after getting the job done.
/*--------------------------------------------------------------------@@-@@-*/
TCHAR ** FILE_CommandLineToArgv ( TCHAR * CmdLine, int * _argc )
/*--------------------------------------------------------------------------*/
{
    TCHAR       ** argv;
    TCHAR       *  _argv;
    ULONG       len;
    ULONG       argc;
    TCHAR       a;
    ULONG       i, j;

    BOOLEAN     in_QM;
    BOOLEAN     in_TEXT;
    BOOLEAN     in_SPACE;

    if ( CmdLine == NULL || _argc == NULL )
        return NULL;

    len         = lstrlen ( CmdLine );
    i           = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

    argv        = (TCHAR**)GlobalAlloc(GMEM_FIXED,
                    (i + (len+2)*sizeof(TCHAR) + 4096 + 1) & (~4095) );

    if ( argv == NULL )
        return NULL;

    _argv       = (TCHAR*)(((UCHAR*)argv)+i);

    argc        = 0;
    argv[argc]  = _argv;
    in_QM       = FALSE;
    in_TEXT     = FALSE;
    in_SPACE    = TRUE;
    i           = 0;
    j           = 0;

    while ( a = CmdLine[i] )
    {
        if ( in_QM )
        {
            if ( a == TEXT('\"') )
            {
                in_QM = FALSE;
            }
            else
            {
                _argv[j] = a;
                j++;
            }
        }
        else
        {
            switch ( a )
            {
                case TEXT('\"'):
                    in_QM   = TRUE;
                    in_TEXT = TRUE;

                    if ( in_SPACE )
                    {
                        argv[argc] = _argv+j;
                        argc++;
                    }

                    in_SPACE = FALSE;
                    break;

                case TEXT(' '):
                case TEXT('\t'):
                case TEXT('\n'):
                case TEXT('\r'):

                    if ( in_TEXT )
                    {
                        _argv[j] = TEXT('\0');
                        j++;
                    }

                    in_TEXT  = FALSE;
                    in_SPACE = TRUE;
                    break;

                default:
                    in_TEXT  = TRUE;

                    if ( in_SPACE )
                    {
                        argv[argc] = _argv+j;
                        argc++;
                    }

                    _argv[j] = a;
                    j++;
                    in_SPACE = FALSE;
                    break;
            }
        }

        i++;
    }

    _argv[j]    = TEXT('\0');
    argv[argc]  = NULL;

    (*_argc)    = argc;

    return argv;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FILE_Extract_ext
/*--------------------------------------------------------------------------*/
//           Type: const TCHAR *
//    Param.    1: const TCHAR * src : hopefully, a good filename :)
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns a pointer to the file extension in src,
//                 or NULL on err.
/*--------------------------------------------------------------------@@-@@-*/
const TCHAR * FILE_Extract_ext ( const TCHAR * src )
/*--------------------------------------------------------------------------*/
{
    DWORD   idx;

    if ( src == NULL )
        return NULL;

    idx = lstrlen ( src )-1;

    while ( ( src[idx] != TEXT('.') ) && ( idx != 0 ) )
        idx--;

    if ( idx == 0 )
        return NULL;

    return ( src+idx );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FILE_Extract_path
/*--------------------------------------------------------------------------*/
//           Type: const TCHAR *
//    Param.    1: const TCHAR * src: a full path
//    Param.    2: BOOL last_bslash : wether to add the last '\' or not
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns a filepath, with or without the last backslash
/*--------------------------------------------------------------------@@-@@-*/
const TCHAR * FILE_Extract_path ( const TCHAR * src, BOOL last_bslash )
/*--------------------------------------------------------------------------*/
{
    DWORD           idx;
    static TCHAR    temp[TMAX_PATH+1];

    if ( src == NULL )
        return NULL;

    idx = lstrlen ( src )-1;

    if ( idx >= TMAX_PATH )
        return NULL;

    while ( ( src[idx] != TEXT('\\') ) && ( idx != 0 ) )
        idx--;

    if ( idx == 0 )
        return NULL;

    if ( last_bslash )
        idx++;

    lstrcpyn ( temp, src, idx+1 );

    return temp;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FILE_Extract_full_fname
/*--------------------------------------------------------------------------*/
//           Type: const TCHAR *
//    Param.    1: const TCHAR * src : full path
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Extract the filename+ext
/*--------------------------------------------------------------------@@-@@-*/
const TCHAR * FILE_Extract_full_fname ( const TCHAR * src )
/*--------------------------------------------------------------------------*/
{
    DWORD   idx;

    if ( src == NULL )
        return NULL;

    idx = lstrlen ( src )-1;

    while ( ( src[idx] != TEXT('\\') ) && ( idx != 0 ) )
        idx--;

    if ( idx == 0 )
        return NULL;

    return (src+idx+1);
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FILE_Extract_filename
/*--------------------------------------------------------------------------*/
//           Type: const TCHAR *
//    Param.    1: const TCHAR * src : full path
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Extract filename, no ext.
/*--------------------------------------------------------------------@@-@@-*/
const TCHAR * FILE_Extract_filename ( const TCHAR * src )
/*--------------------------------------------------------------------------*/
{
    DWORD           idx, len, end;
    static  TCHAR   temp[TMAX_PATH+1];

    if ( src == NULL )
        return NULL;

    len             = lstrlen ( src );
    idx             = len-1;
    end             = 0;

    while ( ( src[idx] != TEXT('\\') ) && ( idx != 0 ) )
    {
        if ( src[idx] == TEXT('.') )
            end = idx;

        idx--;
    }

    if ( ( idx == 0 ) || ( ( end-idx ) >= TMAX_PATH ) )
        return NULL;

    lstrcpyn ( temp, src+idx+1, end-idx );

    return temp;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FILE_FileExists
/*--------------------------------------------------------------------------*/
//           Type: BOOL
//    Param.    1: const TCHAR * fname : full path to file
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Is it or it isn't?
/*--------------------------------------------------------------------@@-@@-*/
BOOL FILE_FileExists ( const TCHAR * fname )
/*--------------------------------------------------------------------------*/
{
    WIN32_FIND_DATA     wd;
    HANDLE              hf;

    hf = FindFirstFile ( fname, &wd );

    if ( hf == INVALID_HANDLE_VALUE )
        return FALSE;

    FindClose ( hf );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FILE_GetFileSize
/*--------------------------------------------------------------------------*/
//           Type: DWORD
//    Param.    1: const TCHAR * filename : full path to file
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Get a file's size
/*--------------------------------------------------------------------@@-@@-*/
DWORD FILE_GetFileSize ( const TCHAR * filename )
/*--------------------------------------------------------------------------*/
{
    HANDLE      hFile;
    DWORD       fsize;

    if ( filename == NULL )
        return (DWORD)-1;

    hFile = CreateFile ( filename, GENERIC_READ,
                            FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );

    if ( hFile == INVALID_HANDLE_VALUE )
        return (DWORD)-1;

    fsize = GetFileSize ( hFile, NULL );
    CloseHandle ( hFile );

    return fsize;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FILE_MakeFullIniFname
/*--------------------------------------------------------------------------*/
//           Type: BOOL
//    Param.    1: TCHAR * fbuf       : where to put the result
//    Param.    2: size_t fbuf_b_size : fbuf size, in chars
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Make a full ini filename from the dir. where app the
//                 started. Superseeded by IniFromModule from config.c
/*--------------------------------------------------------------------@@-@@-*/
//BOOL FILE_MakeFullIniFname ( TCHAR * fbuf, size_t fbuf_b_size )
/*--------------------------------------------------------------------------*/
/*{
    TCHAR   temp[TMAX_PATH+1];
    size_t  i, len;

    if ( !GetModuleFileName ( NULL, temp, ARRAYSIZE ( temp ) ) )
        return FALSE;

    len = lstrlen ( temp );
    i = 0;

    while ( temp[i] != TEXT('.') && i < len ) i++;
    while ( temp[i] != TEXT('\\') && i > 0 ) i--;
    StringCchCopyN ( fbuf, fbuf_b_size, temp, i+1 );
    StringCchCat ( fbuf, fbuf_b_size, TEXT("textedit.ini") );

    return TRUE;
}*/
