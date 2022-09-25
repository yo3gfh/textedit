
// misc.c - amazingly various :-)

//enum not used in switch, = used in conditional, deprecated
#pragma warn(disable: 2008 2118 2228 2231 2030 2260) 

#include "main.h"
#include <windowsx.h>
#include <strsafe.h>
#include <commctrl.h>
#include <versionhelpers.h>
#include <tchar.h>
#include "misc.h"
#include "mem.h"
#include "resource.h"
#include "cfunction.h"
#include "pathutils.h"

#define     MAX_LB_LINE     1024

const char Alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

TCHAR       * opn_filter    = TEXT("Text files (*.txt)\0*.txt\0")
                              TEXT("HTML files (*.htm)\0*.htm\0")
                              TEXT("HTML files (*.html)\0*.html\0")
                              TEXT("XML files (*.xml)\0*.xml\0")
                              TEXT("Style sheets (*.css)\0*.css\0")        
                              TEXT("Batch files (*.bat)\0*.bat\0")
                              TEXT("Batch files (*.cmd)\0*.cmd\0")  
                              TEXT("Ini files (*.ini)\0*.ini\0")
                              TEXT("Nfo files (*.nfo)\0*.nfo\0")
                              TEXT("Diz files (*.diz)\0*.diz\0")
                              TEXT("C sources (*.c)\0*.c\0")
                              TEXT("C++ sources (*.cpp)\0*.cpp\0")
                              TEXT("C/C++ Header files (*.h)\0*.h\0")
                              TEXT("Pascal sources (*.pas)\0*.pas\0")      
                              TEXT("Assembly sources (*.asm)\0*.asm\0")
                              TEXT("Assembly/Pascal includes (*.inc)\0*.inc\0")
                              TEXT("All Files (*.*)\0*.*\0");

TCHAR       * opn_title     = TEXT("Open a text file...");
TCHAR       * sav_title     = TEXT("Save file as...");
TCHAR       * opn_defext    = TEXT("txt");

TCHAR       * app_classname = TEXT("TED_CLASS_666");
TCHAR       * app_classname2 = TEXT("TED_CLASS_6969");
TCHAR       * app_title     = TEXT("TextEdit v2, copyright (c) "
                                    "2002-2020 Adrian Petrila, YO3GFH");

TCHAR       * macro_hlp     = TEXT("N O T E S:\n1). The following macros "
    "will be expanded if found on \"Command line\"\n\t{f} \t"
    "- full path of current file\n\t{F}\t- file name + extension\n\t{n}\t- "
    "file name only\n\t{e}\t- extension only\n\t{p}\t- path only (also valid"
    " on Working dir)\n\t{P}\t- path to editor\n2). Output capturing will not "
    "work for GUI apps.\n3). You can leave \"Working dir\" empty.");

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ShowMessage 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hown     : HWND to parent
//    Param.    2: TCHAR * msg   : text to display
//    Param.    3: DWORD style   : same as for MessageBox
//    Param.    4: DWORD icon_id : an resource ID or 0 to display default
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Custom MessageBox
/*--------------------------------------------------------------------@@-@@-*/
int ShowMessage ( HWND hown, TCHAR * msg, DWORD style, DWORD icon_id )
/*--------------------------------------------------------------------------*/
{
    MSGBOXPARAMS            mp;
    TCHAR                   temp[TMAX_PATH+1];
    size_t                  i, len;

    GetModuleFileName ( NULL, temp, ARRAYSIZE ( temp ) );
    len                     = lstrlen ( temp );
    i                       = 0;

    while ( temp[i] != TEXT('.') && i < len ) i++;
    temp[i]                 = TEXT('\0');
    while ( temp[i] != TEXT('\\') && i > 0 ) i--;

    mp.cbSize               = sizeof ( mp );
    mp.dwStyle              = MB_USERICON | style;
    mp.hInstance            = ( icon_id == 0 ) ? 
        NULL : GetModuleHandle ( NULL );
    mp.lpszIcon             = ( icon_id == 0 ) ? 
        MAKEINTRESOURCE ( IDI_INFORMATION ) : MAKEINTRESOURCE ( icon_id );
    mp.hwndOwner            = hown;
    mp.lpfnMsgBoxCallback   = NULL;
    mp.lpszCaption          = temp+i+1;
    mp.lpszText             = msg;
    mp.dwContextHelpId      = 0;
    mp.dwLanguageId         = MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT );

    return MessageBoxIndirect ( &mp );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: BeginDraw 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hwnd : handle to window
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Mark the beginning of a lengthy operation. Use before some
//                 long updates in a list, edit control etc. to avoid flicker
/*--------------------------------------------------------------------@@-@@-*/
void BeginDraw ( HWND hwnd )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hwnd, WM_SETREDRAW, FALSE, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: EndDraw 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hwnd : HWND to window
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Mark the end of a lengthy operation. Generates a repaint
/*--------------------------------------------------------------------@@-@@-*/
void EndDraw ( HWND hwnd )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hwnd, WM_SETREDRAW, TRUE, 0 );
    InvalidateRect ( hwnd, NULL, TRUE );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: undiv 
/*--------------------------------------------------------------------------*/
//           Type: DWORD 
//    Param.    1: DWORD val    : value to divide
//    Param.    2: DWORD * quot : value to receive quotient
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Unsigned division :-) Returns reminder.
/*--------------------------------------------------------------------@@-@@-*/
DWORD undiv ( DWORD val, DWORD * quot )
/*--------------------------------------------------------------------------*/
{
    ldiv_t d;

    d = ldiv ( val, *quot );

    *quot = d.quot; 
    return d.rem;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: untoa 
/*--------------------------------------------------------------------------*/
//           Type: TCHAR * 
//    Param.    1: DWORD value   : unsigned value to convert
//    Param.    2: TCHAR * buffer: buffer that receives ASCII conversion
//    Param.    3: int radix     : radix in which conversion is done
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Convert a UINT to its ascii representation.
//                 Returnd NULL on some error.
/*--------------------------------------------------------------------@@-@@-*/
TCHAR * untoa ( DWORD value, TCHAR * buffer, int radix )
/*--------------------------------------------------------------------------*/
{
    TCHAR       * p;
    char        * q;
    DWORD       rem;
    DWORD       quot;
    char        buf[64];      // only holds ASCII so 'char' is OK

    if ( buffer == NULL )
        return NULL;

    buffer[0]   = TEXT('\0');
    p           = buffer;
    buf[0]      = '\0';
    q           = &buf[1];

    __try
    {
        do
        {
            quot    = radix;
            rem     = undiv( value, (DWORD *)&quot );
            *q      = Alphabet[rem];
            ++q;
            value   = quot;
        }
        while ( value != 0 );

        while ( *p++ = (TCHAR)*--q ); 
            
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        buffer[0]   = TEXT('\0');
    }

    return buffer;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: intoa 
/*--------------------------------------------------------------------------*/
//           Type: TCHAR * 
//    Param.    1: int value     : int value to convert to ASCII
//    Param.    2: TCHAR * buffer: buffer to receive conversion
//    Param.    3: int radix     : radix for conversion
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Convert a UINT to its ascii representation.
/*--------------------------------------------------------------------@@-@@-*/
TCHAR * intoa ( int value, TCHAR * buffer, int radix )
/*--------------------------------------------------------------------------*/
{
    TCHAR   * p = buffer;

    if ( radix == 10 )
    {
        if ( value < 0 )
        {
            *p++ = TEXT('-');
            value = -value;
        }
    }

    untoa ( value, p, radix );

    return ( buffer );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: topxy 
/*--------------------------------------------------------------------------*/
//           Type: DWORD 
//    Param.    1: DWORD wwidth : window width
//    Param.    2: DWORD swidth : screen width
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Return the width of centered window to screen
/*--------------------------------------------------------------------@@-@@-*/
DWORD topxy ( DWORD wwidth, DWORD swidth )
/*--------------------------------------------------------------------------*/
{
    swidth >>= 1;
    wwidth >>= 1;

    return ( swidth - wwidth );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Parse_OUT_Buf 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: const TCHAR * src: source data
//    Param.    2: PBPROC pbproc    : callback to call on each line
//    Param.    3: LPARAM lParam    : custom data to send to callback
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Reads a line from src and calls pbproc for this line;
//                 Rinse and repeat :)
/*--------------------------------------------------------------------@@-@@-*/
BOOL Parse_OUT_Buf ( const TCHAR * src, PBPROC pbproc, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    DWORD       len, src_idx, dst_idx;
    TCHAR       temp[MAX_LB_LINE];

    if ( src == NULL )
        return FALSE;
        
    len         = lstrlen ( src );
    src_idx     = 0;
    dst_idx     = 0;

    while ( src_idx < len )
    {
        if ( (src[src_idx] == TEXT('\n')) ||
                (src[src_idx] == TEXT('\r')) ||
                    (dst_idx >= MAX_LB_LINE-1) )
        {
            temp[dst_idx] = TEXT('\0');

            if ( !pbproc ( temp, lParam ) )
                break;

            dst_idx = 0;

            while ( (src[src_idx] == TEXT('\n')) ||
                (src[src_idx] == TEXT('\r')) )
                    src_idx++;
        }
        else
        {
            if ( src[src_idx] != TEXT('\t'))
                temp[dst_idx] = src[src_idx];

            src_idx++;
            dst_idx++;
        }
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Parse_OUT_Buf_Proc 
/*--------------------------------------------------------------------------*/
//           Type: LRESULT CALLBACK 
//    Param.    1: const TCHAR * s: text line as prepared by Parse_OUT_Buf
//    Param.    2: LPARAM lParam  : custom data received from Parse_OUT_Buf
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Small callback for Parse_OUT_Buf; adds the received line
//                 of text to the listbox with the HWND received as lParam
/*--------------------------------------------------------------------@@-@@-*/
LRESULT CALLBACK Parse_OUT_Buf_Proc ( const TCHAR * s, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    INT_PTR     res;

    if ( s == NULL ) 
        return FALSE;

    res = SendMessage ( (HWND)lParam, LB_ADDSTRING,
                        (WPARAM)0, (LPARAM)(LPCTSTR)s );

    if ( (res == LB_ERR) || (res == LB_ERRSPACE) )
        return FALSE;

    return TRUE;
}

// optimizer with -Ot -Ox breaks this function
// #pragma optimize(none)
// solved by declaring buffPtr a volatile pointer

/*-@@+@@--------------------------------------------------------------------*/
//       Function: HexDump 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: const TCHAR * src      : source data
//    Param.    2: DWORD dwsrclen         : data length (bytes)
//    Param.    3: HEXDUMPPROC hexdumpproc: callback to call when having a
//                                          line of formatted output ready
//    Param.    4: LPARAM lParam          : custom data to send to hexdumpproc
/*--------------------------------------------------------------------------*/
//         AUTHOR: Matt Pietrek, small changes by Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Do a hexdump of src
/*--------------------------------------------------------------------@@-@@-*/
void HexDump ( const TCHAR * src, DWORD dwsrclen,
    HEXDUMPPROC hexdumpproc, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR       buffer[MAX_LB_LINE];
    TCHAR       * volatile buffPtr;
    TCHAR       * buffPtr2;
    DWORD       cOutput, i, rightlimit, bytesToGo;
    TCHAR       value;
    size_t      remaining;

    bytesToGo   = dwsrclen;
    rightlimit  = ( HEX_DUMP_WIDTH * 3 ) + ( HEX_DUMP_WIDTH >> 2 );

    while ( bytesToGo  )
    {
        cOutput = ( bytesToGo >= HEX_DUMP_WIDTH ) ? HEX_DUMP_WIDTH : bytesToGo;

        buffPtr = buffer;
        StringCchPrintfEx ( buffPtr, ARRAYSIZE(buffer), 
                            (TCHAR **)&buffPtr, (size_t *)&remaining, 
                            STRSAFE_FILL_ON_FAILURE|STRSAFE_NULL_ON_FAILURE, 
                            TEXT("%08X:  "), 
                            dwsrclen - bytesToGo );

        buffPtr2 = buffPtr + rightlimit;
        
        for ( i = 0; i < HEX_DUMP_WIDTH; i++ )
        {
            value = *( src + i );
            //value = src[i];

            if ( i >= cOutput )
            {
                *buffPtr++ = TEXT(' ');
                *buffPtr++ = TEXT(' ');
                *buffPtr++ = TEXT(' ');
            }
            else
            {
                if ( value < 0x10 )
                {
                    *buffPtr++ = TEXT('0');
                    intoa ( value, buffPtr++, 16 );
                }
                else
                {
                    intoa ( value, buffPtr, 16 );
                    buffPtr+=2;
                }
                *buffPtr++  = TEXT(' ');
                *buffPtr2++ = isprint ( value ) ? value : TEXT('.');
            }

            if ( (( i + 1 ) & 3) == 0 )
                *buffPtr++ = TEXT(' ');
        }

        *buffPtr2 = TEXT('\0');  // Null terminate it.

        if ( !hexdumpproc ( buffer, lParam ) )
            break;

        bytesToGo -= cOutput;
        src += HEX_DUMP_WIDTH;
    }
}
// #pragma optimize()

/*-@@+@@--------------------------------------------------------------------*/
//       Function: HexDumpProc 
/*--------------------------------------------------------------------------*/
//           Type: BOOL CALLBACK 
//    Param.    1: const TCHAR * line: a line of formatted output from HexDump
//    Param.    2: LPARAM lParam     : custom data from HexDump
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Small callback to update a listbox with lines of text from
//                 HexDump
/*--------------------------------------------------------------------@@-@@-*/
BOOL CALLBACK HexDumpProc ( const TCHAR * line, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    INT_PTR     res;

    if ( line == NULL )
        return FALSE;

    res = SendMessage ( (HWND)lParam, LB_ADDSTRING,
                        (WPARAM)0, (LPARAM)(TCHAR *)line );

    if ( (res == LB_ERR) || (res == LB_ERRSPACE) )
        return FALSE;

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: PB_NewProgressBar 
/*--------------------------------------------------------------------------*/
//           Type: HWND 
//    Param.    1: HWND hParent: parent window
//    Param.    2: DWORD left  : x coord
//    Param.    3: DWORD width : width of the p. bar
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Create a progressbar control on parent hParent. Created
//                 initially to show status when opening a large file, 
//                 this is now unused, might as well comment it out
/*--------------------------------------------------------------------@@-@@-*/
HWND PB_NewProgressBar ( HWND hParent, DWORD left, DWORD width )
/*--------------------------------------------------------------------------*/
{
    RECT        r;
    HINSTANCE   hInst;

    hInst       = GetModuleHandle ( NULL );

    GetClientRect ( hParent, &r );

    return CreateWindowEx ( 0, PROGRESS_CLASS, NULL, 
                            WS_CHILD | WS_VISIBLE, left, 
                            r.bottom - 15, 
                            width, 13, 
                            hParent, 0, hInst, NULL);
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ExecAndCapture 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: TCHAR * cmdline  : command line for the new process
//    Param.    2: TCHAR * workindir: working dir for the new process
//                                    (can be NULL)
//    Param.    3: PBPROC pbproc    : callback to exec when having a line
//                                    of text ready
//    Param.    4: LPARAM lParam    : optional extra data for pbproc
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: TLDR: Creates a console process, captures it's output.
//                 A mumbo-jumbo of pipes, handles and processes. If you don't
//                 understand much, I don't blame you :)... Amazing :-)))
//                 Don't use it to exec GUI apps =))
/*--------------------------------------------------------------------@@-@@-*/
BOOL ExecAndCapture ( TCHAR * cmdline, TCHAR * workindir,
    PBPROC pbproc, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;
    SECURITY_ATTRIBUTES     sa;

    DWORD                   nRead, dwExitcode;
    #ifdef UNICODE
    WCHAR                   cBuf[MAX_LB_LINE]; // for the really long lines :-)
    UINT                    wchars;
    #endif
    char                    aBuf[MAX_LB_LINE]; // for the really long lines :-)
    char                    chB[1];
    UINT                    ccB;
    BOOL                    user_break;

    HANDLE                  hOutputReadTmp,
                            hOutputRead,
                            hOutputWrite,
                            hInputWriteTmp,
                            hInputRead,
                            hInputWrite,
                            hErrorWrite;

    RtlZeroMemory ( &pi, sizeof ( PROCESS_INFORMATION ) );
    RtlZeroMemory ( &sa, sizeof ( SECURITY_ATTRIBUTES ) );

    sa.nLength              = sizeof ( SECURITY_ATTRIBUTES );
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = TRUE;

    if ( !CreatePipe ( &hOutputReadTmp, &hOutputWrite, &sa, 0 ) )
        return FALSE;

    if ( !DuplicateHandle ( GetCurrentProcess(),
                            hOutputWrite,
                            GetCurrentProcess(),
                            &hErrorWrite, 0, TRUE,
                            DUPLICATE_SAME_ACCESS
                          ) )
    {
        CloseHandle ( hOutputReadTmp );
        CloseHandle ( hOutputWrite );
        return FALSE;
    }

    if ( !CreatePipe ( &hInputRead, &hInputWriteTmp, &sa, 0 ) )
    {
        CloseHandle ( hOutputReadTmp );
        CloseHandle ( hOutputWrite );
        CloseHandle ( hErrorWrite );
        return FALSE;
    }

    if ( !DuplicateHandle ( GetCurrentProcess(),
                            hOutputReadTmp,
                            GetCurrentProcess(),
                            &hOutputRead, 0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS
                          ) )
    {
        CloseHandle ( hOutputReadTmp );
        CloseHandle ( hInputWriteTmp );
        CloseHandle ( hOutputWrite );
        CloseHandle ( hErrorWrite );
        CloseHandle ( hInputRead );
        return FALSE;
    }

    if ( !DuplicateHandle ( GetCurrentProcess(),
                            hInputWriteTmp,
                            GetCurrentProcess(),
                            &hInputWrite, 0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS
                          ) )
    {
        CloseHandle ( hOutputReadTmp );
        CloseHandle ( hInputWriteTmp );
        CloseHandle ( hOutputRead );
        CloseHandle ( hOutputWrite );
        CloseHandle ( hErrorWrite );
        CloseHandle ( hInputRead );
        return FALSE;
    }
        
    CloseHandle ( hOutputReadTmp );
    CloseHandle ( hInputWriteTmp );

    RtlZeroMemory ( &si, sizeof ( STARTUPINFO ) );
    si.cb           = sizeof ( STARTUPINFO );
    si.dwFlags      = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput    = hInputRead;
    si.hStdOutput   = hOutputWrite;
    si.hStdError    = hErrorWrite;

    if ( !CreateProcess ( NULL, (TCHAR *)cmdline,
                          &sa, &sa,
                          TRUE, 0, NULL, workindir,
                          &si, &pi ) )
    {
        CloseHandle ( hOutputRead );
        CloseHandle ( hOutputWrite );
        CloseHandle ( hErrorWrite );
        CloseHandle ( hInputRead );
        CloseHandle ( hInputWrite );
        return FALSE;
    }
        
    CloseHandle ( hOutputWrite );
    CloseHandle ( hInputRead );
    CloseHandle ( hErrorWrite );

    ccB         = 0;
    user_break  = FALSE;

    while ( 1 )
    {
        if ( !ReadFile ( hOutputRead, chB,
                sizeof(chB), &nRead, NULL ) || ( nRead == 0 ) )
        {
            if ( GetLastError() == ERROR_BROKEN_PIPE )
                break;
            else
            {                
                CloseHandle ( hOutputRead );
                CloseHandle ( hInputWrite );
                CloseHandle ( pi.hThread );
                CloseHandle ( pi.hProcess );
                return FALSE;
            }
        }
        // finish the buf if we're on cr/lf or at the max. length
        if ( chB[0] == '\n' || chB[0] == '\r' || ccB >= ARRAYSIZE(aBuf)-1 )
        {
            aBuf[ccB] = '\0';
        }
        else
        {
            if ( chB[0] != '\t' )
                aBuf[ccB++] = chB[0];

            continue;

        }

        #ifdef UNICODE
            wchars = MultiByteToWideChar ( CP_UTF8, 0, 
                (LPCSTR)aBuf, -1, NULL, 0 );

            MultiByteToWideChar ( CP_UTF8, 0,
                (LPCSTR)aBuf, -1, (LPWSTR)cBuf,
                    (wchars > MAX_PATH) ? MAX_PATH : wchars );

            if ( !pbproc ( cBuf, lParam ) )
            {
                user_break = TRUE;
                break;
            }
        #else
            if ( !pbproc ( aBuf, lParam ) )
            {
                user_break = TRUE;
                break;
            }
        #endif
        ccB = 0;
    }

    dwExitcode = 0;
    GetExitCodeProcess ( pi.hProcess, &dwExitcode );

    CloseHandle ( hOutputRead );
    CloseHandle ( hInputWrite );
    CloseHandle ( pi.hThread );
    CloseHandle ( pi.hProcess );

    // spawn.exe return 1 on success, 4 on helpscreen and 0 on failure
    // if using something else, change the values here accordingly
    if  ( dwExitcode == 1 || dwExitcode == 4 || user_break == TRUE )    
        return TRUE;                                                    

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SimpleExec 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: TCHAR * cmdline         : cmdline for CreateProcess
//    Param.    2: const TCHAR * workindir : working dir (can be NULL)
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: A (hopefully) better WinExec :-)) Creates a new process
/*--------------------------------------------------------------------@@-@@-*/
BOOL SimpleExec ( TCHAR * cmdline, const TCHAR * workindir )
/*--------------------------------------------------------------------------*/
{
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;

    RtlZeroMemory ( &si, sizeof ( STARTUPINFO ) );
    RtlZeroMemory ( &pi, sizeof ( PROCESS_INFORMATION ) );

    si.cb           = sizeof ( STARTUPINFO );
    si.dwFlags      = STARTF_USESHOWWINDOW;
    si.wShowWindow  = SW_SHOWNORMAL;

    if ( !CreateProcess ( NULL, cmdline, NULL, NULL, TRUE,
            0, NULL, workindir, &si, &pi ) )
                return FALSE;

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: GetDefaultBrowser 
/*--------------------------------------------------------------------------*/
//           Type: TCHAR * 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Get the full path to the system def. browser binary.
//                 Not that straightforward, unfortunately =)
/*--------------------------------------------------------------------@@-@@-*/
TCHAR * GetDefaultBrowser ( void )
/*--------------------------------------------------------------------------*/
{
    static TCHAR    buf[TMAX_PATH+1];
    DWORD           result, size;
    HKEY            key;

    buf[0]          = TEXT('\0');

    if ( IsWindows7OrGreater() )
    {
        result = RegOpenKeyEx ( HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\Windows\\Shell\\Associations"
                "\\UrlAssociations\\http\\UserChoice"), 0,
                    KEY_QUERY_VALUE, &key );

        if ( result != ERROR_SUCCESS )
        {
            buf[0]  = TEXT('\0');
            return buf;
        }

        size        = sizeof (buf);
        result      = RegQueryValueEx ( key, TEXT("Progid"),
                                        0, 0, (LPBYTE)buf, &size );

        if ( result != ERROR_SUCCESS )
        {
            RegCloseKey ( key );
            buf[0]  = TEXT('\0');
            return buf;
        }

        RegCloseKey ( key );
        key         = NULL;
        StringCchCat ( buf, ARRAYSIZE(buf), TEXT("\\shell\\open\\command") );
        result      = RegOpenKeyEx ( HKEY_CLASSES_ROOT,
                        buf, 0, KEY_QUERY_VALUE, &key );

        if ( result != ERROR_SUCCESS )
        {
            buf[0]  = TEXT('\0');
            return buf;
        }
    }
    else //we're on XP
    {
        result      = RegOpenKeyEx ( HKEY_CURRENT_USER,
            TEXT("Software\\Classes\\http\\shell\\open\\command"), 0,
                KEY_QUERY_VALUE, &key );

        if ( result != ERROR_SUCCESS )
        {
            buf[0]  = TEXT('\0');
            return buf;
        }
    }

    size            = sizeof (buf);
    result          = RegQueryValueEx ( key, 0, 0, 0, (LPBYTE)buf, &size );

    if ( result != ERROR_SUCCESS )
    {
        RegCloseKey ( key );
        buf[0]  = TEXT('\0');
        return buf;
    } // all this work in vain :-)

    size /= sizeof (TCHAR);
    
    while ( buf[--size] != TEXT('.') && size > 0 ) {;}
    // get over exe and null terminate, keeping the quote
    buf[size+5] = TEXT('\0'); 
    RegCloseKey ( key );

    return buf;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CopyListToClipboard 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hParent: parent to listbox, for OpenClipboard
//    Param.    2: HWND hList  : listbox to copy from
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Copies all items from a listbox to clipboard,
//                 newline separated
/*--------------------------------------------------------------------@@-@@-*/
BOOL CopyListToClipboard ( HWND hParent, HWND hList )
/*--------------------------------------------------------------------------*/
{
    HGLOBAL     hmem;
    TCHAR       * buf, item[CF_MAX_LEN];
    INT_PTR     i, list_items, size;
    DWORD       cf_flags;

    #ifdef UNICODE
    cf_flags    = CF_UNICODETEXT;
    #else
    cf_flags    = CF_TEXT;
    #endif
    list_items  = ListBox_GetCount ( hList );
    
    if ( list_items == LB_ERR || list_items == 0 )
        return FALSE;

    size = 0;

    // calculate how much mem. we need
    // for every line, add 8 bytes to spare
    for ( i = 0; i < list_items; i++ )
        size += ( SendMessage ( hList, LB_GETTEXTLEN, i, 0) + 8 );

    size        *= sizeof(TCHAR);                    
    // allocate in 4k chunks
    // alloc memory, movable and 0 init (clipboard wants it this way)
    hmem        = GlobalAlloc ( GHND, ALIGN_4K(size) );                  

    if ( hmem == NULL )
        return FALSE;

    //when movable flag set, hmem is NOT a pointer to alloc mem
    buf         = (TCHAR *)GlobalLock ( hmem );
    
    for ( i = 0; i < list_items; i++ )
    {
        if ( ListBox_GetText ( hList, i, item ) == LB_ERR )
        {
            GlobalUnlock ( hmem );
            GlobalFree ( hmem );
            return FALSE;                       // bail out
        }
        StringCchCat ( buf, size, item );
        StringCchCat ( buf, size, TEXT("\n") );
    }

    GlobalUnlock ( hmem );                      // done, unlock mem

    if ( !OpenClipboard ( hParent ) )
    {
        GlobalFree ( hmem );
        return FALSE;                           // all this work in vain lol
    }

    EmptyClipboard();

    if ( !SetClipboardData ( cf_flags, hmem ) ) // give hmem to clipboard,
                                                // don't GlobalFree :-)
    {
        GlobalFree ( hmem );
        CloseClipboard();
        return FALSE;
    }

    CloseClipboard();
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CopyListSelToClipboard 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hParent: parent to listbox, for OpenClipboard
//    Param.    2: HWND hList  : lb. to copy from
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Copies all selected items from listbox hList to clipboard
/*--------------------------------------------------------------------@@-@@-*/
BOOL CopyListSelToClipboard ( HWND hParent, HWND hList )
/*--------------------------------------------------------------------------*/
{
    HGLOBAL     hmem;
    TCHAR       * buf, item[CF_MAX_LEN];
    int         sel_items[256];
    INT_PTR     i, list_items, sel, size;
    DWORD       cf_flags;

    #ifdef UNICODE
    cf_flags    = CF_UNICODETEXT;
    #else
    cf_flags    = CF_TEXT;
    #endif
    list_items  = ListBox_GetCount ( hList );
    sel         = ListBox_GetSelCount ( hList );
    
    if ( list_items == LB_ERR || list_items == 0 || sel == LB_ERR || sel == 0 )
        return FALSE;

    // all items selected, go to known path :)
    if ( sel == list_items )                                   
        return CopyListToClipboard ( hParent, hList );

    sel = ListBox_GetSelItems ( hList, (WPARAM)256, (LPARAM)sel_items );

    if ( sel == LB_ERR )
        return FALSE;

    size = 0;

    for ( i = 0; i < sel; i++ )
        size += ListBox_GetTextLen ( hList, (WPARAM)sel_items[i] );

    size        *= sizeof(TCHAR);
    // allocate in 4k chunks                
    // alloc memory, movable and 0 init (clipboard wants it this way)
    hmem        = GlobalAlloc ( GHND, ALIGN_4K(size) );

    if ( hmem == NULL )
        return FALSE;

    //when movable flag set, hmem is NOT a pointer to alloc mem
    buf         = (TCHAR *)GlobalLock ( hmem );                 
    
    for ( i = 0; i < sel; i++ )
    {
        if ( ListBox_GetText ( hList, (WPARAM)sel_items[i],
                (LPARAM)item ) == LB_ERR )
        {
            GlobalUnlock ( hmem );
            GlobalFree ( hmem );
            return FALSE;                       //bail out
        }

        StringCchCat ( buf, size, item );
        StringCchCat ( buf, size, TEXT("\n") );
    }

    GlobalUnlock ( hmem );                      //done, unlock mem

    if ( !OpenClipboard ( hParent ) )
    {
        GlobalFree ( hmem );
        return FALSE;                           //all this work in vain lol
    }

    EmptyClipboard();

    //give hmem to clipboard, don't GlobalFree :-)
    if ( !SetClipboardData ( cf_flags, hmem ) )
    {
        GlobalFree ( hmem );
        CloseClipboard();
        return FALSE;
    }

    CloseClipboard();
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CenterDlg 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hDlg    : dlg to center :)
//    Param.    2: HWND hParent : parent to center in relation to
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Centers dlg hDlg with respect to parent hParent
/*--------------------------------------------------------------------@@-@@-*/
BOOL CenterDlg ( HWND hDlg, HWND hParent )
/*--------------------------------------------------------------------------*/
{
    RECT    rdlg, rparent;
    int     w, h, left, top;
    
    GetWindowRect ( hParent, &rparent );
    GetWindowRect ( hDlg, &rdlg );

    w       = rdlg.right-rdlg.left;
    h       = rdlg.bottom-rdlg.top;
    left    = rparent.left+(((rparent.right-rparent.left)-w)>>1);
    top     = rparent.top+(((rparent.bottom-rparent.top)-h)>>1);

    return MoveWindow ( hDlg, left, top, w, h, FALSE );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: IsThereAnotherInstance 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: const TCHAR * classname : window class to search for
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Searches for a window with the windowclass "classname"
/*--------------------------------------------------------------------@@-@@-*/
BOOL IsThereAnotherInstance ( const TCHAR * classname )
/*--------------------------------------------------------------------------*/
{
    return ( FindWindow ( classname, NULL ) != NULL );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Today 
/*--------------------------------------------------------------------------*/
//           Type: TCHAR * 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: A song by Smashing Pumpkins 8-)
/*--------------------------------------------------------------------@@-@@-*/
TCHAR * Today ( void )
/*--------------------------------------------------------------------------*/
{
    static TCHAR    buf[128];
    SYSTEMTIME      st;

    buf[0]          = TEXT('\0');
    GetLocalTime ( &st );

    StringCchPrintf ( buf, ARRAYSIZE(buf),
        TEXT("%02u.%02u.%u"), st.wDay, st.wMonth, st.wYear );

    return buf;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Now 
/*--------------------------------------------------------------------------*/
//           Type: TCHAR * 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns current time as hh:mm:ss:msmsms
/*--------------------------------------------------------------------@@-@@-*/
TCHAR * Now ( void )
/*--------------------------------------------------------------------------*/
{
    static TCHAR    buf[128];
    SYSTEMTIME      st;

    buf[0]          = TEXT('\0');
    GetLocalTime ( &st );

    StringCchPrintf ( buf, ARRAYSIZE(buf),
        TEXT("%02u:%02u:%02u:%03u"), st.wHour, st.wMinute,
            st.wSecond, st.wMilliseconds );

    return buf;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: tok_times 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: TCHAR * str       : string to examine
//    Param.    2: const TCHAR * tok : token to search for in the form of {x}
//                                  where x is the macro letter (f,F,e,n,p,P)
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns # of occurences of tok in str. Support for
//                 all_tok_times and ExpandMacro
/*--------------------------------------------------------------------@@-@@-*/
int tok_times ( TCHAR * str, const TCHAR * tok )
/*--------------------------------------------------------------------------*/
{
    TCHAR     * p;
    int       count, len;

    if ( tok == NULL || str == NULL )
        return -1;

    p       = str;
    count   = 0;
    len     = lstrlen ( tok );

    while ( ( p = _tcsstr ( p, tok ) ) != NULL )
    {
        p += len;
        count++;
    }

    return count;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: all_tok_times 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: TCHAR * str       : string to examine
//    Param.    2: const TCHAR * tok : NULL term. string with all macros to
//                 search for, without the enclosing {} ("fFenpP" for example)
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
int all_tok_times ( TCHAR * str, const TCHAR * tok )
/*--------------------------------------------------------------------------*/
{
    TCHAR   temp[8];
    size_t  i, len, count;
    
    if ( tok == NULL || str == NULL )
        return -1;

    len     = lstrlen (tok);
    count   = 0;

    for ( i = 0; i < len; i++ )
    {
        temp[0] = L'{';
        temp[1] = tok[i];
        temp[2] = L'}';
        temp[3] = L'\0';

        count += tok_times ( str, temp );
    }

    return count;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ExpandMacro 
/*--------------------------------------------------------------------------*/
//           Type: TCHAR * 
//    Param.    1: const TCHAR * src      : input string to expand
//    Param.    2: const TCHAR * filespec : full path to file
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns a pointer to a dynamically allocated TCHAR string
//                 which has all {x} macros expanded to their coresponding 
//                 value: 
//                         {f} = full path
//                         {F} = f. name + ext.
//                         {n} = f. name
//                         {e} = ext.
//                         {p} = path
//                         {P} = editor path
//                 Don't forget to free_mem the returned TCHAR * !
/*--------------------------------------------------------------------@@-@@-*/
TCHAR * ExpandMacro ( const TCHAR * src, const TCHAR * filespec )
/*--------------------------------------------------------------------------*/
{
    TCHAR       * final, * fbits;
    TCHAR       macro;
    TCHAR       temp[TMAX_PATH];
    INT_PTR     size, alloc_size, src_idx, dst_idx;
    int         len;

    if ( ( src == NULL ) || ( filespec == NULL ) )
        return NULL;

    len = lstrlen ( src );

    if ( len < 3 ) 
        return (TCHAR *)src;

    size        = 1;
    size        += all_tok_times ( (TCHAR *)src, TEXT("fFnepP") );
    size        *= TMAX_PATH;
    alloc_size  = size * sizeof(TCHAR);
    final       = alloc_and_zero_mem ( ALIGN_4K(alloc_size) );

    if ( final == NULL )
        return NULL;

    src_idx     = 0;
    dst_idx     = 0;
    
    while ( src[src_idx] )
    {
        if ( src[src_idx] != TEXT('{') )
            final[dst_idx] = src[src_idx];
        else
        {
            if ( ( len-src_idx > 2 ) && ( src[src_idx+2] == TEXT('}') ) )
            {
                macro = src[src_idx+1];
                switch ( macro )
                {
                    case TEXT('f'):
                        StringCchCat ( final, size, filespec );
                        break;

                    case TEXT('F'):
                        fbits = (TCHAR *)FILE_Extract_full_fname ( filespec );

                        if ( fbits != NULL )
                            StringCchCat ( final, size, fbits );

                        break;

                    case TEXT('n'):
                        fbits = (TCHAR *)FILE_Extract_filename ( filespec );

                        if ( fbits != NULL )
                            StringCchCat ( final, size, fbits );

                        break;

                    case TEXT('e'):
                        fbits = (TCHAR *)FILE_Extract_ext ( filespec );

                        if ( fbits != NULL )
                            StringCchCat ( final, size, fbits );

                        break;

                    case TEXT('p'):
                        fbits = (TCHAR *)FILE_Extract_path ( filespec, TRUE );

                        if ( fbits != NULL )
                            StringCchCat ( final, size, fbits );

                        break;

                    case TEXT('P'):
                        GetModuleFileName ( NULL, temp, ARRAYSIZE(temp) );
                        fbits = (TCHAR *)FILE_Extract_path ( temp, TRUE );

                        if ( fbits != NULL )
                            StringCchCat ( final, size, fbits );

                        break;

                    default:
                        final[dst_idx] = TEXT('{');
                        src_idx++;
                        dst_idx++;
                        continue;
                }
                dst_idx = lstrlen ( final )-1;
                src_idx += 2;
            }
            else
            {
                final[dst_idx] = TEXT('{');
                src_idx++;
                dst_idx++;
                continue;
            }
        }

        src_idx++;
        dst_idx++;
    }

    return final;
}

#ifdef HAVE_HTMLHELP
/*-@@+@@--------------------------------------------------------------------*/
//       Function: ChmFromModule 
/*--------------------------------------------------------------------------*/
//           Type: TCHAR * 
//    Param.    1: HMODULE hMod : module handle
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 27.05.2021
//    DESCRIPTION: Make a full path to a CHM help file from the running exe
/*--------------------------------------------------------------------@@-@@-*/
TCHAR * ChmFromModule ( HMODULE hMod )
/*--------------------------------------------------------------------------*/
{
    static TCHAR    buf[TMAX_PATH];
    TCHAR           * p;

    buf[0]  = TEXT('\0');
    p       = buf;

    if ( GetModuleFileName ( hMod, buf, ARRAYSIZE(buf) ) )
    {
        while ( *p != TEXT('.') && *p != TEXT('\0') )
            p++;
        
        if ( *p == TEXT('\0') )
            return buf;

        *(p+1) = TEXT('c');
        *(p+2) = TEXT('h');
        *(p+3) = TEXT('m');
        *(p+4) = TEXT('\0');
    }
    
    return buf;
}
#endif

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FileVersionAsString 
/*--------------------------------------------------------------------------*/
//           Type: TCHAR * 
//    Param.    1: const TCHAR * fpath : full path to file to retrieve
//                                       version for
/*--------------------------------------------------------------------------*/
//         AUTHOR: stackoverflow, Adrian Petrila, YO3GFH
//           DATE: 25.09.2022
//    DESCRIPTION: returns file version as a NULL terminated string 
//                 (e.g. 1.0.0.2) or empty string on error.
/*--------------------------------------------------------------------@@-@@-*/
TCHAR * FileVersionAsString ( const TCHAR * fpath )
/*--------------------------------------------------------------------------*/
{
    static TCHAR        buf[TMAX_PATH];
    const TCHAR         * work_path;
    DWORD               ver_datasize;
    void                * pverdata;
    BYTE                * pbyte;
    UINT                size;
    VS_FIXEDFILEINFO    * ver_info;

    work_path = fpath;

    if ( fpath == NULL ) 
    {
        GetModuleFileName ( NULL, buf, ARRAYSIZE(buf));
        work_path = buf;
    }
    
    ver_datasize = GetFileVersionInfoSize ( work_path, NULL );

    if ( ver_datasize )
    {
        pverdata = alloc_and_zero_mem ( ver_datasize );
        if ( pverdata )
        {
            if ( GetFileVersionInfo ( work_path, 0, ver_datasize, pverdata ) )
            {
                if ( VerQueryValue ( pverdata, TEXT("\\"), 
                        (void**)&pbyte, &size ) )
                {
                    if ( size )
                    {
                        ver_info = (VS_FIXEDFILEINFO *)pbyte;
                        if ( ver_info->dwSignature == 0xfeef04bd )
                        {
                            StringCchPrintf ( buf, ARRAYSIZE(buf), 
                              TEXT("%d.%d.%d.%d"), 
                              ( ver_info->dwFileVersionMS >> 16 ) & 0xffff, 
                              ( ver_info->dwFileVersionMS >> 0 ) & 0xffff, 
                              ( ver_info->dwFileVersionLS >> 16 ) & 0xffff, 
                              ( ver_info->dwFileVersionLS >> 0 ) & 0xffff );
                            
                            free_mem ( pverdata );
                            return buf;
                        }
                    }
                }
            }

            free_mem ( pverdata );
        }
    }

    buf[0] = TEXT('\0');
    return buf;
}
