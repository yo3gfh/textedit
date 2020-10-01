/*
    SPAWN, support (in)utility for TextEdit editor
    ------------------------------------------------
    Copyright (c) 2002-2020 Adrian Petrila, YO3GFH
    
    Console app to "spawn" other processes.

                                * * *
                                
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                * * *

    Features
    ---------

        - spawn.exe is for executing external tools (masm, for example).

*/
#define UNICODE
#ifdef UNICODE
    #define _UNICODE
#endif

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

int _tmain ( int argc, _TCHAR ** argv )
{
    TCHAR               szCmdLine[MAX_PATH];
    int                 ArgIdx;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    BOOL                res;

    if ( argc < 2 )
    {
        _ftprintf ( stdout, 
            TEXT("\nSpawn v1.0, copyright (c) 2002-2020"
                " by Adrian Petrila, YO3GFH\n")
            TEXT("Create and spawn a new process. "
                "Helper app for textedit.exe\n\n")
            TEXT("\tUsage: spawn <program [command line]>\n") );

        return 4;
    }

    RtlZeroMemory ( szCmdLine, sizeof ( szCmdLine ) );

    for ( ArgIdx = 1; ArgIdx < argc; ArgIdx++ )
    {
        lstrcat ( szCmdLine, argv[ArgIdx] ); 
        lstrcat ( szCmdLine, TEXT(" ") );
    }

    RtlZeroMemory ( &si, sizeof ( STARTUPINFO ) );
    si.cb = sizeof ( STARTUPINFO );
    RtlZeroMemory ( &pi, sizeof ( PROCESS_INFORMATION ) );

    res = CreateProcess ( NULL, szCmdLine, NULL, 
        NULL, TRUE, 0, NULL, NULL, &si, &pi );

    if ( res )
    {
        CloseHandle ( pi.hThread );
        WaitForSingleObject ( pi.hProcess, INFINITE );
        CloseHandle ( pi.hProcess );
        return TRUE;
    }

    return FALSE;
}
