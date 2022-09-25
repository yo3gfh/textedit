
// rich.c - wrapper and helper functions for working with a richedit control
// could make it into a library
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include "main.h"
#include "rich.h"
#include "misc.h"
#include "mem.h"
#include "resource.h"
#include <strsafe.h>

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GetCaretPos 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich       : richedit
//    Param.    2: CARET_POS * cpos : pointer to a CARET_POS struct to
//                                    receive data
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Updates a CARET_POS struct with caret position
/*--------------------------------------------------------------------@@-@@-*/
void Rich_GetCaretPos ( HWND hRich, CARET_POS * cpos )
/*--------------------------------------------------------------------------*/
{
    CHARRANGE   chr;

    if ( cpos == NULL )
        return;

    SendMessage ( hRich, EM_EXGETSEL, 0, (LPARAM)&chr );

    cpos->c_line = (DWORD)SendMessage ( hRich, EM_EXLINEFROMCHAR,
                                        0, (LPARAM)chr.cpMax );

    cpos->c_col  = chr.cpMax - (DWORD)SendMessage ( hRich, EM_LINEINDEX,
                                                    (WPARAM)-1, (LPARAM)0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GetModified 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich : richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Check the dirty flag on richedit control
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_GetModified ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    return (BOOL)SendMessage ( hRich, EM_GETMODIFY, 0, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_CanUndo 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich : richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Is undo possible?
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_CanUndo ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    return (BOOL)SendMessage ( hRich, EM_CANUNDO, 0, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_CanRedo 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich : richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Is redo possible?
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_CanRedo ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    return (BOOL)SendMessage ( hRich, EM_CANREDO, 0, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_CanPaste 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich : richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Is paste possible?
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_CanPaste ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    return (BOOL)SendMessage ( hRich, EM_CANPASTE, CF_TEXT, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_SetModified 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich : richedit control
//    Param.    2: BOOL state : TRUE or FALSE
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Set the dirty flag for richedit control
/*--------------------------------------------------------------------@@-@@-*/
void Rich_SetModified ( HWND hRich, BOOL state )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hRich, EM_SETMODIFY, state, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_ConvertCase 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich : richedit control
//    Param.    2: BOOL lower : TRUE if lowercase
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Copies sel. text into a dynamically alloc. buffer and
//                 converts it to specified case, then replaces selection
//                 with the new text. Returns TRUE on success,
//                 FALSE otherwise.
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_ConvertCase ( HWND hRich, BOOL lower )
/*--------------------------------------------------------------------------*/
{
    void        * buf;
    CHARRANGE   chr;
    INT_PTR     size, alloc_size;
    BOOL        allow_undo;

    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&chr );

    if ( chr.cpMax == -1 )
        size = GetWindowTextLength ( hRich );
    else
        size = chr.cpMax - chr.cpMin;

    // always assume unicode
    alloc_size = size * sizeof(WCHAR);
    buf = alloc_and_zero_mem ( ALIGN_4K(alloc_size));  
    
    if ( buf == NULL )
        return FALSE;

    // disable undo over 4 meg, there seems to be some mem leak in richedit?
    allow_undo = (alloc_size > 0x00400000) ? FALSE : TRUE; 
    SendMessage ( hRich, EM_GETSELTEXT, 0, ( LPARAM )( TCHAR * )buf );

    if ( lower )
        CharLower ( ( TCHAR * )buf );
    else
        CharUpper ( ( TCHAR * )buf );

    SendMessage ( hRich, EM_REPLACESEL, allow_undo, (LPARAM)(TCHAR *)buf );
    
    return free_mem ( buf );
}


/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_ConvertTabsToSpaces 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich        : richedit control
//    Param.    2: DWORD tabstops    : how many spaces to insert
//    Param.    3: CVTTABSPROC cbproc: callback to call on each found item
//    Param.    4: LPARAM lParam     : custom data to pass to cbproc
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Call Rich_TabReplace repeatedly until EOF or cbproc
//                 returns FALSE.
/*--------------------------------------------------------------------@@-@@-*/
void Rich_ConvertTabsToSpaces ( HWND hRich, DWORD tabstops,
    CVTTABSPROC cbproc, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR   spaces[] = TEXT("        *");// 8 sp.
    DWORD   done;

    if ( tabstops > 8 )
        tabstops = 8;

    spaces[tabstops] = TEXT('\0');

    done = 0;
    while ( Rich_TabReplace ( hRich, spaces ) )
    {
        done++;

        if ( !cbproc ( tabstops, done, lParam ) )
            break;
    }
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_SetBGColor 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich            : richedit control
//    Param.    2: HWND hParent          : parent control
//    Param.    3: COLORREF color        : color to set, see description
//    Param.    4: const TCHAR * inifile : full path to config
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Set bg. color for a richedit control; color param. can be
//                 a COLORREF value or one of the following:
//                 - CC_INIRESTORE: restore prev. saved color from config
//                   file "inifile"
//                 - CC_USERDEFINED: displays ChooseColor dlg. and,
//                   if inifile not NULL, saves selection to config
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_SetBGColor ( HWND hRich, HWND hParent,
    COLORREF color, const TCHAR * inifile )
/*--------------------------------------------------------------------------*/
{
    CHOOSECOLOR     cc;
    COLORREF        cr[16];
    COLORREF        newcolor, oldcolor;
    DWORD           sysbckg;
    size_t          i;

    if ( hRich == NULL )
        return FALSE;

    if ( ( inifile == NULL ) && ( color == CC_INIRESTORE ) )
        return FALSE;

    RtlZeroMemory ( &cc, sizeof ( cc ) );

    switch ( color )
    {
        case CC_USERDEFINED:
            if ( !GetPrivateProfileStruct ( TEXT("editor"),
                    TEXT("bg_custom_colors"), cr, sizeof ( cr ), inifile ) )
            {
                sysbckg = GetSysColor ( COLOR_BTNFACE );
                for ( i = 0; i < 16; i++ )
                    cr[i] = sysbckg;
            }

            cc.lStructSize  = sizeof ( cc );
            cc.hwndOwner    = ( hParent == NULL ) ? hRich : hParent;
            cc.lpCustColors = cr;

            if ( GetPrivateProfileStruct ( TEXT("editor"),
                    TEXT("bgcolor"), &oldcolor, sizeof (oldcolor), inifile ))
            {
                cc.rgbResult    = oldcolor;
                cc.Flags        = CC_RGBINIT;
            }

            if ( !ChooseColor ( &cc ) )
                return FALSE;

            newcolor = cc.rgbResult;

            if ( inifile != NULL )
            {
                WritePrivateProfileStruct ( TEXT("editor"),
                    TEXT("bgcolor"), &cc.rgbResult,
                        sizeof ( cc.rgbResult ), inifile );

                WritePrivateProfileStruct ( TEXT("editor"),
                    TEXT("bg_custom_colors"), cc.lpCustColors,
                        sizeof ( cr ), inifile );
            }
            break;

        case CC_INIRESTORE:

            if ( GetPrivateProfileStruct ( TEXT("editor"),
                    TEXT("bgcolor"), &oldcolor, sizeof (oldcolor), inifile ))
                newcolor = oldcolor;
            else
                newcolor = RGB ( 255, 255, 255 );

            break;

        default:
            newcolor = color;

            if ( inifile != NULL )
                WritePrivateProfileStruct ( TEXT("editor"),
                    TEXT("bgcolor"), &newcolor, sizeof (COLORREF), inifile );

            break;
    }

    SendMessage ( hRich, EM_SETBKGNDCOLOR, 0, newcolor );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_SetFGColor 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich            : 
//    Param.    2: HWND hParent          : 
//    Param.    3: COLORREF color        : 
//    Param.    4: const TCHAR * inifile : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Sane as Rich_SetBGColor, see above
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_SetFGColor ( HWND hRich, HWND hParent,
    COLORREF color, const TCHAR * inifile )
/*--------------------------------------------------------------------------*/
{
    CHOOSECOLOR     cc;
    COLORREF        cr[16];
    COLORREF        oldcolor;
    CHARFORMAT      cf;
    DWORD           sysbckg;
    size_t          i;

    if ( hRich == NULL )
        return FALSE;

    if ( ( inifile == NULL ) && ( color == CC_INIRESTORE ) )
        return FALSE; 

    RtlZeroMemory ( &cc, sizeof ( cc ) );
    RtlZeroMemory ( &cf, sizeof ( cf ) );
    cf.cbSize       = sizeof ( cf );
    cf.dwMask       = CFM_COLOR;

    switch ( color )
    {
        case CC_USERDEFINED:
            if ( !GetPrivateProfileStruct ( TEXT("editor"),
                    TEXT("fg_custom_colors"), cr, sizeof (cr), inifile ))
            {
                sysbckg = GetSysColor ( COLOR_BTNFACE );
                for ( i = 0; i < 16; i++ )
                    cr[i] = sysbckg;
            }

            cc.lStructSize  = sizeof ( cc );
            cc.hwndOwner    = ( hParent == NULL ) ? hRich : hParent;
            cc.lpCustColors = cr;

            if ( GetPrivateProfileStruct ( TEXT("editor"),
                    TEXT("fgcolor"), &oldcolor, sizeof (oldcolor), inifile ))
            {
                cc.rgbResult    = oldcolor;
                cc.Flags        = CC_RGBINIT;
            }

            if ( !ChooseColor ( &cc ) )
                return FALSE;

            cf.crTextColor  = cc.rgbResult;

            if ( inifile != NULL )
            {
                WritePrivateProfileStruct ( TEXT("editor"),
                    TEXT("fgcolor"), &cc.rgbResult,
                        sizeof ( cc.rgbResult ), inifile );

                WritePrivateProfileStruct ( TEXT("editor"),
                    TEXT("fg_custom_colors"), cc.lpCustColors,
                        sizeof ( cr ), inifile );
            }

            break;

        case CC_INIRESTORE:

            if ( GetPrivateProfileStruct ( TEXT("editor"),
                    TEXT("fgcolor"), &oldcolor, sizeof (oldcolor), inifile ))
                cf.crTextColor = oldcolor;
            else
                cf.crTextColor = RGB ( 0, 0, 0 );

            break;

        default:
            cf.crTextColor  = color;

            if ( inifile != NULL )
                WritePrivateProfileStruct ( TEXT("editor"),
                    TEXT("fgcolor"), &color, sizeof ( COLORREF ), inifile );

            break;

    }

    SendMessage ( hRich, EM_SETCHARFORMAT, SCF_ALL, ( LPARAM )&cf );
    SendMessage ( hRich, EM_EMPTYUNDOBUFFER, 0, 0 );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_FindNext 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich  : richedit control
//    Param.    2: HWND hParent: parent for richedit (may be NULL)
//    Param.    3: BOOL fwd    : TRUE to search FWND
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Calls Rich_FindReplaceText with the text under selection.
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_FindNext ( HWND hRich, HWND hParent, BOOL fwd )
/*--------------------------------------------------------------------------*/
{
    CHARRANGE   chr;
    TCHAR       temp[256];

    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&chr );

    if ( ( chr.cpMax - chr.cpMin ) > 250 )
    {
        ShowMessage ( hParent,
            TEXT("Selection is too large. Please keep it under 250 chars."),
                MB_OK, IDI_APP );

        return FALSE;
    }

    SendMessage ( hRich, EM_GETSELTEXT, 0, ( LPARAM )temp );

    return Rich_FindReplaceText ( hRich, hParent,
                temp, NULL, FALSE, FALSE, FALSE, fwd );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_TabReplace 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich  : richedit control
//    Param.    2: TCHAR * rep : string with spaces to replace tabs
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Replace tabs with rep string, using EM_FINDTEXTEX/
//                 EM_REPLACESEL messages. Much slower than some custom
//                 designed routine, but the laziness :)
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_TabReplace ( HWND hRich, TCHAR * rep )
/*--------------------------------------------------------------------------*/
{
    FINDTEXTEX      ft;
    CHARRANGE       chr;
    INT_PTR         tpos;

    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&chr );

    ft.chrg.cpMin   = chr.cpMin;
    ft.chrg.cpMax   = -1;
    ft.lpstrText    = TEXT("\t");

    tpos = SendMessage ( hRich, EM_FINDTEXTEX,
                            ( WPARAM )FR_DOWN, ( LPARAM )&ft );

    if ( tpos != -1 )
    {
        SendMessage ( hRich, EM_EXSETSEL, 0, ( LPARAM )&ft.chrgText );
        // no undo for tab replacement
        SendMessage ( hRich, EM_REPLACESEL, FALSE, ( LPARAM )( TCHAR * )rep ); 
        return TRUE;
    }

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GotoLine 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich : richedit control
//    Param.    2: DWORD line : line to go to
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Jump to specified line.
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_GotoLine ( HWND hRich, DWORD line )
/*--------------------------------------------------------------------------*/
{
    INT_PTR     index;
    CHARRANGE   chr;

    index = SendMessage ( hRich, EM_LINEINDEX, line, 0 );

    if ( index != -1 )
    {
        chr.cpMax = chr.cpMin = (int)index;
        SendMessage ( hRich, EM_EXSETSEL, 0, ( LPARAM )&chr );
        SetFocus ( hRich );
        return TRUE;
    }

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_LineIndex 
/*--------------------------------------------------------------------------*/
//           Type: DWORD 
//    Param.    1: HWND hRich : richedit control
//    Param.    2: DWORD line : line to get starting char for
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns the char index of the starting element of
//                 the specified line.
/*--------------------------------------------------------------------@@-@@-*/
DWORD Rich_LineIndex ( HWND hRich, DWORD line )
/*--------------------------------------------------------------------------*/
{
    return (DWORD)SendMessage ( hRich, EM_LINEINDEX, (WPARAM)line, (LPARAM)0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_CrtLineIndex 
/*--------------------------------------------------------------------------*/
//           Type: DWORD 
//    Param.    1: HWND hRich : richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns the char index of the starting element for line the
//                 line that contains the caret (current line)
/*--------------------------------------------------------------------@@-@@-*/
DWORD Rich_CrtLineIndex ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    return (DWORD)SendMessage ( hRich, EM_LINEINDEX, (WPARAM)-1, (LPARAM)0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GetLineLength 
/*--------------------------------------------------------------------------*/
//           Type: DWORD 
//    Param.    1: HWND hRich : richedit control
//    Param.    2: DWORD line : line to get length for
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns the length (in chars) of the specified line
/*--------------------------------------------------------------------@@-@@-*/
DWORD Rich_GetLineLength ( HWND hRich, DWORD line )
/*--------------------------------------------------------------------------*/
{
    DWORD   chIdx;

    chIdx = Rich_LineIndex ( hRich, line );
    return (DWORD)SendMessage (hRich, EM_LINELENGTH, (WPARAM)chIdx, (LPARAM)0);
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_LineFromChar 
/*--------------------------------------------------------------------------*/
//           Type: DWORD 
//    Param.    1: HWND hRich  : richedit control
//    Param.    2: DWORD ichar : desired char index
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns the line on which ichar resides
/*--------------------------------------------------------------------@@-@@-*/
DWORD Rich_LineFromChar ( HWND hRich, DWORD ichar )
/*--------------------------------------------------------------------------*/
{
    return (DWORD)SendMessage ( hRich, EM_EXLINEFROMCHAR,
                                (WPARAM)0, (LPARAM)ichar );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GetLineText 
/*--------------------------------------------------------------------------*/
//           Type: WORD 
//    Param.    1: HWND hRich  : richedit control
//    Param.    2: DWORD line  : line whos text to fetch
//    Param.    3: TCHAR * dest: buffer to copy to
//    Param.    4: WORD cchMax : buffer size, in chars
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Copies the desired line to dest, cchMax max.
/*--------------------------------------------------------------------@@-@@-*/
WORD Rich_GetLineText ( HWND hRich, DWORD line, TCHAR * dest, WORD cchMax )
/*--------------------------------------------------------------------------*/
{
    WORD    res;

    if ( dest == NULL || cchMax == 0 )
        return 0;

    // fill the first word from the buffer with buffer size,
    // as required by EM_GETLINE
    ((WORD *)dest)[0]   = cchMax;
    res                 = (WORD)SendMessage ( hRich, EM_GETLINE,
                                                (WPARAM)line, (LPARAM)dest );
    dest[res]           = TEXT('\0'); // null terminate the buffer

    return res;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_FindText 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich       : richedit control
//    Param.    2: TCHAR * txt      : text to find
//    Param.    3: FINDALLPROC fproc: callback to exec when hit 
//    Param.    4: BOOL mcase       : case sensitive ?
//    Param.    5: BOOL whole       : whole words?
//    Param.    6: BOOL fwd         : fwd/bkwd search
//    Param.    7: LPARAM lParam    : custom data for fproc
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 01.10.2020
//    DESCRIPTION: Search all occurences of txt in a richedit.
/*--------------------------------------------------------------------@@-@@-*/
DWORD Rich_FindText ( HWND hRich, TCHAR * txt, 
    FINDALLPROC fproc, BOOL mcase, BOOL whole, BOOL fwd, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    FINDTEXTEX      ft;
    CHARRANGE       chr;
    DWORD           sflags;
    INT_PTR         tpos;
    DWORD           line, lines, dwHits;
    TCHAR           temp[2048];

    ft.chrg.cpMax   = -1;
    ft.lpstrText    = txt;
    sflags          = 0;
    dwHits          = 0;
    line            = 0;
    lines           = Rich_GetLineCount ( hRich );

    if ( mcase ) { sflags |= FR_MATCHCASE; }
    if ( whole ) { sflags |= FR_WHOLEWORD; }
    if ( fwd )   { sflags |= FR_DOWN; } // search up-down

    while ( TRUE )
    {
        SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&chr );
        chr.cpMin++;

        ft.chrg.cpMin   = chr.cpMin;
        tpos            = SendMessage ( hRich, 
                            EM_FINDTEXTEX, sflags, ( LPARAM )&ft );
        line            = (DWORD)SendMessage ( hRich, EM_EXLINEFROMCHAR,
                            0, ( LPARAM )ft.chrgText.cpMax );

        if ( tpos != -1 )
        {
            Rich_GetLineText ( hRich, line, temp, ARRAYSIZE(temp) );

            if ( !fproc ( TRUE, temp, line, lParam ) )
                break;

            if ( fwd )
                ft.chrgText.cpMin = ft.chrgText.cpMax;
            else
                ft.chrgText.cpMax = ft.chrgText.cpMin;

            SendMessage ( hRich, EM_EXSETSEL, 0, ( LPARAM )&ft.chrgText );
            dwHits++;
        }
        else
            break;

        temp[0] = TEXT('\0');

        if ( !fproc ( FALSE, temp, line, lParam ) )
            break;
    }

    return dwHits;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_FindReplaceText 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich  : richedit control
//    Param.    2: HWND hParent: parent to richedit control (may be null)
//    Param.    3: TCHAR * txt : text to find
//    Param.    4: TCHAR * rep : text to replace with (may be null)
//    Param.    5: BOOL mcase  : case sensitive or not
//    Param.    6: BOOL whole  : whole words or not
//    Param.    7: BOOL prompt : prompt on replace or not
//    Param.    8: BOOL fwd    : FWD/BKWD search/replace
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Search/replace for a richedit control, EM_FINDTEXTEX based
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_FindReplaceText ( HWND hRich, HWND hParent, TCHAR* txt, TCHAR* rep,
                            BOOL mcase, BOOL whole,   BOOL prompt, BOOL fwd )
/*--------------------------------------------------------------------------*/
{
    FINDTEXTEX  ft;
    CHARRANGE   chr;
    DWORD       sflags;
    INT_PTR     tpos;
    TCHAR       temp[512];

    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&chr );

    if ( rep == NULL ) 
        chr.cpMin++;

    ft.chrg.cpMin   = chr.cpMin;
    ft.chrg.cpMax   = -1;//GetWindowTextLength ( hRich );
    ft.lpstrText    = txt;

    sflags          = 0;
    if ( mcase ) { sflags |= FR_MATCHCASE; }
    if ( whole ) { sflags |= FR_WHOLEWORD; }
    if ( fwd )   { sflags |= FR_DOWN; } // search up-down

    tpos = SendMessage ( hRich, EM_FINDTEXTEX, sflags, ( LPARAM )&ft );
    if ( tpos != -1 )
    {
        SendMessage ( hRich, EM_EXSETSEL, 0, ( LPARAM )&ft.chrgText );
        if ( rep != NULL )
        {
            if ( prompt )
            {
                #ifdef UNICODE
                    StringCchPrintf ( temp, ARRAYSIZE(temp),
                        L"Replace \"%ls\" with \"%ls\"?", txt, rep );
                #else
                    StringCchPrintf ( temp, ARRAYSIZE(temp),
                        "Replace \"%s\" with \"%s\"?", txt, rep );
                #endif
                switch ( ShowMessage ( hParent, temp,
                            MB_YESNOCANCEL, IDI_APP ) )
                {
                    case IDCANCEL:
                        return FALSE;
                        break;

                    case IDNO:
                        ft.chrgText.cpMin = ft.chrgText.cpMax;
                        SendMessage ( hRich, EM_EXSETSEL,
                                        0, ( LPARAM )&ft.chrgText );
                        break;

                    case IDYES:
                        SendMessage ( hRich, EM_REPLACESEL,
                                        TRUE, ( LPARAM )rep );
                        break;
                }
            }
            else
                SendMessage ( hRich, EM_REPLACESEL, TRUE, ( LPARAM )rep );
        }
    }
    else
    {
        if ( ( rep == NULL ) || ( prompt == TRUE ) )
        {
            #ifdef UNICODE
                StringCchPrintf ( temp, ARRAYSIZE(temp),
                    L"No more matches for \"%ls\"", txt );
            #else
                StringCchPrintf ( temp, ARRAYSIZE(temp),
                    "No more matches for \"%s\"", txt );
            #endif
            ShowMessage ( hParent, temp, MB_OK, IDI_APP );
        }

        return FALSE;
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GetLineCount 
/*--------------------------------------------------------------------------*/
//           Type: DWORD 
//    Param.    1: HWND hRich : richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns # of lines in a richedit control
/*--------------------------------------------------------------------@@-@@-*/
DWORD Rich_GetLineCount ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    return (DWORD)SendMessage ( hRich, EM_GETLINECOUNT, 0, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GetCrtLine 
/*--------------------------------------------------------------------------*/
//           Type: DWORD 
//    Param.    1: HWND hRich : richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns the current line in a richedit control
/*--------------------------------------------------------------------@@-@@-*/
DWORD Rich_GetCrtLine ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    CHARRANGE   chr;

    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    return (DWORD)SendMessage ( hRich, EM_EXLINEFROMCHAR,
                                0, ( LPARAM )chr.cpMax );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GetSelText 
/*--------------------------------------------------------------------------*/
//           Type: size_t 
//    Param.    1: HWND hRich    : richedit control
//    Param.    2: TCHAR * dest  : buffer to write to
//    Param.    3: size_t cchMax : max. chars to write to dest
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Copies selection to dest. and returns # of copied chars.
//                 If some error occurs, returns -1 and dest[0] is null;
//                 if no selection or selection is greater than cchMax, returns
//                 selection length and dest[0] is null;
/*--------------------------------------------------------------------@@-@@-*/
size_t Rich_GetSelText ( HWND hRich, TCHAR * dest, size_t cchMax )
/*--------------------------------------------------------------------------*/
{
    CHARRANGE       chr;
    size_t          size;

    if ( dest == NULL || cchMax == 0 )
        return (size_t)-1;

    dest[0] = TEXT('\0');
    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    
    if ( chr.cpMax == -1 ) 
        return (size_t)-1;

    size = chr.cpMax - chr.cpMin;

    if ( size == 0 || size > cchMax )
        return size;

    return SendMessage ( hRich, EM_GETSELTEXT, 0, ( LPARAM )( TCHAR * )dest );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GetSelLen 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hRich : richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Returns selection length in a arichedit control
/*--------------------------------------------------------------------@@-@@-*/
int Rich_GetSelLen ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    CHARRANGE       chr;

    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    return ( chr.cpMax - chr.cpMin );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GetSel 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich      : richedit control
//    Param.    2: CHARRANGE * chr : pointer to struct to receive selection
//                                   start/end
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Fills a CHARRANGE struct with the limits of current
//                 selection
/*--------------------------------------------------------------------@@-@@-*/
void Rich_GetSel ( HWND hRich, CHARRANGE * chr )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )chr );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_SetSel 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich      : richedit control
//    Param.    2: CHARRANGE * chr : pointer to struct that holds selection
//                                   data
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Sets the selection in a richedit control, as indicated
//                 by chr
/*--------------------------------------------------------------------@@-@@-*/
void Rich_SetSel ( HWND hRich, CHARRANGE * chr )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hRich, EM_EXSETSEL, 0, ( LPARAM )chr );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_ReplaceSelText 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich       : richedit control
//    Param.    2: const TCHAR * buf: text to replace with
//    Param.    3: BOOL allow_undo  : wether operation is UNDO-able
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Replace the selection in  a richedit control with buf.
/*--------------------------------------------------------------------@@-@@-*/
void Rich_ReplaceSelText ( HWND hRich, const TCHAR * buf, BOOL allow_undo )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hRich, EM_REPLACESEL, (WPARAM)allow_undo, (LPARAM)buf );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_Print 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich             : richedit control
//    Param.    2: HWND hParent           : parent to richedit control
//    Param.    3: const TCHAR * doc_name : a name for the printing job
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Yes, this is what it takes to print from a richedit control
//                 using Win API :-)) If you use it, please give credit 8-)
//                 It isn't top notch, but from what I tested, works with my
//                 Canon and with Adobe and MS XPS virtual printers. YMMV :)
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_Print ( HWND hRich, HWND hParent, const TCHAR * doc_name )
/*--------------------------------------------------------------------------*/
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
    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    SendMessage ( hRich, EM_GETSCROLLPOS, 0, ( LPARAM )&pt );

    // initialize the PRINTDLG structure
    RtlZeroMemory ( &pd, sizeof(PRINTDLG) );
    pd.lStructSize  = sizeof(PRINTDLG);
    pd.hwndOwner    = hParent;
    pd.hDevMode     = NULL;
    pd.hDevNames    = NULL;
    pd.Flags        = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
    pd.nCopies      = 1;
    pd.nFromPage    = 1;
    pd.nToPage      = 1;
    pd.nMinPage     = 1;
    pd.nMaxPage     = 0xFFFF;

    // we have no selection, disable the "print selection"
    if ( chr.cpMin == chr.cpMax )
        pd.Flags |= PD_NOSELECTION;

    // display Print dialog
    if ( !PrintDlg ( &pd ) )
        return FALSE;                      

    hdc             = pd.hDC;

    // initialize the DOCINFO struct
    // will fail if no doc name!
    RtlZeroMemory ( &di, sizeof(DOCINFO) );
    di.cbSize       = sizeof(DOCINFO);
    di.lpszDocName  = ( doc_name == NULL ) ? TEXT("Untitled") : doc_name;  
    di.fwType       = 0;
    di.lpszDatatype = NULL;
    di.lpszOutput   = NULL;

    // start the spool job
    if ( !StartDoc ( hdc, &di ) )
    {
        if ( hdc )
            DeleteDC ( hdc );

        if ( pd.hDevMode )
            GlobalFree ( pd.hDevMode );

        if ( pd.hDevNames )
            GlobalFree ( pd.hDevNames );

        return FALSE;
    }

    // get returned device caps
    cxPhysOffset    = GetDeviceCaps ( hdc, PHYSICALOFFSETX );
    cyPhysOffset    = GetDeviceCaps ( hdc, PHYSICALOFFSETY );

    cxPhys          = GetDeviceCaps ( hdc, PHYSICALWIDTH );
    cyPhys          = GetDeviceCaps ( hdc, PHYSICALHEIGHT );

    // ppi ( 1 point = 1/72 inch = 20 twips)
    ppx             = GetDeviceCaps ( hdc, LOGPIXELSX );                        
    ppy             = GetDeviceCaps ( hdc, LOGPIXELSY ); // ppi

    if ( !SendMessage ( hRich, EM_SETTARGETDEVICE, ( WPARAM )hdc, 0 ) )
    {
        if ( hdc )
            DeleteDC ( hdc );

        if ( pd.hDevMode )
            GlobalFree ( pd.hDevMode );

        if ( pd.hDevNames )
            GlobalFree ( pd.hDevNames );

        return FALSE;
    }

    // start filling FORMATRANGE struct with req. bits
    fr.hdc          = hdc;
    fr.hdcTarget    = hdc;

    // set page rect to physical page size in twips.
    fr.rcPage.top    = 0;
    fr.rcPage.left   = 0;
    fr.rcPage.right  = MulDiv ( cxPhys, 1440, ppx );
    fr.rcPage.bottom = MulDiv ( cyPhys, 1440, ppy );

    // on virtual prn, offset will be 0.. on some, it's very small
    // try and get some sane values
    cxPhysOffset    = ( cxPhysOffset == 0 ) ? ppx >> 1 : cxPhysOffset;//200
    cyPhysOffset    = ( cyPhysOffset == 0 ) ? ppy >> 1 : cyPhysOffset;//400

    cxPhysOffset    = ( cxPhysOffset < 200 ) ? 200 : cxPhysOffset;//100
    cyPhysOffset    = ( cyPhysOffset < 400 ) ? 400 : cyPhysOffset;//200

    // set the rendering rectangle to the pintable area of the page.
    fr.rc.left      = MulDiv ( cxPhysOffset, 1440, ppx );
    fr.rc.right     = MulDiv ( cxPhys - cxPhysOffset, 1440, ppx ) ;//+
    fr.rc.top       = MulDiv ( cyPhysOffset, 1440, ppy );
    fr.rc.bottom    = MulDiv ( cyPhys - cyPhysOffset, 1440, ppy ) ;//+

    // save these >:-/ richedit WILL fuck them up during the so called
    // EM_FORMATRANGE simulation
    CopyRect ( &rc, &fr.rc );
    CopyRect ( &rc_page, &fr.rcPage );

    // see what's been selected in the prn dlg
    to_print = PRINT_ALL;

    if ( pd.Flags & PD_PAGENUMS )
        to_print = PRINT_PAG;

    if ( ( pd.Flags & PD_SELECTION ) && ( chr.cpMin != chr.cpMax ) )
        to_print = PRINT_SEL;

    num_pages       = 1;
    pages_begin     = 0;
    pages_end       = 0;
    fin_page        = 0;
    crt_page        = pd.nFromPage;

    // make font for the header
    hFont = CreateFont ( -MulDiv ( 10, ppy, 72 ), 0, 0, 0, FW_DONTCARE, TRUE,
                         FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                         CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,
                         VARIABLE_PITCH,TEXT("Courier New" ) );

    SelectObject ( fr.hdc, hFont );

    // select the entire contents.
    SendMessage ( hRich, EM_SETSEL, 0, ( LPARAM )-1 );
    // get the selection into a CHARRANGE.
    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&fr.chrg );  

    // soooo, let's see how many pages do we have
    // and get the charrange for printing
    switch ( to_print )
    {
        case PRINT_SEL:
            while ( fr.chrg.cpMin < fr.chrg.cpMax )
            {
                cpMin = (int)SendMessage ( hRich, EM_FORMATRANGE,
                                            FALSE, ( LPARAM )&fr );

                // we hit the page with the selection
                if ( cpMin >= chr.cpMin && fin_page == 0 )
                    fin_page = num_pages;   

                // but continue to see how many pages
                num_pages++;                                                           
                fr.chrg.cpMin = cpMin;
            }
            // restore original selection for print
            fr.chrg.cpMin = chr.cpMin;                         
            fr.chrg.cpMax = chr.cpMax;

            if ( fin_page == num_pages )
                fin_page++;

            crt_page = fin_page;
            // clear cache
            SendMessage ( hRich, EM_FORMATRANGE, FALSE, 0 );   
            // restore these...
            CopyRect ( &fr.rc, &rc );                          
            CopyRect ( &fr.rcPage, &rc_page );
            break;

        case PRINT_PAG:
            while ( fr.chrg.cpMin < fr.chrg.cpMax )
            {
                if ( num_pages == pd.nFromPage )
                    // we hit the "from" page, save it
                    pages_begin = fr.chrg.cpMin;      

                cpMin = (int)SendMessage ( hRich, EM_FORMATRANGE,
                                            FALSE, ( LPARAM )&fr );

                if ( num_pages == pd.nToPage )
                {
                    pages_end = cpMin;
                    break;
                }           // we hit the "to" page, we can go now
                
                num_pages++;
                fr.chrg.cpMin = cpMin;
            }
            // some sanity checks, set the selection for print
            // "from" page is in the woods
            if ( pd.nFromPage > num_pages )                                            
            {
                fr.chrg.cpMin = 0;
                fr.chrg.cpMax = 0;
                break;
            }

            fr.chrg.cpMin = pages_begin;

            if ( !(pd.nToPage > num_pages) )
               fr.chrg.cpMax = pages_end;
            else
               // "to" page is in the woods 
               pd.nToPage = (WORD)(num_pages-1);                                               

            fin_page = pd.nToPage;
            // clear cahe
            SendMessage ( hRich, EM_FORMATRANGE, FALSE, 0 );                           
            CopyRect ( &fr.rc, &rc );
            CopyRect ( &fr.rcPage, &rc_page );
            break;

        case PRINT_ALL:
            // save selection
            pages_begin = fr.chrg.cpMin;                                               
            pages_end = fr.chrg.cpMax;

            while ( fr.chrg.cpMin < fr.chrg.cpMax )
            {
                // just number the pages
                cpMin = (int)SendMessage ( hRich, EM_FORMATRANGE,
                                            FALSE, ( LPARAM )&fr );
                num_pages++;
                fr.chrg.cpMin = cpMin;
            }

            // restore selection for print
            fr.chrg.cpMin = pages_begin;                                               
            fr.chrg.cpMax = pages_end;
            fin_page = num_pages-1;
            // clear cache
            SendMessage ( hRich, EM_FORMATRANGE, FALSE, 0 );                           
            CopyRect ( &fr.rc, &rc );
            CopyRect ( &fr.rcPage, &rc_page );
            break;
    }

    // now do the actual printing
    while ( ( fr.chrg.cpMin < fr.chrg.cpMax ) && fSuccess )
    {
        // start page
        fSuccess = StartPage ( hdc ) > 0;                                               

        if ( !fSuccess )
            break;

        cpMin = (int)SendMessage ( hRich, EM_FORMATRANGE, TRUE, (LPARAM)&fr );
        // make the header
        StringCchPrintf ( temp, ARRAYSIZE(temp),
                            TEXT(" Page %d of %d"), crt_page, fin_page );

        barlen = lstrlen ( temp );

        if ( barlen > ARRAYSIZE(wunderbar)-3 )
            barlen = ARRAYSIZE(wunderbar)-3;

        i = barlen+3;

        while ( i-- )
            wunderbar[i] = TEXT('_');

        SelectObject ( fr.hdc, hFont );
        SetTextColor ( fr.hdc, GetSysColor ( COLOR_3DDKSHADOW ) );
        TextOut ( hdc, cxPhysOffset, cyPhysOffset-(ppy >> 2),
                    temp, lstrlen ( temp ) );//180
        TextOut ( hdc, cxPhysOffset, cyPhysOffset-(ppy >> 2),
                    wunderbar, barlen+2 );//180
        crt_page++;
        fr.chrg.cpMin = cpMin;
        // end page
        fSuccess = EndPage ( hdc ) > 0;                                                 
    }

    // clear cache
    SendMessage ( hRich, EM_FORMATRANGE, FALSE, 0 );                                    

    // finish the spool if ok
    if ( fSuccess )
        EndDoc ( hdc );                                                 
    // ..or not
    else
        AbortDoc ( hdc );                                                          

    // cleanup
    if ( hdc )
        DeleteDC ( hdc );

    if ( pd.hDevMode )
        GlobalFree ( pd.hDevMode );

    if ( pd.hDevNames )
        GlobalFree ( pd.hDevNames );

    if ( hFont )
        DeleteObject ( hFont );

    // restore selection and cursor position
    SendMessage ( hRich, EM_EXSETSEL, 0, ( LPARAM )&chr );
    SendMessage ( hRich, EM_SETSCROLLPOS, 0, ( LPARAM )&pt );

    return fSuccess;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_NewRichedit 
/*--------------------------------------------------------------------------*/
//           Type: HWND 
//    Param.    1: HINSTANCE hInst       : current mod. instance
//    Param.    2: WNDPROC * old_proc    : address of var. to receive old
//                                         wndproc, if subclassing.May be NULL
//    Param.    3: WNDPROC new_proc      : new wnd proc (if subclassing).
//                                         May be NULL
//    Param.    4: HWND hParent          : parent for richedit (may be NULL)
//    Param.    5: INT_PTR richid        : a unique ID for our richedit
//    Param.    6: HFONT * pfont         : address of a HFONT to receive new
//                                         font if old one is NULL
//    Param.    7: BOOL wrap             : wordwrap or not
//    Param.    8: BOOL margin           : sel. margin or not
//    Param.    9: const TCHAR * inifile : optional config to load fg/bg
//                                         colors from
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Creates a new richedit control and returns a HWND to it,
//                 if successful. Or NULL if not =)
/*--------------------------------------------------------------------@@-@@-*/
HWND Rich_NewRichedit ( HINSTANCE hInst, WNDPROC * old_proc, WNDPROC new_proc,
                        HWND hParent, INT_PTR richid,
                        HFONT * pfont, BOOL wrap, BOOL margin,
                        const TCHAR * inifile )
/*--------------------------------------------------------------------------*/
{
    HWND            hRich;
    ULONG_PTR       style;
    DWORD           options;
    COLORREF        cr;
    CHARFORMAT      cf;

    if ( hInst == NULL || pfont == NULL )
        return NULL;

    options = RICH_STYLE;

    if ( margin )
        options |= ES_SELECTIONBAR;

    hRich = CreateWindowEx ( 0, RED_CLASS, 0,
                             options, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             hParent, ( HMENU )richid, hInst,
                             NULL
                           );

    if ( !hRich ) 
        return NULL;

    style = GetClassLongPtr ( hRich, GCL_STYLE );

    if ( style )
    {
        style &= ~( CS_HREDRAW|CS_VREDRAW|CS_PARENTDC );
        style |= ( CS_BYTEALIGNWINDOW|CS_BYTEALIGNCLIENT );
        SetClassLongPtr ( hRich, GCL_STYLE, style );
    }

    if ( *pfont == NULL )
    {
        *pfont = CreateFont ( -13, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                TEXT("Courier New")
                            );
    }

    SendMessage ( hRich, WM_SETFONT, (WPARAM)*pfont, TRUE );

    SendMessage ( hRich, EM_SETEVENTMASK, 0,
        ENM_KEYEVENTS|ENM_CHANGE|ENM_SELCHANGE/*|ENM_MOUSEEVENTS*/ );

    SendMessage ( hRich, EM_EXLIMITTEXT, 0, 60000000 ); // ~60MB of text :-)

    if ( new_proc != NULL )
        if ( old_proc != NULL )
            *old_proc = ( WNDPROC )
                SetWindowLongPtr ( hRich, GWLP_WNDPROC, (LONG_PTR)new_proc );

    if ( GetPrivateProfileStruct ( TEXT("editor"),
            TEXT("bgcolor"), &cr, sizeof ( cr ), inifile ) )
                SendMessage ( hRich, EM_SETBKGNDCOLOR, 0, cr );

    if ( GetPrivateProfileStruct ( TEXT("editor"),
            TEXT("fgcolor"), &cr, sizeof ( cr ), inifile ) )
    {
        RtlZeroMemory ( &cf, sizeof ( cf ) );
        cf.cbSize       = sizeof ( cf );
        cf.dwMask       = CFM_COLOR;
        cf.crTextColor  = cr;
        SendMessage ( hRich, EM_SETCHARFORMAT, SCF_ALL, ( LPARAM )&cf );
        SendMessage ( hRich, EM_EMPTYUNDOBUFFER, 0, 0 );
    }

    SendMessage ( hRich, EM_SETTEXTMODE,
        TM_PLAINTEXT|TM_MULTILEVELUNDO|TM_MULTICODEPAGE, 0 );

    SendMessage ( hRich, EM_SETTYPOGRAPHYOPTIONS,
        TO_SIMPLELINEBREAK, TO_SIMPLELINEBREAK );
    //SendMessage ( hRich, EM_SETEDITSTYLEEX,
    //  SES_EX_NOACETATESELECTION, SES_EX_NOACETATESELECTION ); // no effect
    //SendMessage ( hRich, EM_SETEDITSTYLE,
    //  SES_MULTISELECT, SES_MULTISELECT ); // Win8+
    // faster op., no multilevel undo
    //SendMessage ( hRich, EM_SETEDITSTYLE,
    //  SES_EMULATESYSEDIT, SES_EMULATESYSEDIT ); 

    Rich_WordWrap ( hRich, wrap );
    SendMessage ( hRich, EM_SETMODIFY, 0, 0 );

    return hRich;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_Streamin 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich                      : richedit control
//    Param.    2: HWND pb_parent                  : parent to the optional
//                                                   progress bar; if NULL,
//                                                   no pb is created
//    Param.    3: EDITSTREAMCALLBACK streamin_proc: callback for EM_STREAMIN
//    Param.    4: int encoding                    : encoding, as defined in
//                                                   encoding.h
//    Param.    5: TCHAR * fname                   : file to open
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Loads a file's contents in a richedit control by sending
//                 EM_STREAMIN message to it. The streaminPproc will receive
//                 a pointer to a STREAM_AUX_DATA structure ( the est.dwCookie
//                 member of the EDITSTREAM struct passed to EM_STREAMIN ).
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_Streamin ( HWND hRich, HWND pb_parent,
    EDITSTREAMCALLBACK streamin_proc, int encoding, TCHAR * fname )
/*--------------------------------------------------------------------------*/
{
    HANDLE              hFile;
    HWND                hpbar;
    EDITSTREAM          est;
    DWORD               fsize;
    STREAM_AUX_DATA     sdata;

    hFile = CreateFile ( fname, GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );

    /*
    hFile = CreateFile ( fname, GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );    
    */

    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;

    fsize = GetFileSize ( hFile, NULL );
    hpbar = ( pb_parent == NULL )
                ? NULL : PB_NewProgressBar ( pb_parent, 60, 150 );

    sdata.fsize = fsize;
    sdata.hfile = hFile;
    sdata.hpbar = hpbar;
    sdata.fencoding = ( encoding == -1 ) ? E_DONT_BOTHER : encoding ;

    est.dwCookie        = (DWORD_PTR)&sdata;
    est.dwError         = 0;
    est.pfnCallback     = streamin_proc;

    SendMessage ( hRich, EM_STREAMIN, SF_TEXT|SF_UNICODE, ( LPARAM )&est );

    if ( hpbar )
        DestroyWindow ( hpbar );

    CloseHandle ( hFile );

    return ( est.dwError == 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_SelStreamin 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich                      : richedit control
//    Param.    2: HWND pb_parent                  : parent to the optional
//                                                   progress bar; if NULL,
//                                                   no pb is created.
//    Param.    3: EDITSTREAMCALLBACK streamin_proc: callback for EM_STREAMIN
//    Param.    4: int encoding                    : encoding
//    Param.    5: TCHAR * fname                   : file to stremin
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_SelStreamin ( HWND hRich, HWND pb_parent,
    EDITSTREAMCALLBACK streamin_proc, int encoding, TCHAR * fname )
/*--------------------------------------------------------------------------*/
{
    HANDLE              hFile;
    HWND                hpbar;
    EDITSTREAM          est;
    DWORD               fsize;
    STREAM_AUX_DATA     sdata;

    hFile = CreateFile ( fname, GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );

    /*
    hFile = CreateFile ( fname, GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );    
    */

    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;

    fsize = GetFileSize ( hFile, NULL );
    hpbar = ( pb_parent == NULL )
                ? NULL : PB_NewProgressBar ( pb_parent, 60, 150 );

    sdata.fsize = fsize;
    sdata.hfile = hFile;
    sdata.hpbar = hpbar;
    sdata.fencoding = ( encoding == -1 ) ? E_DONT_BOTHER : encoding;

    est.dwCookie        = (DWORD_PTR)&sdata;
    est.dwError         = 0;
    est.pfnCallback     = streamin_proc;

    SendMessage ( hRich, EM_STREAMIN,
        SF_TEXT|SFF_SELECTION|SF_UNICODE, ( LPARAM )&est );

    if ( hpbar )
        DestroyWindow ( hpbar );

    CloseHandle ( hFile );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_Streamout 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich                       : richedit control
//    Param.    2: HWND pb_parent                   : parent to the optional
//                                                    progress bar; if NULL,
//                                                    no pb is created.
//    Param.    3: EDITSTREAMCALLBACK streamout_proc: callback for
//                                                    EM_STREAMOUT message
//    Param.    4: int encoding                     : file encoding
//    Param.    5: TCHAR * fname                    : filename
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_Streamout ( HWND hRich, HWND pb_parent,
    EDITSTREAMCALLBACK streamout_proc, int encoding, TCHAR * fname )
/*--------------------------------------------------------------------------*/
{
    HANDLE              hFile;
    HWND                hpbar;
    EDITSTREAM          est;
    DWORD               fsize;
    STREAM_AUX_DATA     sdata;

    hFile = CreateFile ( fname, GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );

    /*
    hFile = CreateFile ( fname, GENERIC_WRITE,
        FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );    
    */

    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;

    fsize = GetWindowTextLength ( hRich );
    hpbar = ( pb_parent == NULL )
        ? NULL : PB_NewProgressBar ( pb_parent, 60, 150 );

    sdata.fsize = fsize;
    sdata.hfile = hFile;
    sdata.hpbar = hpbar;
    sdata.fencoding = ( encoding == -1 ) ? E_DONT_BOTHER : encoding ;

    est.dwCookie        = (DWORD_PTR)&sdata;
    est.dwError         = 0;
    est.pfnCallback     = streamout_proc;

    SendMessage ( hRich, EM_STREAMOUT, SF_TEXT|SF_UNICODE, ( LPARAM )&est );

    if ( hpbar ) 
        DestroyWindow ( hpbar );

    CloseHandle ( hFile );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_SelStreamout 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich                       : richedit control
//    Param.    2: HWND pb_parent                   : parent to the optional
//                                                    progress bar; if NULL,
//                                                    no pb is created.
//    Param.    3: EDITSTREAMCALLBACK streamout_proc: callback for 
//                                                    EM_STREAMOUT message
//    Param.    4: int encoding                     : file encoding
//    Param.    5: TCHAR * fname                    : filename
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Saves ("streams out") current richedit selection to the
//                 specified filename, using a EM_STREAMOUT message.
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_SelStreamout ( HWND hRich, HWND pb_parent,
    EDITSTREAMCALLBACK streamout_proc, int encoding, TCHAR * fname )
/*--------------------------------------------------------------------------*/
{
    HANDLE              hFile;
    HWND                hpbar;
    EDITSTREAM          est;
    DWORD               fsize;
    STREAM_AUX_DATA     sdata;

    hFile = CreateFile ( fname, GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );

    /*
    hFile = CreateFile ( fname, GENERIC_WRITE,
        FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );    
    */

    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;

    fsize = Rich_GetSelLen ( hRich );
    hpbar = ( pb_parent == NULL )
        ? NULL : PB_NewProgressBar ( pb_parent, 60, 150 );

    sdata.fsize = fsize;
    sdata.hfile = hFile;
    sdata.hpbar = hpbar;
    sdata.fencoding = ( encoding == -1 ) ? E_DONT_BOTHER : encoding ;

    est.dwCookie        = (DWORD_PTR)&sdata;
    est.dwError         = 0;
    est.pfnCallback     = streamout_proc;

    SendMessage ( hRich, EM_STREAMOUT,
        SF_TEXT|SFF_SELECTION|SF_UNICODE, ( LPARAM )&est );

    if ( hpbar ) 
        DestroyWindow ( hpbar );

    CloseHandle ( hFile );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_SetFontNoCF 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich : richedit control
//    Param.    2: HFONT font : initialized font object to pass to WM_SETFONT
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Set richedit font
/*--------------------------------------------------------------------@@-@@-*/
void Rich_SetFontNoCF ( HWND hRich, HFONT font )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hRich, WM_SETFONT, ( WPARAM ) font, MAKELPARAM (TRUE,0) );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_SetFont 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich            : richedit control
//    Param.    2: HWND hParent          : parent for ChooseFont dlg
//                                         (may be null)
//    Param.    3: HFONT * pfont         : pointer to a font object to be
//                                         initialized after ChooseFont
//                                         returns
//    Param.    4: const TCHAR * inifile : optional config to load/save to
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Set a richedit font via the ChooseFont dlg. If a config
//                 file is specified, tries to load/save LOGFONT/CHOOSEFONT
//                 structs from/to it.
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_SetFont ( HWND hRich, HWND hParent, 
    HFONT * pfont, const TCHAR * inifile )
/*--------------------------------------------------------------------------*/
{
    CHOOSEFONT  cf;
    LOGFONT     lf;

    if ( hRich == NULL || pfont == NULL )
        return FALSE;

    RtlZeroMemory ( &cf, sizeof ( cf ) );
    RtlZeroMemory ( &lf, sizeof ( lf ) );

    if ( inifile != NULL )
    {
        GetPrivateProfileStruct ( TEXT("editor"),
            TEXT("log_font"), &lf, sizeof(LOGFONT), inifile );

        GetPrivateProfileStruct ( TEXT("editor"),
            TEXT("choose_font"), &cf, sizeof(CHOOSEFONT), inifile );
    }

    cf.lStructSize  = sizeof ( cf );
    cf.hwndOwner    = hParent;
    cf.lpLogFont    = &lf;
    cf.Flags        = CF_EFFECTS|CF_INITTOLOGFONTSTRUCT|
                        CF_SCREENFONTS|CF_NOVECTORFONTS;

    if ( ChooseFont ( &cf ) )
    {
        DeleteObject ( *pfont );
        *pfont = CreateFontIndirect ( &lf );

        SendMessage ( hRich, WM_SETFONT, ( WPARAM ) *pfont,
                        MAKELPARAM (TRUE,0) );

        Rich_SetFGColor ( hRich, NULL, cf.rgbColors, inifile );

        if ( inifile != NULL )
        {
            WritePrivateProfileStruct ( TEXT("editor"),
                TEXT("log_font"), cf.lpLogFont, sizeof ( LOGFONT ), inifile );

            WritePrivateProfileStruct ( TEXT("editor"),
                TEXT("choose_font"), &cf, sizeof ( CHOOSEFONT ), inifile );
        }

        return TRUE;
    }

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_ToggleSelBar 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HINSTANCE hInst       : 
//    Param.    2: WNDPROC * old_proc    : 
//    Param.    3: WNDPROC new_proc      : 
//    Param.    4: HWND * phrich         : 
//    Param.    5: HWND hParent          : 
//    Param.    6: int rich_id           : 
//    Param.    7: HFONT * pfont         : 
//    Param.    8: BOOL wrap             : 
//    Param.    9: BOOL margin           : 
//    Param.   10: const TCHAR * inifile : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: You have to destroy and recreate the control to change
//                 this feature. No comment :-)
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_ToggleSelBar ( HINSTANCE hInst, WNDPROC * old_proc,
                            WNDPROC new_proc,
                            HWND * phrich, HWND hParent,
                            int rich_id, HFONT * pfont,
                            BOOL wrap, BOOL margin,
                            const TCHAR * inifile )
/*--------------------------------------------------------------------------*/
{
    INT_PTR             size, alloc_size;
    void                * buf;
    TEXTRANGE           tr;
    CHARRANGE           chr;
    POINT               pt;
    BOOL                allow_undo;

    buf                 = NULL;
    size                = GetWindowTextLength ( *phrich );
    alloc_size          = 0;

    SendMessage ( *phrich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    SendMessage ( *phrich, EM_GETSCROLLPOS, 0, ( LPARAM )&pt );

    if ( size != 0 )
    {
        alloc_size    = size * sizeof(WCHAR);
        buf = alloc_and_zero_mem ( ALIGN_4K(alloc_size) );

        if ( buf == NULL )
            return FALSE;

        tr.chrg.cpMin = 0;
        tr.chrg.cpMax = (DWORD)size;
        tr.lpstrText  = (TCHAR *)buf;
        SendMessage ( *phrich, EM_GETTEXTRANGE, 0, (LPARAM)&tr );
    }

    DestroyWindow ( *phrich );
    *phrich = Rich_NewRichedit ( hInst, old_proc, new_proc, hParent,
        rich_id, pfont, wrap, margin, inifile );

    if ( size != 0 )
    {
        SetWindowText ( *phrich, (TCHAR *)buf ); //EM_SETTEXTEX?
        allow_undo = (alloc_size > 0x00400000) ? FALSE : TRUE;

        if ( !allow_undo  )
            SendMessage ( *phrich, EM_EMPTYUNDOBUFFER, (WPARAM)0, (LPARAM)0 );

        free_mem ( buf );
    }

    SetFocus ( *phrich );
    SendMessage ( *phrich, EM_EXSETSEL, 0, ( LPARAM )&chr );
    SendMessage ( *phrich, EM_SETSCROLLPOS, 0, ( LPARAM )&pt );
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_WordWrap 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich: richedit control
//    Param.    2: BOOL wrap : duh :)
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Unlike the sel. bar case, for wordwrap you can get away
//                 with a bit of voodoo, by setting the break line limit for
//                 the printing target device to some crazy value.
/*--------------------------------------------------------------------@@-@@-*/
void Rich_WordWrap ( HWND hRich, BOOL wrap )
/*--------------------------------------------------------------------------*/
{
    if ( wrap )
        SendMessage ( hRich, EM_SETTARGETDEVICE, 0, 0 );
    else
        SendMessage ( hRich, EM_SETTARGETDEVICE, 0, 100000 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_SaveSelandcurpos 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich           : richedit control
//    Param.    2: SELANDCUR_POS * pscp : pointer to structure to receive
//                                        data
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Saves selection and caret pos.
/*--------------------------------------------------------------------@@-@@-*/
void Rich_SaveSelandcurpos ( HWND hRich, SELANDCUR_POS * pscp )
/*--------------------------------------------------------------------------*/
{
    if ( pscp == NULL || hRich == NULL )
        return;

    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&pscp->chr );
    SendMessage ( hRich, EM_GETSCROLLPOS, 0, ( LPARAM )&pscp->pt );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_RestoreSelandcurpos 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich                 : richedit control
//    Param.    2: const SELANDCUR_POS * pscp : pointer to structure with
//                                              saved data
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Restores selection and caret position.
/*--------------------------------------------------------------------@@-@@-*/
void Rich_RestoreSelandcurpos ( HWND hRich, const SELANDCUR_POS * pscp )
/*--------------------------------------------------------------------------*/
{
    if ( pscp == NULL || hRich == NULL )
        return;

    SendMessage ( hRich, EM_EXSETSEL, 0, ( LPARAM )&pscp->chr );
    SendMessage ( hRich, EM_SETSCROLLPOS, 0, ( LPARAM )&pscp->pt );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_CreateShowCaret 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich   : richedit control
//    Param.    2: int cw       : caret width
//    Param.    3: int ch       : caret height
//    Param.    4: BOOL insmode : insert or not
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Useless code, but I like block carets :))
/*--------------------------------------------------------------------@@-@@-*/
BOOL Rich_CreateShowCaret ( HWND hRich, int cw, int ch, BOOL insmode )
/*--------------------------------------------------------------------------*/
{
    BOOL  result;

    result = CreateCaret ( hRich, (insmode)
        ? ((HBITMAP) 1) : ((HBITMAP) NULL), cw, ch );

    ShowCaret ( hRich );
    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_GetFontHeight 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hRich : richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Retrieve font height, in points
/*--------------------------------------------------------------------@@-@@-*/
int Rich_GetFontHeight ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    CHARFORMAT      cf;
    int             ppy;
    HDC             hdc;

    hdc = GetDC ( hRich );

    if ( hdc == NULL )
        return -1;

    ppy = GetDeviceCaps ( hdc, LOGPIXELSY );

    RtlZeroMemory ( &cf, sizeof ( cf ) );
    cf.cbSize = sizeof ( cf );
    cf.dwMask = CFM_SIZE;

    if ( !SendMessage ( hRich, EM_GETCHARFORMAT,
            (WPARAM)SCF_DEFAULT, (LPARAM)&cf ) )
                return -1;

    return MulDiv ( cf.yHeight, ppy, 1440 ); //  point = 1/72 inch = 20 twips
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Rich_ClearText 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hRich : richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Clear and reset a richedit control
/*--------------------------------------------------------------------@@-@@-*/
void Rich_ClearText ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hRich, EM_SETTEXTMODE,
        TM_PLAINTEXT|TM_MULTILEVELUNDO|TM_MULTICODEPAGE, 0 );

    SendMessage ( hRich, WM_SETTEXT, 0, 0 );
    SendMessage ( hRich, EM_FORMATRANGE, FALSE, 0 );  // clear cache
}
