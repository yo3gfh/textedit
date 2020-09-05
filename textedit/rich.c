
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

// rich.c - wrapper and helper functions for working with a richedit control
// could make it into a library
#pragma warn(disable: 2008 2118 2228 2231 2030 2260) //enum not used in switch, = used in conditional

#include "main.h"
#include "rich.h"
#include "misc.h"
#include "mem.h"
#include "resource.h"
#include <strsafe.h>



void Rich_GetCaretPos ( HWND hrich, CARET_POS * cpos )
/*****************************************************************************************************************/
{
    CHARRANGE   chr;

    if ( cpos == NULL ) { return; }
    SendMessage ( hrich, EM_EXGETSEL, 0, (LPARAM)&chr );
    cpos->c_line = (DWORD)SendMessage ( hrich, EM_EXLINEFROMCHAR, 0, (LPARAM)chr.cpMax );
    cpos->c_col  = chr.cpMax - (DWORD)SendMessage ( hrich, EM_LINEINDEX, (WPARAM)-1, (LPARAM)0 );
}

BOOL Rich_GetModified ( HWND hrich )
/*****************************************************************************************************************/
{
    return (BOOL)SendMessage ( hrich, EM_GETMODIFY, 0, 0 );
}

BOOL Rich_CanUndo ( HWND hrich )
/*****************************************************************************************************************/
{
    return (BOOL)SendMessage ( hrich, EM_CANUNDO, 0, 0 );
}

BOOL Rich_CanRedo ( HWND hrich )
/*****************************************************************************************************************/
{
    return (BOOL)SendMessage ( hrich, EM_CANREDO, 0, 0 );
}

BOOL Rich_CanPaste ( HWND hrich )
/*****************************************************************************************************************/
{
    return (BOOL)SendMessage ( hrich, EM_CANPASTE, CF_TEXT, 0 );
}

void Rich_SetModified ( HWND hrich, BOOL state )
/*****************************************************************************************************************/
{
    SendMessage ( hrich, EM_SETMODIFY, state, 0 );
}

BOOL Rich_ConvertCase ( HWND hrich, BOOL lower )
/*****************************************************************************************************************/
{
    void        * buf;
    CHARRANGE   chr;
    DWORD       size;
    
    SendMessage ( hrich, EM_EXGETSEL, 0, ( LPARAM )&chr );

    if ( chr.cpMax == -1 )
        size = GetWindowTextLength ( hrich );
    else
        size = chr.cpMax - chr.cpMin;
    
    buf = alloc_and_zero_mem ( size * sizeof(TCHAR) );
    if ( buf == NULL ) { return FALSE; }

    SendMessage ( hrich, EM_GETSELTEXT, 0, ( LPARAM )( TCHAR * )buf );
    if ( lower )
        CharLower ( ( TCHAR * )buf );
    else
        CharUpper ( ( TCHAR * )buf );
    SendMessage ( hrich, EM_REPLACESEL, 1, ( LPARAM )( TCHAR * )buf );
    free_mem ( buf );
    return TRUE;
}

void Rich_ConvertTabsToSpaces ( HWND hrich, DWORD tabstops, CVTTABSPROC cbproc, DWORD_PTR lParam )
/*****************************************************************************************************************/
{
    TCHAR   spaces[] = TEXT("        *");// 8 sp.
    DWORD   done;

    if ( tabstops > 8 )
        tabstops = 8;

    spaces[tabstops] = TEXT('\0');

    done = 0;
    while ( Rich_TabReplace ( hrich, spaces ) )
    {
        done++;
        if ( !cbproc ( tabstops, done, lParam ) )
            break;
    }
}

BOOL Rich_SetBGColor ( HWND hrich, HWND hparent, COLORREF color, const TCHAR * inifile )
/*****************************************************************************************************************/
{
    CHOOSECOLOR     cc;
    COLORREF        cr[16];
    COLORREF        newcolor, oldcolor;
    DWORD           sysbckg;
    int             i;

    if ( hrich == NULL ) { return FALSE; }
    if ( ( inifile == NULL ) && ( color == CC_INIRESTORE ) ) { return FALSE; }
    RtlZeroMemory ( &cc, sizeof ( cc ) );
    
    switch ( color )
    {
        case CC_USERDEFINED:
            if ( !GetPrivateProfileStruct ( TEXT("editor"), TEXT("bg_custom_colors"), cr, sizeof ( cr ), inifile ) )
            {
                sysbckg = GetSysColor ( COLOR_BTNFACE );
                for ( i = 0; i < 16; i++ )
                    cr[i] = sysbckg;
            }

            cc.lStructSize  = sizeof ( cc );
            cc.hwndOwner    = ( hparent == NULL ) ? hrich : hparent;
            cc.lpCustColors = cr;

            if ( GetPrivateProfileStruct ( TEXT("editor"), TEXT("bgcolor"), &oldcolor, sizeof ( oldcolor ), inifile ) )
            {
                cc.rgbResult    = oldcolor;
                cc.Flags        = CC_RGBINIT;
            }

            if ( !ChooseColor ( &cc ) ) { return FALSE; }
            newcolor = cc.rgbResult;
            if ( inifile != NULL )
            {
                WritePrivateProfileStruct ( TEXT("editor"), TEXT("bgcolor"), &cc.rgbResult, sizeof ( cc.rgbResult ), inifile );
                WritePrivateProfileStruct ( TEXT("editor"), TEXT("bg_custom_colors"), cc.lpCustColors, sizeof ( cr ), inifile );
            }
            break;

        case CC_INIRESTORE:
            if ( GetPrivateProfileStruct ( TEXT("editor"), TEXT("bgcolor"), &oldcolor, sizeof ( oldcolor ), inifile ) )
            {
                newcolor = oldcolor;
            }
            else
            {
                newcolor = RGB ( 255, 255, 255 );
            }   
            break;

        default:
            newcolor = color;
            if ( inifile != NULL )
                WritePrivateProfileStruct ( TEXT("editor"), TEXT("bgcolor"), &newcolor, sizeof ( COLORREF ), inifile );
            break;
    }
    SendMessage ( hrich, EM_SETBKGNDCOLOR, 0, newcolor );
    return TRUE;
}

BOOL Rich_SetFGColor ( HWND hrich, HWND hparent, COLORREF color, const TCHAR * inifile )
/*****************************************************************************************************************/
{
    CHOOSECOLOR     cc;
    COLORREF        cr[16];
    COLORREF        oldcolor;
    CHARFORMAT      cf;
    DWORD           sysbckg;
    int             i;

    if ( hrich == NULL ) { return FALSE; }
    if ( ( inifile == NULL ) && ( color == CC_INIRESTORE ) ) { return FALSE; }

    RtlZeroMemory ( &cc, sizeof ( cc ) );
    RtlZeroMemory ( &cf, sizeof ( cf ) );
    cf.cbSize       = sizeof ( cf );
    cf.dwMask       = CFM_COLOR;

    switch ( color )
    {
        case CC_USERDEFINED:
            if ( !GetPrivateProfileStruct ( TEXT("editor"), TEXT("fg_custom_colors"), cr, sizeof ( cr ), inifile ) )
            {
                sysbckg = GetSysColor ( COLOR_BTNFACE );
                for ( i = 0; i < 16; i++ )
                    cr[i] = sysbckg;
            }

            cc.lStructSize  = sizeof ( cc );
            cc.hwndOwner    = ( hparent == NULL ) ? hrich : hparent;
            cc.lpCustColors = cr;

            if ( GetPrivateProfileStruct ( TEXT("editor"), TEXT("fgcolor"), &oldcolor, sizeof ( oldcolor ), inifile ) )
            {
                cc.rgbResult    = oldcolor;
                cc.Flags        = CC_RGBINIT;
            }
            if ( !ChooseColor ( &cc ) ) { return FALSE; }
            cf.crTextColor  = cc.rgbResult;
            if ( inifile != NULL )
            {
                WritePrivateProfileStruct ( TEXT("editor"), TEXT("fgcolor"), &cc.rgbResult, sizeof ( cc.rgbResult ), inifile );
                WritePrivateProfileStruct ( TEXT("editor"), TEXT("fg_custom_colors"), cc.lpCustColors, sizeof ( cr ), inifile );
            }
            break;

        case CC_INIRESTORE:
            if ( GetPrivateProfileStruct ( TEXT("editor"), TEXT("fgcolor"), &oldcolor, sizeof ( oldcolor ), inifile ) )
            {
                cf.crTextColor = oldcolor;
            }
            else
            {
                cf.crTextColor = RGB ( 0,0,0 );
            }
            break;

        default:
            cf.crTextColor  = color;
            if ( inifile != NULL )
                WritePrivateProfileStruct ( TEXT("editor"), TEXT("fgcolor"), &color, sizeof ( COLORREF ), inifile );
            break;

    }

    SendMessage ( hrich, EM_SETCHARFORMAT, SCF_ALL, ( LPARAM )&cf );
    SendMessage ( hrich, EM_EMPTYUNDOBUFFER, 0, 0 );

    return TRUE;
}

BOOL Rich_FindNext ( HWND hrich, HWND hparent, BOOL fwd )
/*****************************************************************************************************************/
{
    CHARRANGE   chr;
    TCHAR       temp[256];//char

    SendMessage ( hrich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    if ( ( chr.cpMax - chr.cpMin ) > 250 )
    {
        ShowMessage ( hparent, TEXT("Selection is too large. Please keep it under 250 chars."), MB_OK, IDI_LARGE );
        return FALSE;
    }

    SendMessage ( hrich, EM_GETSELTEXT, 0, ( LPARAM )&temp );
    return Rich_FindReplaceText ( hrich, hparent, temp, NULL, FALSE, FALSE, FALSE, fwd );
}

BOOL Rich_TabReplace ( HWND hrich, TCHAR * rep )
/*****************************************************************************************************************/
{
    FINDTEXTEX      ft;
    CHARRANGE       chr;
    INT_PTR         tpos;
    
    SendMessage ( hrich, EM_EXGETSEL, 0, ( LPARAM )&chr );

    ft.chrg.cpMin   = chr.cpMin;
    ft.chrg.cpMax   = -1;
    ft.lpstrText    = TEXT("\t");

    tpos = SendMessage ( hrich, EM_FINDTEXTEX, ( WPARAM )FR_DOWN, ( LPARAM )&ft );

    if ( tpos != -1 )
    {
        SendMessage ( hrich, EM_EXSETSEL, 0, ( LPARAM )&ft.chrgText );
        SendMessage ( hrich, EM_REPLACESEL, FALSE, ( LPARAM )( TCHAR * )rep ); // no undo for tab replacement
        return TRUE;
    }    

    return FALSE;
}

BOOL Rich_GotoLine ( HWND hrich, DWORD line )
/*****************************************************************************************************************/
{
    INT_PTR     index;
    CHARRANGE   chr;

    index = SendMessage ( hrich, EM_LINEINDEX, line, 0 );
    if ( index != -1 )
    {
        chr.cpMax = chr.cpMin = index;
        SendMessage ( hrich, EM_EXSETSEL, 0, ( LPARAM )&chr );
        SetFocus ( hrich );
        return TRUE;
    }
    return FALSE;
}

BOOL Rich_FindReplaceText ( HWND hrich, HWND hparent, TCHAR * txt, TCHAR * rep, 
                            BOOL mcase, BOOL whole, BOOL prompt, BOOL fwd )
/*****************************************************************************************************************/
{
    FINDTEXTEX  ft;
    CHARRANGE   chr;
    DWORD       sflags;
    INT_PTR     tpos;
    TCHAR       temp[512];

    SendMessage ( hrich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    if ( rep == NULL ) { chr.cpMin++; }

    ft.chrg.cpMin   = chr.cpMin;
    ft.chrg.cpMax   = -1;//GetWindowTextLength ( hrich );
    ft.lpstrText    = txt;

    sflags          = 0;
    if ( mcase ) { sflags |= FR_MATCHCASE; }
    if ( whole ) { sflags |= FR_WHOLEWORD; }
    if ( fwd )   { sflags |= FR_DOWN; } // search up-down

    tpos = SendMessage ( hrich, EM_FINDTEXTEX, sflags, ( LPARAM )&ft );
    if ( tpos != -1 )
    {
        SendMessage ( hrich, EM_EXSETSEL, 0, ( LPARAM )&ft.chrgText );
        if ( rep != NULL )
        {
            if ( prompt )
            {
#ifdef UNICODE
                StringCchPrintf ( temp, ARRAYSIZE(temp), L"Replace \"%ls\" with \"%ls\"?", txt, rep );
#else
                StringCchPrintf ( temp, ARRAYSIZE(temp), "Replace \"%s\" with \"%s\"?", txt, rep );
#endif
                switch ( ShowMessage ( hparent, temp, MB_YESNOCANCEL, IDI_LARGE ) )
                {
                    case IDCANCEL:
                        return FALSE;
                        break;
            
                    case IDNO:
                        ft.chrgText.cpMin = ft.chrgText.cpMax;
                        SendMessage ( hrich, EM_EXSETSEL, 0, ( LPARAM )&ft.chrgText );
                        break;
            
                    case IDYES:
                        SendMessage ( hrich, EM_REPLACESEL, TRUE, ( LPARAM )rep );
                        break;
                }
            }
            else
                SendMessage ( hrich, EM_REPLACESEL, TRUE, ( LPARAM )rep );
        }
    }
    else
    {
        if ( ( rep == NULL ) || ( prompt == TRUE ) )
        {
#ifdef UNICODE
            StringCchPrintf ( temp, ARRAYSIZE(temp), L"No more matches for \"%ls\"", txt );
#else
            StringCchPrintf ( temp, ARRAYSIZE(temp), "No more matches for \"%s\"", txt );
#endif
            ShowMessage ( hparent, temp, MB_OK, IDI_LARGE );
        }
        return FALSE;
    }
    
    return TRUE;
}

DWORD Rich_GetLineCount ( HWND hrich )
/*****************************************************************************************************************/
{
    return (DWORD)SendMessage ( hrich, EM_GETLINECOUNT, 0, 0 );
}

DWORD Rich_GetCrtLine ( HWND hrich )
/*****************************************************************************************************************/
{
    CHARRANGE   chr;

    SendMessage ( hrich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    return (DWORD)SendMessage ( hrich, EM_EXLINEFROMCHAR, 0, ( LPARAM )chr.cpMax );
}

TCHAR * Rich_GetSelText ( HWND hrich )
/*****************************************************************************************************************/
{
    static TCHAR    buf[256];
    CHARRANGE       chr;
    DWORD           size;
 
    SendMessage ( hrich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    if ( chr.cpMax == -1 ) { return NULL; }
    size = chr.cpMax - chr.cpMin;
    if ( ( size == 0 ) || ( size > 250 ) ) { return NULL; }

    SendMessage ( hrich, EM_GETSELTEXT, 0, ( LPARAM )( TCHAR * )buf );
    return buf;
}

int Rich_GetSelLen ( HWND hrich )
/*****************************************************************************************************************/
{
    CHARRANGE       chr;

    SendMessage ( hrich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    return ( chr.cpMax - chr.cpMin );
}

BOOL Rich_Print ( HWND hrich, HWND hparent, const TCHAR * doc_name )
/*****************************************************************************************************************/
/* Print from a richedit control                                                                                 */
{
    HDC             hdc;
    RECT            rc, rc_page;
    HFONT           hFont;
    POINT           pt;
    PRINTDLG        pd;
    DOCINFO         di;
    FORMATRANGE     fr;
    CHARRANGE       chr;
    BOOL            fSuccess = TRUE;
    int             num_pages, to_print, cxPhysOffset, cyPhysOffset,
                    cxPhys, cyPhys, cpMin, ppx, ppy,
                    pages_begin, pages_end, crt_page, fin_page;
    DWORD           i, barlen;
    TCHAR           temp[256];
    TCHAR           wunderbar[256];

    RtlZeroMemory ( temp, sizeof(temp) );
    RtlZeroMemory ( wunderbar, sizeof(wunderbar) );

    // save current selection and scroll position
    SendMessage ( hrich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    SendMessage ( hrich, EM_GETSCROLLPOS, 0, ( LPARAM )&pt );

    // initialize the PRINTDLG structure
    RtlZeroMemory ( &pd, sizeof(PRINTDLG) );
    pd.lStructSize  = sizeof(PRINTDLG);
    pd.hwndOwner    = hparent;
    pd.hDevMode     = NULL;
    pd.hDevNames    = NULL;
    pd.Flags        = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
    pd.nCopies      = 1;
    pd.nFromPage    = 1;
    pd.nToPage      = 1;
    pd.nMinPage     = 1;
    pd.nMaxPage     = 0xFFFF;

    if ( chr.cpMin == chr.cpMax ) { pd.Flags |= PD_NOSELECTION; }   // we have no selection, disable the "print selection"
    if ( !PrintDlg ( &pd ) ) { return FALSE; }                      // display Print dialog

    hdc             = pd.hDC;

    // initialize the DOCINFO struct
    RtlZeroMemory ( &di, sizeof(DOCINFO) );
    di.cbSize       = sizeof(DOCINFO);
    di.lpszDocName  = ( doc_name == NULL ) ? TEXT("Untitled") : doc_name;  // will fail if no doc name!
    di.fwType       = 0;
    di.lpszDatatype = NULL;
    di.lpszOutput   = NULL;

    if ( !StartDoc ( hdc, &di ) )                                   // start the spool job
    {
        if ( hdc ) { DeleteDC ( hdc ); }
        if ( pd.hDevMode ) { GlobalFree ( pd.hDevMode ); }
        if ( pd.hDevNames ) { GlobalFree ( pd.hDevNames ); }
        return FALSE;
    }
    
    // get returned device caps
    cxPhysOffset = GetDeviceCaps ( hdc, PHYSICALOFFSETX );
    cyPhysOffset = GetDeviceCaps ( hdc, PHYSICALOFFSETY );
    
    cxPhys = GetDeviceCaps ( hdc, PHYSICALWIDTH );
    cyPhys = GetDeviceCaps ( hdc, PHYSICALHEIGHT );

    ppx = GetDeviceCaps ( hdc, LOGPIXELSX );                        // ppi ( 1 point = 1/72 inch = 20 twips)
    ppy = GetDeviceCaps ( hdc, LOGPIXELSY );                        // ppi

    if ( !SendMessage ( hrich, EM_SETTARGETDEVICE, ( WPARAM )hdc, 0 ) )
    {
        if ( hdc ) { DeleteDC ( hdc ); }
        if ( pd.hDevMode ) { GlobalFree ( pd.hDevMode ); }
        if ( pd.hDevNames ) { GlobalFree ( pd.hDevNames ); }
        return FALSE;
    }

    // start filling FORMATRANGE struct with req. bits
    fr.hdc       = hdc;
    fr.hdcTarget = hdc;

    // set page rect to physical page size in twips.
    fr.rcPage.top    = 0;  
    fr.rcPage.left   = 0;  
    fr.rcPage.right  = MulDiv ( cxPhys, 1440, ppx );  
    fr.rcPage.bottom = MulDiv ( cyPhys, 1440, ppy ); 

    // on virtual prn, offset will be 0.. on some, it's very small
    // try and get some sane values
    cxPhysOffset = ( cxPhysOffset == 0 ) ? ppx >> 1 : cxPhysOffset;//200
    cyPhysOffset = ( cyPhysOffset == 0 ) ? ppy >> 1 : cyPhysOffset;//400

    cxPhysOffset = ( cxPhysOffset < 100 ) ? 100 : cxPhysOffset;//200
    cyPhysOffset = ( cyPhysOffset < 200 ) ? 200 : cyPhysOffset;//400

    // set the rendering rectangle to the pintable area of the page.
    fr.rc.left   = MulDiv ( cxPhysOffset, 1440, ppx );
    fr.rc.right  = MulDiv ( cxPhys - cxPhysOffset, 1440, ppx ) ;//+
    fr.rc.top    = MulDiv ( cyPhysOffset, 1440, ppy );
    fr.rc.bottom = MulDiv ( cyPhys - cyPhysOffset, 1440, ppy ) ;//+

    // save these >:-/ richedit WILL fuck them up during the so called
    // EM_FORMATRANGE simulation
    CopyRect ( &rc, &fr.rc );
    CopyRect ( &rc_page, &fr.rcPage );

    // see what's been selected in the prn dlg
    to_print = PRINT_ALL;
    if ( pd.Flags & PD_PAGENUMS ) { to_print = PRINT_PAG; }
    if ( ( pd.Flags & PD_SELECTION ) && ( chr.cpMin != chr.cpMax ) ) { to_print = PRINT_SEL; }
    
    num_pages     = 1;
    pages_begin   = 0;
    pages_end     = 0;
    fin_page      = 0;
    crt_page      = pd.nFromPage;

    // make font for the header
    hFont = CreateFont ( -MulDiv ( 10, ppy, 72 ), 0, 0, 0, FW_DONTCARE, TRUE,
                         FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                         CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY, 
                         VARIABLE_PITCH,TEXT("Courier New" ) );

    SelectObject ( fr.hdc, hFont );

    SendMessage ( hrich, EM_SETSEL, 0, ( LPARAM )-1 );          // select the entire contents.
    SendMessage ( hrich, EM_EXGETSEL, 0, ( LPARAM )&fr.chrg );  // get the selection into a CHARRANGE.
    
    // soooo, let's see how many pages do we have and get the charrange for printing
    switch ( to_print )
    {
        case PRINT_SEL:
            while ( fr.chrg.cpMin < fr.chrg.cpMax )
            {
                cpMin = (int)SendMessage ( hrich, EM_FORMATRANGE, FALSE, ( LPARAM )&fr );
                if ( cpMin >= chr.cpMin && fin_page == 0 ) { fin_page = num_pages; }   // we hit the page with the selection
                num_pages++;                                                           // but continue to see how many pages
                fr.chrg.cpMin = cpMin;
            }
            fr.chrg.cpMin = chr.cpMin;                         // restore original selection for print
            fr.chrg.cpMax = chr.cpMax;
            if ( fin_page == num_pages ) { fin_page++; }
            crt_page = fin_page;
            SendMessage ( hrich, EM_FORMATRANGE, FALSE, 0 );   // clear cache
            CopyRect ( &fr.rc, &rc );                          // restore these...
            CopyRect ( &fr.rcPage, &rc_page );
            break;

        case PRINT_PAG:
            while ( fr.chrg.cpMin < fr.chrg.cpMax )
            {
                if ( num_pages == pd.nFromPage ) { pages_begin = fr.chrg.cpMin; }      // we hit the "from" page, save it
                cpMin = (int)SendMessage ( hrich, EM_FORMATRANGE, FALSE, ( LPARAM )&fr );
                if ( num_pages == pd.nToPage ) { pages_end = cpMin; break; }           // we hit the "to" page, we can go now
                num_pages++;
                fr.chrg.cpMin = cpMin;
            }
            // some sanity checks, set the selection for print   
            if ( pd.nFromPage > num_pages )                                            // "from" page is in the woods
            {
                fr.chrg.cpMin = 0;
                fr.chrg.cpMax = 0;
                break;
            }
            fr.chrg.cpMin = pages_begin;

            if ( !(pd.nToPage > num_pages) )
               fr.chrg.cpMax = pages_end;
            else
               pd.nToPage = (WORD)(num_pages-1);                                               // "to" page is in the woods

            fin_page = pd.nToPage;
            SendMessage ( hrich, EM_FORMATRANGE, FALSE, 0 );                           // clear cahe
            CopyRect ( &fr.rc, &rc );
            CopyRect ( &fr.rcPage, &rc_page );
            break;

        case PRINT_ALL:
            pages_begin = fr.chrg.cpMin;                                               // save selection
            pages_end = fr.chrg.cpMax;
            while ( fr.chrg.cpMin < fr.chrg.cpMax )
            {
                cpMin = (int)SendMessage ( hrich, EM_FORMATRANGE, FALSE, ( LPARAM )&fr );   // just number the pages
                num_pages++;
                fr.chrg.cpMin = cpMin;
            }
            fr.chrg.cpMin = pages_begin;                                               // restore selection for print
            fr.chrg.cpMax = pages_end;
            fin_page = num_pages-1;
            SendMessage ( hrich, EM_FORMATRANGE, FALSE, 0 );                           // clear cache
            CopyRect ( &fr.rc, &rc );
            CopyRect ( &fr.rcPage, &rc_page );
            break;
    }    

    // now do the actual printing
    while ( ( fr.chrg.cpMin < fr.chrg.cpMax ) && fSuccess ) 
    {
        fSuccess = StartPage ( hdc ) > 0;                                               // start page       
        if ( !fSuccess ) break;        
        cpMin = (int)SendMessage ( hrich, EM_FORMATRANGE, TRUE, ( LPARAM )&fr );
        // make the header
        StringCchPrintf ( temp, ARRAYSIZE(temp), TEXT(" Page %d of %d"), crt_page, fin_page );
        barlen = lstrlen ( temp );

        if ( barlen > ARRAYSIZE(wunderbar) )
            barlen = ARRAYSIZE(wunderbar);

        i = barlen+3;

        while ( i-- )
            wunderbar[i] = TEXT('_');

        SelectObject ( fr.hdc, hFont );
        SetTextColor ( fr.hdc, GetSysColor ( COLOR_3DDKSHADOW ) );
        TextOut ( hdc, cxPhysOffset, cyPhysOffset-(ppy >> 2), temp, lstrlen ( temp ) );//180
        TextOut ( hdc, cxPhysOffset, cyPhysOffset-(ppy >> 2), wunderbar, barlen+2 );//180
        crt_page++;
        fr.chrg.cpMin = cpMin;
        fSuccess = EndPage ( hdc ) > 0;                                                 // end page
    }

    SendMessage ( hrich, EM_FORMATRANGE, FALSE, 0 );                                    // clear cache
    
    if ( fSuccess ) { EndDoc ( hdc ); }                                                 // finish the spool if ok
    else { AbortDoc ( hdc ); }                                                          // ..or not

    // cleanup
    if ( hdc ) { DeleteDC ( hdc ); }
    if ( pd.hDevMode ) { GlobalFree ( pd.hDevMode ); }
    if ( pd.hDevNames ) { GlobalFree ( pd.hDevNames ); }
    if ( hFont ) { DeleteObject ( hFont ); }

    // restore selection and cursor position
    SendMessage ( hrich, EM_EXSETSEL, 0, ( LPARAM )&chr );
    SendMessage ( hrich, EM_SETSCROLLPOS, 0, ( LPARAM )&pt );

    return fSuccess;
}

HWND Rich_NewRichedit ( HINSTANCE hInst, WNDPROC * old_proc, WNDPROC new_proc,
                        HWND hParent, int richid,
                        HFONT * pfont, BOOL wrap, BOOL margin,
                        const TCHAR * inifile )
/*****************************************************************************************************************/
{
    HWND            hrich;
    DWORD           style;
    DWORD           options;
    COLORREF        cr;
    CHARFORMAT      cf;

    if ( hInst == NULL || pfont == NULL ) { return NULL; }

    options = RICH_STYLE;
    
    if ( margin ) { options |= ES_SELECTIONBAR; }

    
    hrich = CreateWindowEx ( WS_EX_CLIENTEDGE, RED_CLASS, 0,
                             options, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             hParent, ( HMENU )richid, hInst,
                             NULL
                           );

    if ( !hrich ) return NULL;
    
    style = GetClassLong ( hrich, GCL_STYLE );
    if ( style )
    {
        style &= ~( CS_HREDRAW|CS_VREDRAW );
        style |= ( CS_BYTEALIGNWINDOW|CS_BYTEALIGNCLIENT );
        SetClassLong ( hrich, GCL_STYLE, style );
    }

    if ( *pfont == NULL )
    {
        *pfont = CreateFont ( -13, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                TEXT("Courier New")
                            );
    }

    SendMessage ( hrich, WM_SETFONT, (WPARAM)*pfont, TRUE );
    SendMessage ( hrich, EM_SETEVENTMASK, 0, ENM_KEYEVENTS|ENM_CHANGE|ENM_SELCHANGE/*|ENM_MOUSEEVENTS*/ );
    SendMessage ( hrich, EM_EXLIMITTEXT, 0, 60000000 ); // ~60MB of text :-)
    if ( new_proc != NULL )
        *old_proc = ( WNDPROC ) SetWindowLongPtr ( hrich, GWLP_WNDPROC, ( LONG_PTR ) new_proc );

    if ( GetPrivateProfileStruct ( TEXT("editor"), TEXT("bgcolor"), &cr, sizeof ( cr ), inifile ) )
        SendMessage ( hrich, EM_SETBKGNDCOLOR, 0, cr );

    if ( GetPrivateProfileStruct ( TEXT("editor"), TEXT("fgcolor"), &cr, sizeof ( cr ), inifile ) )
    {
        RtlZeroMemory ( &cf, sizeof ( cf ) );
        cf.cbSize       = sizeof ( cf );
        cf.dwMask       = CFM_COLOR;
        cf.crTextColor  = cr;
        SendMessage ( hrich, EM_SETCHARFORMAT, SCF_ALL, ( LPARAM )&cf );
        SendMessage ( hrich, EM_EMPTYUNDOBUFFER, 0, 0 );
    }

    SendMessage ( hrich, EM_SETTEXTMODE, TM_PLAINTEXT|TM_MULTILEVELUNDO|TM_MULTICODEPAGE, 0 );
    SendMessage ( hrich, EM_SETTYPOGRAPHYOPTIONS, TO_SIMPLELINEBREAK, TO_SIMPLELINEBREAK );
    //SendMessage ( hrich, EM_SETEDITSTYLEEX, SES_EX_NOACETATESELECTION, SES_EX_NOACETATESELECTION ); // no effect
    //SendMessage ( hrich, EM_SETEDITSTYLE, SES_MULTISELECT, SES_MULTISELECT ); // Win8+
    //SendMessage ( hrich, EM_SETEDITSTYLE, SES_EMULATESYSEDIT, SES_EMULATESYSEDIT ); // faster op., no multilevel undo

    Rich_WordWrap ( hrich, wrap );
    SendMessage ( hrich, EM_SETMODIFY, 0, 0 );

    return hrich;
}

BOOL Rich_Streamin ( HWND hrich, HWND pb_parent, EDITSTREAMCALLBACK streamin_proc, int encoding, TCHAR * fname )
/*****************************************************************************************************************/
{
    HANDLE              hFile;
    HWND                hpbar;
    EDITSTREAM          est;
    DWORD               fsize;
    STREAM_AUX_DATA     sdata;

    hFile = CreateFile ( fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );

    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;

    fsize = GetFileSize ( hFile, NULL );
    hpbar = ( pb_parent == NULL ) ? NULL : PB_NewProgressBar ( pb_parent, 60, 150 );

    sdata.fsize = fsize;
    sdata.hfile = hFile;
    sdata.hpbar = hpbar;
    sdata.fencoding = ( encoding == -1 ) ? E_DONT_BOTHER : encoding ;

    est.dwCookie        = (DWORD_PTR)&sdata;
    est.dwError         = 0;
    est.pfnCallback     = streamin_proc;

    SendMessage ( hrich, EM_STREAMIN, SF_TEXT|SF_UNICODE, ( LPARAM )&est );

    if ( hpbar ) DestroyWindow ( hpbar );
    CloseHandle ( hFile );

    return ( est.dwError == 0 );
}

BOOL Rich_SelStreamin ( HWND hrich, HWND pb_parent, EDITSTREAMCALLBACK streamin_proc, int encoding, TCHAR * fname )
/*****************************************************************************************************************/
{
    HANDLE              hFile;
    HWND                hpbar;
    EDITSTREAM          est;
    DWORD               fsize;
    STREAM_AUX_DATA     sdata;

    hFile = CreateFile ( fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );

    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;

    fsize = GetFileSize ( hFile, NULL );
    hpbar = ( pb_parent == NULL ) ? NULL : PB_NewProgressBar ( pb_parent, 60, 150 );

    sdata.fsize = fsize;
    sdata.hfile = hFile;
    sdata.hpbar = hpbar;
    sdata.fencoding = ( encoding == -1 ) ? E_DONT_BOTHER : encoding;

    est.dwCookie        = (DWORD_PTR)&sdata;
    est.dwError         = 0;
    est.pfnCallback     = streamin_proc;
  
    SendMessage ( hrich, EM_STREAMIN, SF_TEXT|SFF_SELECTION|SF_UNICODE, ( LPARAM )&est );

    if ( hpbar ) DestroyWindow ( hpbar );
    CloseHandle ( hFile );

    return TRUE;
}

BOOL Rich_Streamout ( HWND hrich, HWND pb_parent, EDITSTREAMCALLBACK streamout_proc, int encoding, TCHAR * fname )
/*****************************************************************************************************************/
{
    HANDLE              hFile;
    HWND                hpbar;
    EDITSTREAM          est;
    DWORD               fsize;
    STREAM_AUX_DATA     sdata;
    
    hFile = CreateFile ( fname, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );

    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;

    fsize = GetWindowTextLength ( hrich );
    hpbar = ( pb_parent == NULL ) ? NULL : PB_NewProgressBar ( pb_parent, 60, 150 );

    sdata.fsize = fsize;
    sdata.hfile = hFile;
    sdata.hpbar = hpbar;
    sdata.fencoding = ( encoding == -1 ) ? E_DONT_BOTHER : encoding ;

    est.dwCookie        = (DWORD_PTR)&sdata;
    est.dwError         = 0;
    est.pfnCallback     = streamout_proc;

    SendMessage ( hrich, EM_STREAMOUT, SF_TEXT|SF_UNICODE, ( LPARAM )&est );

    if ( hpbar ) DestroyWindow ( hpbar );    
    CloseHandle ( hFile );

    return TRUE;
}

BOOL Rich_SelStreamout ( HWND hrich, HWND pb_parent, EDITSTREAMCALLBACK streamout_proc, int encoding, TCHAR * fname )
/*****************************************************************************************************************/
{
    HANDLE              hFile;
    HWND                hpbar;
    EDITSTREAM          est;
    DWORD               fsize;
    STREAM_AUX_DATA     sdata;
    
    hFile = CreateFile ( fname, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );

    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;

    fsize = Rich_GetSelLen ( hrich );
    hpbar = ( pb_parent == NULL ) ? NULL : PB_NewProgressBar ( pb_parent, 60, 150 );

    sdata.fsize = fsize;
    sdata.hfile = hFile;
    sdata.hpbar = hpbar;
    sdata.fencoding = ( encoding == -1 ) ? E_DONT_BOTHER : encoding ;

    est.dwCookie        = (DWORD_PTR)&sdata;
    est.dwError         = 0;
    est.pfnCallback     = streamout_proc;

    SendMessage ( hrich, EM_STREAMOUT, SF_TEXT|SFF_SELECTION|SF_UNICODE, ( LPARAM )&est );

    if ( hpbar ) DestroyWindow ( hpbar );
    CloseHandle ( hFile );

    return TRUE;
}

void Rich_SetFontNoCF ( HWND hrich, HFONT font )
/*****************************************************************************************************************/
{
    SendMessage ( hrich, WM_SETFONT, ( WPARAM ) font, MAKELPARAM (TRUE,0) );
}

BOOL Rich_SetFont ( HWND hrich, HWND hparent, HFONT * pfont, const TCHAR * inifile )
/*****************************************************************************************************************/
{
    CHOOSEFONT  cf;
    LOGFONT     lf;

    if ( hrich == NULL || pfont == NULL ) { return FALSE; }

    RtlZeroMemory ( &cf, sizeof ( cf ) );
    RtlZeroMemory ( &lf, sizeof ( lf ) );

    if ( inifile != NULL )
    {
        GetPrivateProfileStruct ( TEXT("editor"), TEXT("log_font"), &lf, sizeof(LOGFONT), inifile );
        GetPrivateProfileStruct ( TEXT("editor"), TEXT("choose_font"), &cf, sizeof(CHOOSEFONT), inifile );
    }

    cf.lStructSize  = sizeof ( cf );
    cf.hwndOwner    = hparent;
    cf.lpLogFont    = &lf;
    cf.Flags        = CF_EFFECTS|CF_INITTOLOGFONTSTRUCT|CF_SCREENFONTS|CF_NOVECTORFONTS;

    if ( ChooseFont ( &cf ) )
    {
        DeleteObject ( *pfont );
        *pfont = CreateFontIndirect ( &lf );
        SendMessage ( hrich, WM_SETFONT, ( WPARAM ) *pfont, MAKELPARAM (TRUE,0) );        
        Rich_SetFGColor ( hrich, NULL, cf.rgbColors, inifile );
        if ( inifile != NULL )
        {
            WritePrivateProfileStruct ( TEXT("editor"), TEXT("log_font"), cf.lpLogFont, sizeof ( LOGFONT ), inifile );/*&lf*/
            WritePrivateProfileStruct ( TEXT("editor"), TEXT("choose_font"), &cf, sizeof ( CHOOSEFONT ), inifile );
        }
        return TRUE;
    }
    return FALSE;
}

BOOL Rich_ToggleSelBar ( HINSTANCE hInst, WNDPROC * old_proc, WNDPROC new_proc,
                         HWND * phrich, HWND hparent, int rich_id, HFONT * pfont,
                         BOOL wrap, BOOL margin, const TCHAR * inifile )
/*****************************************************************************************************************/
{
    DWORD               size;
    void                * buf;
    TEXTRANGE           tr;
    CHARRANGE           chr;
    POINT               pt;

    buf                 = NULL;
    size                = GetWindowTextLength ( *phrich );
    SendMessage ( *phrich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    SendMessage ( *phrich, EM_GETSCROLLPOS, 0, ( LPARAM )&pt );

    if ( size != 0 )
    {
        buf = alloc_and_zero_mem ( size * sizeof(TCHAR) );
        if ( buf == NULL ) return FALSE;
        tr.chrg.cpMin = 0;
        tr.chrg.cpMax = size;
        tr.lpstrText  = ( TCHAR * )buf;
        SendMessage ( *phrich, EM_GETTEXTRANGE, 0, ( LPARAM )&tr );
    }

    DestroyWindow ( *phrich );
    *phrich = Rich_NewRichedit ( hInst, old_proc, new_proc, hparent, rich_id, pfont, wrap, margin, inifile );

    if ( size != 0 )
    {
        SetWindowText ( *phrich, ( TCHAR * )buf );
        free_mem ( buf );
    }
    
    SetFocus ( *phrich );  
    SendMessage ( *phrich, EM_EXSETSEL, 0, ( LPARAM )&chr );
    SendMessage ( *phrich, EM_SETSCROLLPOS, 0, ( LPARAM )&pt );
    return TRUE;
}

void Rich_WordWrap ( HWND hrich, BOOL wrap )
/*****************************************************************************************************************/
{
    if ( wrap )
        SendMessage ( hrich, EM_SETTARGETDEVICE, 0, 0 );
    else
        SendMessage ( hrich, EM_SETTARGETDEVICE, 0, 100000 );
}

void Rich_SaveSelandcurpos ( HWND hrich, SELANDCUR_POS * pscp )
/*****************************************************************************************************************/
{
    if ( pscp == NULL || hrich == NULL ) { return; }
    SendMessage ( hrich, EM_EXGETSEL, 0, ( LPARAM )&pscp->chr );
    SendMessage ( hrich, EM_GETSCROLLPOS, 0, ( LPARAM )&pscp->pt );
}

void Rich_RestoreSelandcurpos ( HWND hrich, const SELANDCUR_POS * pscp )
/*****************************************************************************************************************/
{
    if ( pscp == NULL || hrich == NULL ) { return; }
    SendMessage ( hrich, EM_EXSETSEL, 0, ( LPARAM )&pscp->chr );
    SendMessage ( hrich, EM_SETSCROLLPOS, 0, ( LPARAM )&pscp->pt );
}

BOOL Rich_CreateShowCaret ( HWND hrich, int cw, int ch )
/*****************************************************************************************************************/
{
    BOOL  result;

    result = CreateCaret ( hrich, (HBITMAP) NULL, cw, ch );
    ShowCaret ( hrich );
    return result;
}

int Rich_GetFontHeight ( HWND hrich )
/*****************************************************************************************************************/
{
    CHARFORMAT      cf;
    int             ppy;
    HDC             hdc;

    hdc = GetDC ( hrich );
    if ( hdc == NULL ) return -1;
    ppy = GetDeviceCaps ( hdc, LOGPIXELSY );

    RtlZeroMemory ( &cf, sizeof ( cf ) );
    cf.cbSize = sizeof ( cf );
    cf.dwMask = CFM_SIZE;
    if ( !SendMessage ( hrich, EM_GETCHARFORMAT, (WPARAM)SCF_DEFAULT, (LPARAM)&cf ) ) return -1;
    return MulDiv ( cf.yHeight, ppy, 1440 ); //  point = 1/72 inch = 20 twips
}

void Rich_ClearText ( HWND hrich )
/*****************************************************************************************************************/
{
    SendMessage ( hrich, EM_SETTEXTMODE, TM_PLAINTEXT|TM_MULTILEVELUNDO|TM_MULTICODEPAGE, 0 );
    SendMessage ( hrich, WM_SETTEXT, 0, 0 );
    SendMessage ( hrich, EM_FORMATRANGE, FALSE, 0 );  // clear cache
}
