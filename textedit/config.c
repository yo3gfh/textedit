
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

// config.c - various config saving/loading routines

#pragma warn(disable: 2008 2118 2228 2231 2030 2260) //enum not used in switch, = used in conditional, deprecated

#include "main.h"
#include <strsafe.h>

// maximum number of combo box items to save to ini
#define         MAX_HIST_ITEMS      50



/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: LoadFontFromInifile 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: HFONT 
//                 Param.    1: const TCHAR * inifile : full path to config
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 13.09.2020
//                 DESCRIPTION: This will try and read a LOGFONT struct as prev. saved in the 
//                              config file. If ok, will return a HFONT dor the newly created font
/*--------------------------------------------------------------------------------------------@@-@@-*/
HFONT LoadFontFromInifile ( const TCHAR * inifile )
/*--------------------------------------------------------------------------------------------------*/
{
    LOGFONT     lf;

    if ( ( inifile == NULL ) ||
        ( !GetPrivateProfileStruct ( TEXT("editor"), TEXT("log_font"), &lf, sizeof ( lf ), inifile ) ) ) 
        { return NULL; }

    return CreateFontIndirect ( &lf );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: SaveWindowPosition 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: void 
//                 Param.    1: HWND hwnd             : window handle
//                 Param.    2: const TCHAR * inifile : full path to config
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 13.09.2020
//                 DESCRIPTION: Saves a window position and state to a ini file
/*--------------------------------------------------------------------------------------------@@-@@-*/
void SaveWindowPosition ( HWND hwnd, const TCHAR * inifile )
/*--------------------------------------------------------------------------------------------------*/
{
    WINDOWPLACEMENT wp;
    RECT            r;

    wp.length = sizeof ( wp );
    GetWindowPlacement ( hwnd, &wp );

    if ( wp.showCmd == SW_SHOWMAXIMIZED )
        WritePrivateProfileString ( TEXT("window"), TEXT("maximized"), TEXT("1"), inifile );
    else
    {
        WritePrivateProfileString ( TEXT("window"), TEXT("maximized"), TEXT("0"), inifile );
        if ( wp.showCmd == SW_SHOWNORMAL )
        {
            if ( GetWindowRect ( hwnd, &r ) )
                WritePrivateProfileStruct ( TEXT("window"), TEXT("position"), &r, sizeof ( r ), inifile );
        }
    }
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: LoadWindowPosition 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: BOOL 
//                 Param.    1: RECT * r              : pointer to RECT that will receive coord.
//                 Param.    2: const TCHAR * inifile : full path to config
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 13.09.2020
//                 DESCRIPTION: Load window coord. form a ini file
//
/*--------------------------------------------------------------------------------------------@@-@@-*/
BOOL LoadWindowPosition ( RECT * r, const TCHAR * inifile )
/*--------------------------------------------------------------------------------------------------*/
{
    return GetPrivateProfileStruct ( TEXT("window"), TEXT("position"), r, sizeof ( RECT ), inifile );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: SaveDlgPosition 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: void 
//                 Param.    1: HWND hwnd             : dlg. handle
//                 Param.    2: const TCHAR * inifile : full path to config
//                 Param.    3: const TCHAR * section : ini section to save in
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 13.09.2020
//                 DESCRIPTION: Save a dlg. window position to a ini file
/*--------------------------------------------------------------------------------------------@@-@@-*/
void SaveDlgPosition ( HWND hwnd, const TCHAR * inifile, const TCHAR * section )
/*--------------------------------------------------------------------------------------------------*/
{
    RECT            r;

    if ( GetWindowRect ( hwnd, &r ) )
        WritePrivateProfileStruct ( section, TEXT("position"), &r, sizeof ( r ), inifile );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: LoadDlgPosition 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: BOOL 
//                 Param.    1: RECT * r              : pointer to RECT for received coord.
//                 Param.    2: const TCHAR * inifile : full path to config
//                 Param.    3: const TCHAR * section : ini section to read from
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 13.09.2020
//                 DESCRIPTION: Read dlg. window placement from config file. TRUE on success,
//                              FALSE otherwise
/*--------------------------------------------------------------------------------------------@@-@@-*/
BOOL LoadDlgPosition ( RECT * r, const TCHAR * inifile, const TCHAR * section )
/*--------------------------------------------------------------------------------------------------*/
{
    return GetPrivateProfileStruct ( section, TEXT("position"), r, sizeof ( RECT ), inifile );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: LoadCBBHistoryFromINI 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: BOOL 
//                 Param.    1: HWND hDlg               : dlg. window handle
//                 Param.    2: int cbbid               : cb. box control ID
//                 Param.    3: const TCHAR * inisection: ini section to read from
//                 Param.    4: const TCHAR * inifile   : full path to config
//                 Param.    5: BOOL restoresel         : restore the last selected item?
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 13.09.2020
//                 DESCRIPTION: Load and populate a combo box from an ini file. TRUE on success,
//                              FALSE otherwise.
/*--------------------------------------------------------------------------------------------@@-@@-*/
BOOL LoadCBBHistoryFromINI ( HWND hDlg, int cbbid, const TCHAR * inisection, const TCHAR * inifile, BOOL restoresel )
/*--------------------------------------------------------------------------------------------------*/
{
    TCHAR       temp[TMAX_PATH];
    TCHAR       item_txt[TMAX_PATH];
    HWND        hCbb;
    INT_PTR     hist_count, hist_idx,
                hist_crtidx;

    hist_count = GetPrivateProfileInt ( inisection, TEXT("count"), 0, inifile );
    hist_crtidx = GetPrivateProfileInt ( inisection, TEXT("crtsel"), CB_ERR, inifile );

    if ( hist_count != 0 )
    {
        hCbb = GetDlgItem ( hDlg, cbbid );

        for ( hist_idx = 0; hist_idx < hist_count; hist_idx++ )
        {
            StringCchPrintf ( temp, ARRAYSIZE(temp), TEXT("item%zd"), hist_idx );
            GetPrivateProfileString ( inisection, temp, TEXT("<EMPTY>"), item_txt, ARRAYSIZE(item_txt), inifile );
            SendMessage ( hCbb, CB_ADDSTRING, 0, ( LPARAM )( LPCTSTR )item_txt );
        }

        if ( restoresel )
        {
            hist_crtidx = ( hist_crtidx == CB_ERR ) ? ( (hist_idx-1) ) : hist_crtidx;
            SendMessage ( hCbb, CB_SETCURSEL, ( WPARAM )(hist_crtidx), 0 );
        }

        return TRUE;
    }

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: SaveCBBHistoryToINI 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: BOOL 
//                 Param.    1: HWND hDlg               : dlgbox containing the cbbox
//                 Param.    2: int cbbid               : cbbox dlg ID
//                 Param.    3: const TCHAR * inisection: section where to put the keys
//                 Param.    4: const TCHAR * inifile   : full path to config
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: algernon77
//                        DATE: 11.09.2020
//                 DESCRIPTION: Save the contents of a combo box to an ini file
/*--------------------------------------------------------------------------------------------@@-@@-*/
BOOL SaveCBBHistoryToINI ( HWND hDlg, int cbbid, const TCHAR * inisection, const TCHAR * inifile )
/*--------------------------------------------------------------------------------------------------*/
{
    TCHAR       temp[TMAX_PATH];
    TCHAR       item_txt[TMAX_PATH];
    HWND        hCbb;
    INT_PTR     hist_count, hist_idx,
                hist_crtidx;

    hCbb = GetDlgItem ( hDlg, cbbid );
    if ( GetWindowTextLength ( hCbb ) != 0 )
    {
        GetDlgItemText ( hDlg, cbbid, item_txt, ARRAYSIZE ( item_txt ) );

        if ( SendMessage ( hCbb, CB_FINDSTRINGEXACT, (WPARAM)-1, ( LPARAM )( LPCTSTR )item_txt ) == CB_ERR )
            SendMessage ( hCbb, CB_ADDSTRING, 0, ( LPARAM )( LPCTSTR )item_txt );

        hist_count = SendMessage ( hCbb, CB_GETCOUNT, 0, 0 );

        if ( hist_count > MAX_HIST_ITEMS )
        {
            for ( hist_idx = 0; hist_idx < ( hist_count-MAX_HIST_ITEMS ); hist_idx++ )
                SendMessage ( hCbb, CB_DELETESTRING, (WPARAM)hist_idx, 0 );

            hist_count = SendMessage ( hCbb, CB_GETCOUNT, 0, 0 );
        }

        if ( ( hist_count != 0 ) && ( hist_count != CB_ERR ) )
        {
            StringCchPrintf ( temp, ARRAYSIZE(temp), TEXT("%zd"), hist_count );
            WritePrivateProfileString ( inisection, TEXT("count"), temp, inifile );
            //hist_crtidx = SendMessage ( hCbb, LB_GETCURSEL , 0, 0 ); // --> this, for some strange reason, does not work, always returns 0...
            hist_crtidx = SendDlgItemMessage ( hDlg , cbbid, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0 );

            if ( hist_crtidx >= 0 )
            {
                StringCchPrintf ( temp, ARRAYSIZE(temp), TEXT("%zd"), hist_crtidx );
                WritePrivateProfileString ( inisection, TEXT("crtsel"), temp, inifile );
            }

            for ( hist_idx = 0; hist_idx < hist_count; hist_idx++ )
            {
                StringCchPrintf ( temp, ARRAYSIZE(temp), TEXT("item%zd"), hist_idx );
                SendMessage ( hCbb, CB_GETLBTEXT, ( WPARAM )hist_idx, ( LPARAM )( LPCTSTR )item_txt );
                WritePrivateProfileString ( inisection, temp, item_txt, inifile );
            }
        }

        return TRUE;
    }

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: IniFromModule 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: TCHAR * 
//                 Param.    1: HMODULE hMod : calling module mod. handle
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 13.09.2020
//                 DESCRIPTION: Build a full config filename from the calling module name. On
//                              error, first char from returned buf is NULL.
/*--------------------------------------------------------------------------------------------@@-@@-*/
TCHAR * IniFromModule ( HMODULE hMod )
/*--------------------------------------------------------------------------------------------------*/
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

        *(p+1) = TEXT('i');
        *(p+2) = TEXT('n');
        *(p+3) = TEXT('i');
        *(p+4) = TEXT('\0');
    }
    
    return buf;
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: ReadAuthorFromIni 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: TCHAR * 
//                 Param.    1: const TCHAR * inifile : full path to config file
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 13.09.2020
//                 DESCRIPTION: read author name (for CF_CommentCFunction)
/*--------------------------------------------------------------------------------------------@@-@@-*/
TCHAR * ReadAuthorFromIni ( const TCHAR * inifile )
/*--------------------------------------------------------------------------------------------------*/
{
    static TCHAR    buf[TMAX_PATH];

    GetPrivateProfileString ( TEXT("editor"), TEXT("author"), TEXT("<bugmeister>"), buf, ARRAYSIZE(buf), inifile );
    return buf;
}

