
#ifndef _PATHUTILS_H
#define _PATHUTILS_H

#include <windows.h>

extern BOOL             FILE_FileExists             ( const TCHAR * fname );
extern const TCHAR      * FILE_Extract_path         
    ( const TCHAR * src, BOOL last_bslash );

extern const TCHAR      * FILE_Extract_ext          ( const TCHAR * src );
extern const TCHAR      * FILE_Extract_full_fname   ( const TCHAR * src );
extern const TCHAR      * FILE_Extract_filename     ( const TCHAR * src );
extern DWORD            FILE_GetFileSize            ( const TCHAR * filename );

extern TCHAR            ** FILE_CommandLineToArgv   
    ( TCHAR * CmdLine, int * _argc );

#endif
