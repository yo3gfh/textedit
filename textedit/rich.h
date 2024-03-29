
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

#define         RICH_STYLE          WS_VISIBLE|WS_CHILDWINDOW|WS_CLIPSIBLINGS \
                                    |WS_VSCROLL|WS_HSCROLL|ES_AUTOHSCROLL \
                                    |ES_AUTOVSCROLL|ES_MULTILINE|ES_NOHIDESEL

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
        #error "Cannot use richedit version 4.1 (msftedit) on ANSI;"  
        #error "Please define UNICODE or undef HAVE_MSFTEDIT;"
    #else
        #define RED_CLASS RICHEDIT_CLASS
        #define RICH_DLL  TEXT("riched20.dll")
    #endif
#endif

typedef BOOL ( CALLBACK * CVTTABSPROC ) 
    ( DWORD/*tabstops*/, DWORD/*done tabs*/, LPARAM /*user_def*/ );

typedef BOOL ( CALLBACK * FINDALLPROC ) 
    ( BOOL, /*valid*/TCHAR */*buf*/, DWORD /*line*/, LPARAM /*user_def*/ );

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

extern HWND Rich_NewRichedit ( HINSTANCE hInst, WNDPROC * old_proc, 
    WNDPROC new_proc, HWND hParent, INT_PTR richid, 
    HFONT * pfont, BOOL wrap, BOOL margin, const TCHAR * inifile );

extern void Rich_GetCaretPos ( HWND hRich, CARET_POS * cpos );
extern void Rich_SetModified ( HWND hRich, BOOL state );
extern BOOL Rich_GotoLine ( HWND hRich, DWORD line );
extern DWORD Rich_LineIndex ( HWND hRich, DWORD line );
extern DWORD Rich_GetLineLength ( HWND hRich, DWORD line );
extern DWORD Rich_CrtLineIndex ( HWND hRich );
extern WORD Rich_GetLineText ( HWND hRich, DWORD line, TCHAR * dest, 
    WORD cchdest );
extern BOOL Rich_SetFont ( HWND hRich, HWND hparent, HFONT * pfont, 
    const TCHAR * inifile );
extern void Rich_SetFontNoCF ( HWND hRich, HFONT font );

extern BOOL Rich_ToggleSelBar ( HINSTANCE hInst, WNDPROC * old_proc, 
    WNDPROC new_proc, HWND * phrich, HWND hparent, int rich_id, 
    HFONT * pfont, BOOL wrap, BOOL margin, const TCHAR * inifile );

extern void Rich_WordWrap ( HWND hRich, BOOL wrap );
extern BOOL Rich_SetBGColor ( HWND hRich, HWND hparent, COLORREF color,
    const TCHAR * inifile );
extern BOOL Rich_SetFGColor ( HWND hRich, HWND hparent, COLORREF color, 
    const TCHAR * inifile );
extern BOOL Rich_FindNext ( HWND hRich, HWND hparent, BOOL fwd );
extern void Rich_ConvertTabsToSpaces ( HWND hRich, DWORD tabstops, 
    CVTTABSPROC cbproc, LPARAM lParam );
extern BOOL Rich_ConvertCase ( HWND hRich, BOOL lower );
extern BOOL Rich_Streamin ( HWND hRich, HWND pb_parent, 
    EDITSTREAMCALLBACK streamin_proc, int encoding, TCHAR * fname );
extern BOOL Rich_SelStreamin ( HWND hRich, HWND pb_parent, 
    EDITSTREAMCALLBACK streamin_proc, int encoding, TCHAR * fname );
extern BOOL Rich_Streamout ( HWND hRich, HWND pb_parent, 
    EDITSTREAMCALLBACK streamout_proc, int encoding, TCHAR * fname );
extern BOOL Rich_SelStreamout ( HWND hRich, HWND pb_parent, 
    EDITSTREAMCALLBACK streamout_proc, int encoding, TCHAR * fname );
extern BOOL Rich_GetModified ( HWND hRich );
extern BOOL Rich_CanUndo ( HWND hRich );
extern BOOL Rich_CanRedo ( HWND hRich );
extern BOOL Rich_CanPaste ( HWND hRich );
extern BOOL Rich_FindReplaceText ( HWND hRich, HWND hparent, TCHAR * txt, 
    TCHAR * rep, BOOL mcase, BOOL whole, BOOL prompt, BOOL fwd );
extern DWORD Rich_FindText ( HWND hRich, TCHAR * txt, 
    FINDALLPROC fproc, BOOL mcase, BOOL whole, BOOL fwd, LPARAM lParam );
extern BOOL Rich_TabReplace ( HWND hRich, TCHAR * rep );
extern DWORD Rich_GetLineCount ( HWND hRich );
extern DWORD Rich_GetCrtLine ( HWND hRich );
extern size_t Rich_GetSelText ( HWND hRich, TCHAR * dest, size_t cchMax );
extern int Rich_GetSelLen ( HWND hRich );
extern BOOL Rich_Print ( HWND hRich, HWND hparent, const TCHAR * doc_name );
extern void Rich_SaveSelandcurpos ( HWND hRich, SELANDCUR_POS * pscp );
extern void Rich_RestoreSelandcurpos ( HWND hRich, 
    const SELANDCUR_POS * pscp );
extern BOOL Rich_CreateShowCaret ( HWND hRich, int cw, int ch, BOOL insmode );
extern int Rich_GetFontHeight ( HWND hRich );
extern void Rich_ClearText ( HWND hRich );
extern void Rich_GetSel ( HWND hRich, CHARRANGE * chr );
extern void Rich_SetSel ( HWND hRich, CHARRANGE * chr );
extern void Rich_ReplaceSelText ( HWND hRich, const TCHAR * buf, 
    BOOL allow_undo );
extern DWORD Rich_LineFromChar ( HWND hRich, DWORD ichar );

#endif
