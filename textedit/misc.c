
/*
    TEXTEDIT, a (somehow) small multi-file text editor with UNICODE support
    -----------------------------------------------------------------------
    Copyright (c) 2002-2020 Adrian Petrila, YO3GFH
    
    Started this because, at the time, I thought Notepad was crap and how
    hard can it be to write something better, like PFE (Programmers File Editor)?
    Well, after breaking my teeth in UNICODE, turned out that Notepad was not all
    that crappy and my kung-fu was weak.

    So welcome to more crap :-))
    
    Some random thoughts:
    ---------------------
    Originally buit with OpenWatcom, moved it to the Pelle's C compiler system
    http://www.smorgasbordet.com/pellesc/index.htm

    After working with Richedit, I can now fully understand why ppl go all the way
    and write their own custom edit controls.

    After working with the comctl tab control, I can see why OOP is good =)
    After trying to find a way to resize parent+child windows flicker-free,
    I just want to be alone and cry :-)))

    But I'm not all that good, anyway lol.

    Hope you find something useful in this pile of text ( printing and unicode streaming
    comes to mind ), doesnt't seem to have too many bugs (here's hoping lol) and, for
    me at least, gets the job done.

    And the usual disclaimer: this isn't cutting edge, elegant, NSA grade code.
    Not even my grandpa-grade, if I come to think (he was a vet, God rest his soul,
    he couldn't write code but he could make a cow sing).
    I don't work as a full-time programmer (used to, 20 years ago) so I can
    very well afford to be incompetent :-)))
    
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
    
        - uses Richedit v4.1 to open/edit/save text in ANSI and UNICODE
        (UTF8/16/32-LE/BE with/without BOM). You cand use Richedit 3.0/2.0
        if you want (undef HAVE_MSFTEDIT), but not 1.0 (does not support UNICODE).
        It works on win7, 8 and 10, should work on xp sp3 too. Please note that 
        it does not use features specific to latest Richedit versions (7.5 on W8
        and 8.5 on W10).

        - Multiple file support (tabbed).

        - Execute external programs, optionally capturing console output
        (you may use it as a crude IDE).

        - View current selection as HEX.

        - Unfortunately, it supports drag-n-drop

        - Exciting, misterious and surprisingly powerful bugs which will crash
        your box =) These are 100% genuine and are all mine!

    Building
    --------

        - open the workspace editor.ppw and you'll have all three projects (textedit,
        spawn and texeditSH) which you can build all at once. You can also open the 
        individual projects (*.ppj) from each project folder.

        - spawn.exe is for executing external tools (masm, for example); texteditSH
        is a MS Explorer context menu shell extension to open files in TextEdit; 
        the program works without either of these.
*/

// misc.c - amazingly various :-)
#pragma warn(disable: 2008 2118 2228 2231 2030 2260) //enum not used in switch, = used in conditional, deprecated

#include "main.h"
#include "misc.h"
#include "mem.h"
#include "resource.h"
#include  <windowsx.h>
#include  <strsafe.h>
#include  <commctrl.h>
#include  <versionhelpers.h>



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
TCHAR       * app_title     = TEXT("TextEdit v2.0, copyright (c) 2002-2020 Adrian Petrila, YO3GFH");

int ShowMessage ( HWND hown, TCHAR * msg, DWORD style, DWORD icon_id )
/*****************************************************************************************************************/
{
    MSGBOXPARAMS    mp;
    TCHAR           temp[TMAX_PATH+1];
    int             i, len;

    GetModuleFileName ( NULL, temp, ARRAYSIZE ( temp ) );
    len = lstrlen ( temp );
    i = 0;

    while ( temp[i] != TEXT('.') && i < len ) i++;
    temp[i] = TEXT('\0');
    while ( temp[i] != TEXT('\\') && i > 0 ) i--;

    mp.cbSize               = sizeof ( mp );
    mp.dwStyle              = MB_USERICON | style;
    mp.hInstance            = ( icon_id == 0 ) ? NULL : GetModuleHandle ( NULL );
    mp.lpszIcon             = ( icon_id == 0 ) ? MAKEINTRESOURCE ( IDI_INFORMATION ) : MAKEINTRESOURCE ( icon_id );
    mp.hwndOwner            = hown;
    mp.lpfnMsgBoxCallback   = NULL;
    mp.lpszCaption          = temp+i+1;
    mp.lpszText             = msg;
    mp.dwContextHelpId      = 0;
    mp.dwLanguageId         = MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT );
    return MessageBoxIndirect ( &mp );
}

void BeginDraw ( HWND hwnd )
/*****************************************************************************************************************/
{
    SendMessage ( hwnd, WM_SETREDRAW, FALSE, 0 );
}

void EndDraw ( HWND hwnd )
/*****************************************************************************************************************/
{
    SendMessage ( hwnd, WM_SETREDRAW, TRUE, 0 );
    InvalidateRect ( hwnd, NULL, TRUE );
}

DWORD undiv ( DWORD val, DWORD * quot )
/*****************************************************************************************************************/
{
    ldiv_t d;

    d = ldiv ( val, *quot );

    *quot = d.quot; 
    return d.rem;
}

TCHAR * untoa ( DWORD value, TCHAR * buffer, int radix )
/*****************************************************************************************************************/
{
    TCHAR   * p;
    char    * q;
    DWORD   rem;
    DWORD   quot;
    char    buf[64];      // only holds ASCII so 'char' is OK

    p = buffer;
    buf[0] = '\0';
    q = &buf[1];
    do
    {
        quot = radix;
        rem = undiv( value, (DWORD *)&quot );
        *q = Alphabet[rem];
        ++q;
        value = quot;
    }
    while( value != 0 );
    while( *p++ = (TCHAR)*--q );
    return( buffer );
}

TCHAR * intoa ( int value, TCHAR * buffer, int radix )
/*****************************************************************************************************************/
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

DWORD topxy ( DWORD wwidth, DWORD swidth )
/*****************************************************************************************************************/
{
    swidth >>= 1;
    wwidth >>= 1;
    return ( swidth - wwidth );
}

BOOL Parse_OUT_Buf ( const TCHAR * src, PBPROC pbproc, LPARAM lParam )
/*****************************************************************************************************************/
{
    DWORD   len, src_idx, dst_idx;
    TCHAR   * temp;
    void    * buf;

    if ( src == NULL ) { return FALSE; }
        
    buf = alloc_and_zero_mem ( 4096 );

    if ( buf == NULL ) { return FALSE; }

    temp = (TCHAR *)buf;    
    len = lstrlen ( src );
    src_idx = 0;
    dst_idx = 0;
    while ( src_idx < len )
    {
        if ( (src[src_idx] == TEXT('\n')) || (src[src_idx] == TEXT('\r')) )
        {
            temp[dst_idx] = TEXT('\0');
            if ( !pbproc ( temp, lParam ) ) break;
            dst_idx = 0;
            while ( (src[src_idx] == TEXT('\n')) || (src[src_idx] == TEXT('\r')) )
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
    free_mem ( buf );
    return TRUE;
}

LRESULT CALLBACK Parse_OUT_Buf_Proc ( const TCHAR * s, LPARAM lParam )
/*****************************************************************************************************************/
{
    if ( s == NULL ) return FALSE;
    SendMessage ( (HWND)lParam, LB_ADDSTRING, (WPARAM)0, (LPARAM)(LPCTSTR)s );
    return TRUE;
}

void HexDump ( const TCHAR * src, DWORD dwsrclen, HEXDUMPPROC hexdumpproc, LPARAM lParam )
/*****************************************************************************************************************/
{
    TCHAR   buffer[512];
    TCHAR   * buffPtr, * buffPtr2;
    DWORD   cOutput, i, rightlimit, bytesToGo;
    TCHAR   value;
    size_t  remaining;

    bytesToGo   = dwsrclen;
    rightlimit  = ( HEX_DUMP_WIDTH * 3 ) + ( HEX_DUMP_WIDTH >> 2 );

    while ( bytesToGo  )
    {
        cOutput = ( bytesToGo >= HEX_DUMP_WIDTH ) ? HEX_DUMP_WIDTH : bytesToGo;

        buffPtr = buffer;
        StringCchPrintfEx ( buffPtr, ARRAYSIZE(buffer), 
                            &buffPtr, &remaining, 
                            STRSAFE_FILL_ON_FAILURE|STRSAFE_NULL_ON_FAILURE, 
                            TEXT("%08X:  "), 
                            dwsrclen - bytesToGo );

        buffPtr2 = buffPtr + rightlimit;
        
        for ( i = 0; i < HEX_DUMP_WIDTH; i++ )
        {
            value = *( src + i );

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
                *buffPtr++ = TEXT(' ');
                *buffPtr2++ = isprint ( value ) ? value : TEXT('.');
            }
            if ( ( ( i + 1 ) & 3 ) == 0 )
                *buffPtr++ = TEXT(' ');
        }

        *buffPtr2 = TEXT('\0');  // Null terminate it.
        if ( !hexdumpproc ( buffer, lParam ) )
            break;

        bytesToGo -= cOutput;
        src += HEX_DUMP_WIDTH;
    }
}

BOOL CALLBACK HexDumpProc ( const TCHAR * line, LPARAM lParam )
/*****************************************************************************************************************/
{
    SendMessage ( (HWND)lParam, LB_ADDSTRING, (WPARAM)0, (LPARAM)(TCHAR *)line );
    return TRUE;
}

HWND PB_NewProgressBar ( HWND hParent, DWORD left, DWORD width )
/*****************************************************************************************************************/
{
    RECT        r;
    HINSTANCE   hInst;

    hInst = GetModuleHandle ( NULL );

    GetClientRect ( hParent, &r );
    return CreateWindowEx ( 0, PROGRESS_CLASS, NULL, 
        WS_CHILD | WS_VISIBLE, left, 
        r.bottom - 15, 
        width, 13, 
        hParent, 0, hInst, NULL);
}

BOOL ExecAndCapture ( TCHAR * cmdline, TCHAR * workindir, TCHAR * buf, size_t buf_size )
/*****************************************************************************************************************/
{
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;
    SECURITY_ATTRIBUTES     sa;

    DWORD                   nRead;
    #ifdef UNICODE
    WCHAR                   cbuf[MAX_PATH];
    int                     wchars;
    #endif
    char                    aBuf[MAX_PATH];

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

    buf[0] = TEXT('\0');
    while ( 1 )
    {
        if ( !ReadFile ( hOutputRead, aBuf, sizeof(aBuf)-1, &nRead, NULL ) || ( nRead == 0 ) )
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
        aBuf[nRead] = 0;
        #ifdef UNICODE
        wchars = MultiByteToWideChar ( CP_UTF8, 0, (LPCSTR)aBuf, -1, NULL, 0 );
        MultiByteToWideChar ( CP_UTF8, 0, (LPCSTR)aBuf, -1, (LPWSTR)cbuf, (wchars > MAX_PATH) ? MAX_PATH : wchars );
        StringCchCat ( buf, buf_size, cbuf );
        #else
        StringCchCat ( buf, buf_size, aBuf );
        #endif
    }

    CloseHandle ( hOutputRead );
    CloseHandle ( hInputWrite );
    CloseHandle ( pi.hThread );
    CloseHandle ( pi.hProcess );

    return TRUE;
}

BOOL SimpleExec ( TCHAR * cmdline, const TCHAR * workindir )
/*****************************************************************************************************************/
{
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;

    RtlZeroMemory ( &si, sizeof ( STARTUPINFO ) );
    RtlZeroMemory ( &pi, sizeof ( PROCESS_INFORMATION ) );

    si.cb           = sizeof ( STARTUPINFO );
    si.dwFlags      = STARTF_USESHOWWINDOW;
    si.wShowWindow  = SW_SHOWNORMAL;

    if ( !CreateProcess ( NULL, cmdline, NULL, NULL, TRUE, 0, NULL, workindir, &si, &pi ) )
    {
        return FALSE;
    }

    return TRUE;
}

TCHAR * GetDefaultBrowser ( void )
/*****************************************************************************************************************/
{
    static TCHAR    buf[TMAX_PATH+1];
    DWORD           result, size;
    HKEY            key;

    if ( IsWindows7OrGreater() )
    {
        result = RegOpenKeyEx ( HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice"), 0, KEY_QUERY_VALUE, &key );

        if ( result != ERROR_SUCCESS ) { return NULL; }

        size = sizeof (buf);
        result = RegQueryValueEx ( key, TEXT("Progid"), 0, 0, (LPBYTE)buf, &size );

        if ( result != ERROR_SUCCESS ) { RegCloseKey ( key ); return NULL; }

        RegCloseKey ( key );
        key = NULL;
        StringCchCat ( buf, sizeof(buf), TEXT("\\shell\\open\\command") );
        result = RegOpenKeyEx ( HKEY_CLASSES_ROOT, buf, 0, KEY_QUERY_VALUE, &key );

        if ( result != ERROR_SUCCESS ) { return NULL; }
    }
    else //we're on XP
    {
        result = RegOpenKeyEx ( HKEY_CURRENT_USER, TEXT("Software\\Classes\\http\\shell\\open\\command"), 0, KEY_QUERY_VALUE, &key );

        if ( result != ERROR_SUCCESS ) { return NULL; }
    }

    size = sizeof (buf);
    result = RegQueryValueEx ( key, 0, 0, 0, (LPBYTE)buf, &size );

    if ( result != ERROR_SUCCESS ) { RegCloseKey ( key ); return NULL; } // all this work in vain :-)

    size /= sizeof (TCHAR);
    
    while ( buf[--size] != TEXT('.') && size > 0 ) {;}
    buf[size+5] = TEXT('\0'); // get over exe and null terminate, keeping the quote
    RegCloseKey ( key );
    return buf;
}

BOOL CopyListToClipboard ( HWND hparent, HWND hList )
/*****************************************************************************************************************/
{
    HGLOBAL     hmem;
    TCHAR       * buf, item[255];
    INT_PTR     i, list_items, size;
    DWORD       cf_flags;

#ifdef UNICODE
    cf_flags = CF_UNICODETEXT;
#else
    cf_flags = CF_TEXT;
#endif
    list_items = ListBox_GetCount ( hList );
    if ( list_items == LB_ERR || list_items == 0 ) { return FALSE; }

    size = 0;

    for ( i = 0; i < list_items; i++ )                  //calculate how much mem. we need
        size += ( SendMessage ( hList, LB_GETTEXTLEN, i, 0) + 8 ); // for every line, add 8 bytes to spare

    size *= sizeof(TCHAR);

    hmem = GlobalAlloc ( GHND, size );                  //alloc memory, movable and 0 init (clipboard wants it this way)
    if ( hmem == NULL ) { return FALSE; }

    buf = (TCHAR *)GlobalLock ( hmem );                 //when movable flag set, hmem is NOT a pointer to alloc mem
    
    for ( i = 0; i < list_items; i++ )
    {
        if ( ListBox_GetText ( hList, i, item ) == LB_ERR )
        {
            GlobalUnlock ( hmem );
            GlobalFree ( hmem );
            return FALSE;                               //bail out
        }
        StringCchCat ( buf, size, item );
        StringCchCat ( buf, size, TEXT("\n") );
    }

    GlobalUnlock ( hmem );                              //done, unlock mem

    if ( !OpenClipboard ( hparent ) )
    {
        GlobalFree ( hmem );
        return FALSE;                                   //all this work in vain lol
    }

    EmptyClipboard();

    if ( !SetClipboardData ( cf_flags, hmem ) )         //give hmem to clipboard, don't GlobalFree :-)
    {
        GlobalFree ( hmem );
        CloseClipboard();
        return FALSE;
    }

    CloseClipboard();
    return TRUE;
}

BOOL CenterDlg ( HWND hdlg, HWND hparent )
/*****************************************************************************************************************/
{
    RECT    rdlg, rparent;
    int     w, h, left, top;
    
    GetWindowRect ( hparent, &rparent );
    GetWindowRect ( hdlg, &rdlg );
    w       = rdlg.right-rdlg.left;
    h       = rdlg.bottom-rdlg.top;
    left    = rparent.left+(((rparent.right-rparent.left)-w)>>1);
    top     = rparent.top+(((rparent.bottom-rparent.top)-h)>>1);
    return MoveWindow ( hdlg, left, top, w, h, FALSE );
}

BOOL IsThereAnotherInstance ( const TCHAR * classname )
/*****************************************************************************************************************/
{
    return ( FindWindow ( classname, NULL ) != NULL );
}
