
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

//sbar.c - the humble status bar 
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include "main.h"
#include "sbar.h"



/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: SB_MakeStatusBar 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: HWND 
//                 Param.    1: HWND hParent: parent to SB
//                 Param.    2: int sbid    : unique control ID
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Let's make a StatusBar!
/*--------------------------------------------------------------------------------------------@@-@@-*/
HWND SB_MakeStatusBar ( HWND hParent, int sbid )
/*--------------------------------------------------------------------------------------------------*/
{
    HWND        hstatus;
    int         sbparts[4];
    ULONG_PTR   style;

    hstatus = CreateWindowEx ( WS_EX_COMPOSITED, STATUSCLASSNAME, NULL, //WS_EX_COMPOSITED, for flicker :-)
                                WS_CHILDWINDOW|WS_VISIBLE|SBS_SIZEGRIP, 
                                0, 0, 0, 0, hParent, (HMENU)sbid, GetModuleHandle(NULL), NULL );

    if ( !hstatus )
        return NULL;
    
    sbparts[0] = 100;
    sbparts[1] = 150;//150
    sbparts[2] = 300;
    sbparts[3] = -1;

    style = GetClassLongPtr ( hstatus, GCL_STYLE );
    if ( style )
    {
        style &= ~(CS_HREDRAW|CS_VREDRAW);
        style |= ( CS_BYTEALIGNWINDOW|CS_BYTEALIGNCLIENT/*|CS_PARENTDC*/ );
        SetClassLongPtr ( hstatus, GCL_STYLE, style );
    }

    SendMessage ( hstatus, SB_SETPARTS, 4, ( LPARAM )sbparts );
    SendMessage ( hstatus, SB_SETTEXT, 3|SBT_NOBORDERS, (LPARAM)TEXT("") );

    return hstatus;
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: SB_MenuHelp 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: void 
//                 Param.    1: HINSTANCE hInstance: current hinst
//                 Param.    2: HWND hsbar         : handle to SB
//                 Param.    3: DWORD index        : menu index+flags
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Some function to display menu items help strings in the statusbar
//                              when hovering with the mouse. MFC style :-)
//                              This is called in response to WM_MENUSELECT, with wParam as index
/*--------------------------------------------------------------------------------------------@@-@@-*/
void SB_MenuHelp ( HINSTANCE hInstance, HWND hsbar, DWORD index )
/*--------------------------------------------------------------------------------------------------*/
{
    TCHAR    temp[256];

    if ( HIWORD ( index ) == 0xFFFF )           // user has closed the menu
        SB_StatusSetSimple ( hsbar, FALSE );
    else
    {
        SB_StatusSetSimple ( hsbar, TRUE );
        if ( HIWORD ( index ) & MF_HILITE )     // item is highlighted
        {
            LoadString ( hInstance, LOWORD ( index ), temp, ARRAYSIZE ( temp ) ); // load help string from resource
            SB_StatusSetText ( hsbar, 255|SBT_NOBORDERS, temp );
        }
    }
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: SB_StatusResizePanels 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: BOOL 
//                 Param.    1: HWND hsbar : status bar
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Resize SB, according to text length
/*--------------------------------------------------------------------------------------------@@-@@-*/
BOOL SB_StatusResizePanels ( HWND hsbar )
/*--------------------------------------------------------------------------------------------------*/
{
    int     parts[4];
    DWORD   len;

    if ( !SendMessage ( hsbar, SB_GETPARTS, 4, ( LPARAM )parts ) )
        return FALSE;
    
    len = LOWORD ( SendMessage ( hsbar, SB_GETTEXTLENGTH, 0, 0 ) );    
    parts[0] = ( len * 5 ) + 8;//4

    len = LOWORD ( SendMessage ( hsbar, SB_GETTEXTLENGTH, 1, 0 ) );
    parts[1] = parts[0] + ( len * 5 ) + 20;//10

    len = LOWORD ( SendMessage ( hsbar, SB_GETTEXTLENGTH, 2, 0 ) );    
    parts[2] = parts[1] + ( len * 5 ) + 20;//10

    return (BOOL)SendMessage ( hsbar, SB_SETPARTS, 4, ( LPARAM )parts );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: SB_StatusSetSimple 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: void 
//                 Param.    1: HWND hsbar  : Status bar
//                 Param.    2: BOOL simple : yes or no :-)
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Make a statusbar singlepanel or restore it's original structure
/*--------------------------------------------------------------------------------------------@@-@@-*/
void SB_StatusSetSimple ( HWND hsbar, BOOL simple )
/*--------------------------------------------------------------------------------------------------*/
{
    SendMessage ( hsbar, SB_SIMPLE, (WPARAM)simple, (LPARAM)0 );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: SB_StatusSetText 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: BOOL 
//                 Param.    1: HWND hsbar         : status bar
//                 Param.    2: int panel          : panel index (0 based)
//                 Param.    3: const TCHAR * text : text to set
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Set panel with index "panel" caption to "text"
/*--------------------------------------------------------------------------------------------@@-@@-*/
BOOL SB_StatusSetText ( HWND hsbar, int panel, const TCHAR * text )
/*--------------------------------------------------------------------------------------------------*/
{
    return (BOOL)SendMessage ( hsbar, SB_SETTEXT, panel, (LPARAM)text );
}

