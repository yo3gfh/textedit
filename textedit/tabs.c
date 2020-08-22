
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

// tabs.c - tab control handling galore

#include "main.h"
#include "tabs.h"

HWND Tab_NewTab ( HWND hwnd )
/*****************************************************************************************************************/
/* make a tab control                                                                                            */
{
    RECT wr;
    HWND hTab;
    DWORD style;
    HFONT font;

    GetClientRect ( hwnd, &wr );
    hTab = CreateWindow ( WC_TABCONTROL, NULL, TAB_STYLE, wr.left, wr.top, 
                            wr.right-wr.left, wr.bottom-wr.top, hwnd,
                            (HMENU) IDC_TAB, NULL, NULL);

    style = GetClassLong ( hTab, GCL_STYLE );
    if ( style )
    {
        style &= ~( CS_HREDRAW|CS_VREDRAW );
        style |= ( CS_BYTEALIGNWINDOW|CS_BYTEALIGNCLIENT|CS_PARENTDC );
        SetClassLong ( hTab, GCL_STYLE, style );
    }
#ifdef _WIN64
    SetClassLong ( hTab, GCLP_HBRBACKGROUND, (LONG)GetStockObject ( NULL_BRUSH ) );
#else
    SetClassLong ( hTab, GCL_HBRBACKGROUND, (LONG)GetStockObject ( NULL_BRUSH ) );
#endif

    style = GetWindowLong ( hTab, GWL_STYLE );
    if ( style )
    {
        style &= ~TCS_HOTTRACK;
        SetWindowLong ( hTab, GWL_STYLE, style );
    }

    font = (HFONT) GetStockObject ( DEFAULT_GUI_FONT );
    SendMessage ( hTab, WM_SETFONT, ( WPARAM ) font, MAKELPARAM (TRUE,0) );

    return hTab;
}

int Tab_NewTabPage ( HWND htab, int index, TCHAR * text )
/*****************************************************************************************************************/
/* add a new tab page                                                                                            */
{
    TCITEM          tie;

    tie.mask = TCIF_TEXT|TCIF_IMAGE;
    tie.pszText = ( text == NULL ) ? TEXT("") : text;
    return SendMessage ( htab, TCM_INSERTITEM, index, (LPARAM) (LPTCITEM) &tie);
}

BOOL Tab_DeletePage ( HWND htab, int index )
/*****************************************************************************************************************/
/* delete a tab page                                                                                             */
{
    return SendMessage ( htab, TCM_DELETEITEM, index, 0 );
}

int Tab_GetCurSel ( HWND htab )
/*****************************************************************************************************************/
/* return the currently selected tab page                                                                        */
{
    return SendMessage ( htab, TCM_GETCURSEL, 0, 0 );
}

int Tab_SetCurSel ( HWND htab, int index )
/*****************************************************************************************************************/
/* Selects a tab page                                                                                            */
{
    return SendMessage ( htab, TCM_SETCURSEL, (WPARAM)index, 0 );
}

int Tab_GetCurFocus ( HWND htab )
/*****************************************************************************************************************/
/* return the currently on focus tab page                                                                        */
{
    return SendMessage ( htab, TCM_GETCURFOCUS, 0, 0 );
}

int Tab_SetCurFocus ( HWND htab, int index )
/*****************************************************************************************************************/
/* sets focus on a tab page                                                                                      */
{
    return SendMessage ( htab, TCM_SETCURFOCUS, (WPARAM)index, 0 );
}

int Tab_GetCount ( HWND htab )
/*****************************************************************************************************************/
/* return tab item count                                                                                         */
{
    return SendMessage ( htab, TCM_GETITEMCOUNT, 0, 0 );
}

BOOL Tab_SetText ( HWND htab, int index, TCHAR * text )
/*****************************************************************************************************************/
/* set the title text for a tab page                                                                             */
{
    TCITEM          tie;

    tie.mask = TCIF_TEXT;
    tie.pszText = ( text == NULL ) ? TEXT("") : text;
    return SendMessage ( htab, TCM_SETITEM, index, (LPARAM) (LPTCITEM) &tie);
}

void Tab_SetPadding ( HWND htab, int cx, int cy )
/*****************************************************************************************************************/
/* set the title text padding for a tab page                                                                     */
{
    SendMessage ( htab, TCM_SETPADDING, 0, MAKELPARAM ( cx, cy ) );
}

void Tab_SetImageList ( HWND htab, HIMAGELIST hIml )
/*****************************************************************************************************************/
/* set the image list for a tab page                                                                             */
{
    SendMessage ( htab, TCM_SETIMAGELIST, 0, (LPARAM) hIml );
}

BOOL Tab_SetImg ( HWND htab, int index, int img_index )
/*****************************************************************************************************************/
/* set the image index for a tab page                                                                            */
{
    TCITEM          tie;

    tie.mask        = TCIF_IMAGE;
    tie.iImage      = img_index;
    return SendMessage ( htab, TCM_SETITEM, index, (LPARAM) (LPTCITEM) &tie);
}
