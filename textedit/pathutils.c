
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

// pathutils.c - some file path handling routines

#include    "main.h"
#include    <strsafe.h>

#pragma warn(disable: 2231 2030 2260) //enum not used in switch, = used in conditional, deprecated

TCHAR ** FILE_CommandLineToArgv ( TCHAR * CmdLine, int * _argc )
/*****************************************************************************************************************/
/* Thanks to Alexander A. Telyatnikov                                                                            */
/* http://alter.org.ua/en/docs/win/args/                                                                         */
/* Don't forget to GlobalFree                                                                                    */

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

    if ( CmdLine == NULL || _argc == NULL ) { return NULL; }

    len = lstrlen ( CmdLine );
    i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

    argv = (TCHAR**)GlobalAlloc(GMEM_FIXED, i + (len+2)*sizeof(TCHAR));
    if ( argv == NULL ) { return NULL; }
    _argv = (TCHAR*)(((UCHAR*)argv)+i);

    argc = 0;
    argv[argc] = _argv;
    in_QM = FALSE;
    in_TEXT = FALSE;
    in_SPACE = TRUE;
    i = 0;
    j = 0;

    while( a = CmdLine[i] ) {
        if(in_QM) {
            if(a == TEXT('\"')) {
                in_QM = FALSE;
            } else {
                _argv[j] = a;
                j++;
            }
        } else {
            switch(a) {
            case TEXT('\"'):
                in_QM = TRUE;
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                in_SPACE = FALSE;
                break;
            case TEXT(' '):
            case TEXT('\t'):
            case TEXT('\n'):
            case TEXT('\r'):
                if(in_TEXT) {
                    _argv[j] = TEXT('\0');
                    j++;
                }
                in_TEXT = FALSE;
                in_SPACE = TRUE;
                break;
            default:
                in_TEXT = TRUE;
                if(in_SPACE) {
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
    _argv[j] = TEXT('\0');
    argv[argc] = NULL;

    (*_argc) = argc;
    return argv;
}

const TCHAR * FILE_Extract_ext ( const TCHAR * src )
/*****************************************************************************************************************/
{
    DWORD   idx;
    
    if ( src == NULL ) return NULL;
    idx = lstrlen ( src )-1;

    while ( ( src[idx] != TEXT('.') ) && ( idx != 0 ) )
        idx--;

    if ( idx == 0 ) return NULL;
    return (src+idx);
}

const TCHAR * FILE_Extract_path ( const TCHAR * src, BOOL last_bslash )
/*****************************************************************************************************************/
{
    DWORD           idx;
    static TCHAR    temp[TMAX_PATH+1];

    if ( src == NULL ) return NULL;
    idx = lstrlen ( src )-1;
    if ( idx >= TMAX_PATH ) return NULL;

    while ( ( src[idx] != TEXT('\\') ) && ( idx != 0 ) )
        idx--;

    if ( idx == 0 ) return NULL;
    if ( last_bslash )
        idx++;
    lstrcpyn ( temp, src, idx+1 );
    return temp;
}

const TCHAR * FILE_Extract_full_fname ( const TCHAR * src )
/*****************************************************************************************************************/
{
    DWORD   idx;
    
    if ( src == NULL ) return NULL;
    idx = lstrlen ( src )-1;
    while ( ( src[idx] != TEXT('\\') ) && ( idx != 0 ) )
        idx--;
    if ( idx == 0 ) return NULL;
    return (src+idx+1);
}

const TCHAR * FILE_Extract_filename ( const TCHAR * src )
/*****************************************************************************************************************/
{
    DWORD           idx, len, end;
    static  TCHAR   temp[TMAX_PATH+1];

    if ( src == NULL ) return NULL;
    len = lstrlen ( src );
    idx = len-1;
    end = 0;

    while ( ( src[idx] != TEXT('\\') ) && ( idx != 0 ) )
    {
        if ( src[idx] == TEXT('.') )
            end = idx;
        idx--;
    }
    if ( ( idx == 0 ) || ( ( end-idx ) >= TMAX_PATH ) ) return NULL;
    lstrcpyn ( temp, src+idx+1, end-idx );
    return temp;
}

BOOL FILE_FileExists ( const TCHAR * fname )
/*****************************************************************************************************************/
{
    WIN32_FIND_DATA     wd;
    HANDLE              hf;

    hf = FindFirstFile ( fname, &wd );
    if ( hf == INVALID_HANDLE_VALUE ) return FALSE;
    FindClose ( hf );
    return TRUE;
}

DWORD FILE_GetFileSize ( const TCHAR * filename )
/*****************************************************************************************************************/
/* Get file size                                                                                                 */
{
    HANDLE      hFile;
    DWORD       fsize;

    if ( filename == NULL )
        return -1;

    hFile = CreateFile ( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );

    if ( hFile == INVALID_HANDLE_VALUE )
        return -1;

    fsize = GetFileSize ( hFile, NULL );
    CloseHandle ( hFile );

    return fsize;
}

void FILE_MakeFullIniFname ( TCHAR * fbuf, size_t fbuf_b_size )
/*****************************************************************************************************************/
{
    TCHAR   temp[TMAX_PATH+1];
    DWORD   i, len;

    if ( !GetModuleFileName ( NULL, temp, ARRAYSIZE ( temp ) ) )
        return;

    len = lstrlen ( temp );
    i = 0;

    while ( temp[i] != TEXT('.') && i < len ) i++;
    while ( temp[i] != TEXT('\\') && i > 0 ) i--;
    StringCchCopyN ( fbuf, fbuf_b_size, temp, i+1 );
    StringCchCat ( fbuf, fbuf_b_size, TEXT("textedit.ini") );
}



