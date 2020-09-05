
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

#ifndef _RICH_H
#define _RICH_H

#include        "main.h"
#include        "encoding.h"
#include        <richedit.h>

#if (_RICHEDIT_VER < 0x0200)
    #error "RICHEDIT version 1.x not supported (it doesn't know UNICODE)"
#endif

#define         CC_USERDEFINED      0xffffffff
#define         CC_INIRESTORE       0xfefefefe
#define         PRINT_ALL           1
#define         PRINT_SEL           2
#define         PRINT_PAG           3

#define         RICH_STYLE          WS_VISIBLE|WS_CHILDWINDOW|WS_CLIPSIBLINGS|WS_VSCROLL|WS_HSCROLL|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_MULTILINE|ES_NOHIDESEL/*|ES_DISABLENOSCROLL*/
#define         IDC_RICH            6666

// a few "new" richedit const. mostly unused
#define         EM_SETEDITSTYLEEX           (WM_USER + 275)
#define         EM_GETEDITSTYLEEX           (WM_USER + 276)
#define         SES_EX_NOTABLE              0x00000004
#define         SES_EX_HANDLEFRIENDLYURL    0x00000100
#define         SES_EX_NOTHEMING            0x00080000
#define         SES_EX_NOACETATESELECTION   0x00100000
#define         SES_EX_USESINGLELINE        0x00200000
#define         SES_EX_MULTITOUCH           0x08000000
#define         SES_EX_HIDETEMPFORMAT       0x10000000
#define         SES_EX_USEMOUSEWPARAM       0x20000000
#define         SES_MULTISELECT             0x08000000

#ifdef UNICODE
    #ifdef HAVE_MSFTEDIT
        #define RED_CLASS L"RichEdit50W"
        #define RICH_DLL  TEXT("msftedit.dll")
    #else
        #define RED_CLASS RICHEDIT_CLASS
        #define RICH_DLL  TEXT("riched20.dll")
    #endif
#else
    #ifdef HAVE_MSFTEDIT
        #error "Cannot use richedit version 4.1 (msftedit) on ANSI; please define UNICODE or undef HAVE_MSFTEDIT"
    #else
        #define RED_CLASS RICHEDIT_CLASS
        #define RICH_DLL  TEXT("riched20.dll")
    #endif
#endif

typedef BOOL ( CALLBACK * CVTTABSPROC ) ( DWORD/*tabstops*/, DWORD/*done tabs*/, DWORD_PTR lParam/*user_def*/ );

typedef struct _stream_aux_data
{
    HWND        hpbar;
    HANDLE      hfile;
    DWORD       fsize;
    ENC_TYPE    fencoding;
} STREAM_AUX_DATA;

typedef struct _caret_pos
{
    DWORD       c_line;
    DWORD       c_col;
} CARET_POS;

typedef struct _selandcur_pos
{
    CHARRANGE   chr;
    POINT       pt;
} SELANDCUR_POS;

extern HWND     Rich_NewRichedit            ( HINSTANCE hInst, WNDPROC * old_proc, WNDPROC new_proc,
                                                HWND hParent, int richid,
                                                HFONT * pfont, BOOL wrap, BOOL margin,
                                                const TCHAR * inifile );

extern void     Rich_GetCaretPos            ( HWND hrich, CARET_POS * cpos );
extern void     Rich_SetModified            ( HWND hrich, BOOL state );
extern BOOL     Rich_GotoLine               ( HWND hrich, DWORD line );
extern BOOL     Rich_SetFont                ( HWND hrich, HWND hparent, HFONT * pfont, const TCHAR * inifile );
extern void     Rich_SetFontNoCF            ( HWND hrich, HFONT font );

extern BOOL     Rich_ToggleSelBar           ( HINSTANCE hInst, WNDPROC * old_proc, WNDPROC new_proc,
                                                HWND * phrich, HWND hparent, int rich_id, HFONT * pfont,
                                                BOOL wrap, BOOL margin, const TCHAR * inifile );

extern void     Rich_WordWrap               ( HWND hrich, BOOL wrap );
extern BOOL     Rich_SetBGColor             ( HWND hrich, HWND hparent, COLORREF color, const TCHAR * inifile );
extern BOOL     Rich_SetFGColor             ( HWND hrich, HWND hparent, COLORREF color, const TCHAR * inifile );
extern BOOL     Rich_FindNext               ( HWND hrich, HWND hparent, BOOL fwd );
extern void     Rich_ConvertTabsToSpaces    ( HWND hrich, DWORD tabstops, CVTTABSPROC cbproc, DWORD_PTR lParam );
extern BOOL     Rich_ConvertCase            ( HWND hrich, BOOL lower );
extern BOOL     Rich_Streamin               ( HWND hRich, HWND pb_parent, EDITSTREAMCALLBACK streamin_proc, int encoding, TCHAR * fname );
extern BOOL     Rich_SelStreamin            ( HWND hrich, HWND pb_parent, EDITSTREAMCALLBACK streamin_proc, int encoding, TCHAR * fname );
extern BOOL     Rich_Streamout              ( HWND hrich, HWND pb_parent, EDITSTREAMCALLBACK streamout_proc, int encoding, TCHAR * fname );
extern BOOL     Rich_SelStreamout           ( HWND hrich, HWND pb_parent, EDITSTREAMCALLBACK streamout_proc, int encoding, TCHAR * fname );
extern BOOL     Rich_GetModified            ( HWND hrich );
extern BOOL     Rich_CanUndo                ( HWND hrich );
extern BOOL     Rich_CanRedo                ( HWND hrich );
extern BOOL     Rich_CanPaste               ( HWND hrich );
extern BOOL     Rich_FindReplaceText        ( HWND hrich, HWND hparent, TCHAR * txt, TCHAR * rep, BOOL mcase, BOOL whole, BOOL prompt, BOOL fwd );
extern BOOL     Rich_TabReplace             ( HWND hrich, TCHAR * rep );
extern DWORD    Rich_GetLineCount           ( HWND hrich );
extern DWORD    Rich_GetCrtLine             ( HWND hrich );
extern TCHAR    * Rich_GetSelText           ( HWND hrich );
extern int      Rich_GetSelLen              ( HWND hrich );
extern BOOL     Rich_Print                  ( HWND hrich, HWND hparent, const TCHAR * doc_name );
extern void     Rich_SaveSelandcurpos       ( HWND hrich, SELANDCUR_POS * pscp );
extern void     Rich_RestoreSelandcurpos    ( HWND hrich, const SELANDCUR_POS * pscp );
extern BOOL     Rich_CreateShowCaret        ( HWND hrich, int cw, int ch );
extern int      Rich_GetFontHeight          ( HWND hrich );
extern void     Rich_ClearText              ( HWND hrich );

#endif
