
/*
    TEXTEDIT, a (somehow) small multi-file text editor with UNICODE support
    -----------------------------------------------------------------------
    Copyright (c) 2002-2020 Adrian Petrila, YO3GFH

    Started this because, at the time, I thought Notepad was crap and how
    hard can it be to write something better, like PFE (Programmers File
    Editor)? Well, after breaking my teeth in UNICODE, turned out that
    Notepad was not all that crappy and my kung-fu was weak.

    So welcome to more crap :-))

    Some random thoughts:
    ---------------------
    Originally buit with OpenWatcom, moved it to the Pelle's C compiler
    system (http://www.smorgasbordet.com/pellesc/index.htm)

    After working with Richedit, I can now fully understand why ppl go all
    the way and write their own custom edit controls.

    After working with the comctl tab control, I can see why OOP is good =)
    After trying to find a way to resize parent+child windows flicker-free,
    I just want to be alone and cry :-)))

    But I'm not all that good, anyway lol.

    Hope you find something useful in this pile of text ( printing and
    unicode streaming comes to mind ), doesnt't seem to have too many bugs
    (here's hoping lol) and, for me at least, gets the job done.

    And the usual disclaimer: this isn't cutting edge, elegant,
    NSA grade code. Not even my grandpa-grade, if I come to think
    (he was a vet, God rest his soul, he couldn't write code but he could
    make a cow sing). I don't work as a full-time programmer (used to,
    20 years ago) so I can very well afford to be incompetent :-)))

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
    --------

    * Uses Richedit v4.1 to open/edit/save text in ANSI and UNICODE
    (UTF8/16/32-LE/BE with/without BOM). You can use Richedit 3.0/2.0
    if you want (undef HAVE_MSFTEDIT), but not 1.0 (does not support UNICODE).
    It works on win7, 8 and 10, should work on xp sp3 too. Please note that
    it does not use any features specific to latest Richedit versions (7.5
    on W8 and 8.5 on W10).
    
    * Multiple file support (tabbed).
    * User Defined menus (custom apps)
    * Execute external programs in a separate thread, optionally capturing
      console output (you may use it as a crude IDE).
    * View current selection as HEX.
    * Parse source files for C function definitions/declarations in order to
      generate header files
    * Generate a synoptic (as comment) for the C function under cursor or on
      selection
    * Unfortunately, it supports drag-n-drop
    * Exciting, misterious and surprisingly powerful bugs which will crash
      your box =) These are 100% genuine and are all mine!

    Building
    --------

    * open the workspace editor.ppw and you'll have all three projects
      (textedit, spawn and texeditSH)
      which you can build all at once. You can also open the individual 
      projects (.ppj) from each project folder.

    * spawn.exe is for executing external tools (masm, for example); 
      texteditSH is a MS Explorer context menu
      shell extension to open files in TextEdit; the program works without
      either of these.

    * there is another completely optional "feature", detour_selcolor.dll, 
      which is a API hook dll for GetSysColor,
      so you can customize text selection ("highlight") in a Richedit control.
      Starting with v2.0 and up, Richedit does not offer such a possibility
      anymore :-) so we must resort to such hackery :)). Basically, selection
      fore/back will be the same as the one from textedit.ini.

    * you can compile all projects as x64/x86, UNICODE/non UNICODE;
      please note: regardless of build type, the richedit control will be in
      UNICODE mode, to support the above mentioned file encodings.

    * IMPORTANT: build both the app and the extension in the same way
      (both as UNICODE or ANSI, 32 or 64 bit), otherwise interesting things
      start to happen :-))
*/

// textedit.c - main program module
// for UNICODE testing :-)
// http://www.columbia.edu/~fdc/utf8/index.html

//enum not used in switch, = used in conditional
#pragma warn(disable: 2008 2118 2228 2231 2030 2260) 

#include            "main.h"
#include            <strsafe.h>
#include            <windowsx.h>
#include            <shell32x.h>
#include            <shlwapi.h>
#include            <shlobj.h>
#include            <process.h>
#include            <htmlhelp.h>
#include            <tchar.h>

#include            "resource.h"
#include            "textedit.h"
#include            "encoding.h"
#include            "misc.h"
#include            "rich.h"
#include            "sbar.h"
#include            "pathutils.h"
#include            "menus.h"
#include            "config.h"
#include            "mem.h"
#include            "tabs.h"
#include            "cfunction.h"
#include            "threads.h"

// forward declarations

INT_PTR CALLBACK STR_StreamInProc
    ( DWORD_PTR dwCookie,LPBYTE pbBuff, LONG cb, LONG * pcb );

INT_PTR CALLBACK STR_StreamOutProc
    ( DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb );

INT_PTR CALLBACK DLG_GotoDlgProc 
    ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

INT_PTR CALLBACK DLG_FindDlgProc 
    ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

INT_PTR CALLBACK DLG_AboutDlgProc 
    ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

INT_PTR CALLBACK DLG_TabsDlgProc 
    ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

INT_PTR CALLBACK DLG_EXTToolDlgProc 
    ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

INT_PTR CALLBACK DLG_OUTviewDlgProc 
    ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

INT_PTR CALLBACK DLG_HEXViewDlgProc 
    ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

BOOL DLG_OpenDlg 
    ( HWND hParent, TCHAR * filename, int size, BOOL whole, BOOL newtab );

BOOL DLG_SaveDlg 
    ( HWND hParent, TCHAR * filename, int size, BOOL whole );

HWND DLG_OutputWnd_Modeless 
    ( HWND hWnd, LPARAM lParam );

LRESULT CALLBACK WndProc 
    ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

LRESULT CALLBACK SC_OutwSubclassProc 
    ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

LRESULT CALLBACK SC_HexwSubclassProc 
    ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

LRESULT CALLBACK SC_RichSubclassProc 
    ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

LRESULT CALLBACK SC_TabSubclassProc 
    ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

BOOL CALLBACK FindTabsProc 
    ( DWORD tabstops, DWORD donetabs, LPARAM lParam );

BOOL CALLBACK FindAllProc 
    ( BOOL valid, TCHAR * linetext, DWORD line, LPARAM lParam );

BOOL CALLBACK FindCDefsProc 
    ( BOOL valid, DWORD lines, DWORD crtline, WPARAM wParam, LPARAM lParam );

BOOL CALLBACK FindCDeclsProc 
    ( BOOL valid, DWORD lines, DWORD crtline, WPARAM wParam, LPARAM lParam );

BOOL CALLBACK FindCDeclsSelProc 
    ( BOOL valid, DWORD lines, DWORD crtline, WPARAM wParam, LPARAM lParam );

BOOL CALLBACK FindCDefsSelProc 
    ( BOOL valid, DWORD lines, DWORD crtline, WPARAM wParam, LPARAM lParam );

BOOL CALLBACK ExecToolProc 
    ( const TCHAR * s, LPARAM lParam );

void                UpdateMenuStatus        ( void );
BOOL                QuitSaveCheck           ( void );

void UpdateTitlebar ( HWND hWnd, const TCHAR * fname, const TCHAR * wtitle );
BOOL MainWND_OnDropFiles ( HWND hWnd, WPARAM wParam, LPARAM lParam );
void UpdateCaretPos ( HWND hrich, BOOL resize_panels );
BOOL UpdateEncodingMenus ( HMENU menu, int index );

BOOL TD_UpdateTabDataEntry 
    ( TAB_DATA * pdata, DWORD flags, int index, int childid, int encoding, 
        int cmdshow, HWND hchild, const TCHAR * path );

BOOL TD_RebuildTabData ( TAB_DATA * pdata, int ex_element, int elements );

BOOL                Prompt_to_save          ( void );
void                OutwUpdateTitle         ( HWND hWnd, int listitems );

BOOL OutwInsertTimeMarker 
    ( HWND hList, TCHAR * start, TCHAR * end );

void                SizeOutWnd              ( HWND hParent );
void                UpdateModifiedStatus    ( BOOL resize_panels );
void                UpdateEncodingStatus    ( BOOL resize_panels );
BOOL                RemoveTab               ( HWND hTab );
void                CvtEnableMenus          ( BOOL state );
void                CFDeclEnableMenus       ( BOOL state );
void                CFDeclSelEnableMenus    ( BOOL state );
void                CFDefsEnableMenus       ( BOOL state );
void                CFDefsSelEnableMenus    ( BOOL state );
void                ETLastEnableMenus       ( BOOL state );
void                ETEnableMenus           ( BOOL state );
void                ETUserEnableMenus       ( BOOL state );
void                FAllEnableMenus         ( BOOL state );

void                Menu_NewInstance        ( void );
void                Menu_SaveSelection      ( HWND hparent );
void                Menu_WWrap              ( HWND hWnd );
void                Menu_Open               ( HWND hWnd );
BOOL                Menu_OpenInBrowser      ( HWND hWnd );
BOOL                Menu_WipeClipboard      ( HWND hWnd );
BOOL                Menu_MergeFile          ( HWND hWnd );
BOOL                Menu_RefreshFromDisk    ( HWND hWnd );
BOOL                Menu_Save               ( HWND hWnd );
BOOL                Menu_Saveas             ( HWND hWnd );
BOOL                Menu_New                ( HWND hWnd );
BOOL                Menu_ToggleMargin       ( HWND hWnd );
BOOL                Menu_Find               ( HWND hWnd );
BOOL                Menu_ConvertTabs        ( HWND hWnd );
BOOL                Menu_About              ( HWND hWnd );
BOOL                Menu_SetFont            ( HWND hWnd );
BOOL                Menu_Print              ( HWND hWnd );
BOOL                Menu_SetFGColor         ( HWND hWnd );
BOOL                Menu_SetBGColor         ( HWND hWnd );
BOOL                Menu_RunExtTool         ( HWND hWnd );
BOOL                Menu_RunLastCmd         ( HWND hWnd );
BOOL                Menu_RunUserCmd         ( INT_PTR menu_idx );
BOOL                Menu_HexView            ( HWND hWnd );
BOOL                Menu_Quit               ( HWND hWnd );
BOOL                Menu_Ontop              ( HWND hWnd );
BOOL                Menu_Properties         ( HWND hWnd );
BOOL                Menu_CFDefAll           ( HWND hWnd );
BOOL                Menu_CFDeclAll          ( HWND hWnd );
BOOL                Menu_CFDefSel           ( HWND hWnd );
BOOL                Menu_CFDeclSel          ( HWND hWnd );
BOOL                Menu_FindAll            ( HWND hWnd );

BOOL MainWND_OnNotify           ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnCreate           ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnSize             ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnInitMenuPopup    ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnCommand          ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnCopyData         ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnENDCVTABS        ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnUPDCVTABS        ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnENDCFDECL        ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnUPDCFDECL        ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnENDCFDEFS        ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnUPDCFDEFS        ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnENDCFDEFSSEL     ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnUPDCFDEFSSEL     ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnENDCFDECLSEL     ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnUPDCFDECLSEL     ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnENDEXECTOOL      ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnBUFEXECTOOL      ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnBUFCFDECL        ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnBUFCFDEFS        ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnBUFCFDECLSEL     ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnBUFCFDEFSSEL     ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnENDFALL          ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnUPDFALL          ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnBUFFALL          ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainWND_OnHelp             ( HWND hWnd, WPARAM wParam, LPARAM lParam );

BOOL OpenFileListFromCMDL       ( TCHAR ** filelist, int files );
BOOL OpenFileListFromOFD        ( TCHAR * filelist, int fofset, BOOL newtab );
void CycleTabs                  ( HWND hWnd, int direction );
HIMAGELIST InitImgList          ( void );

// you can judge a programmer by the # of globals :-))
HWND                g_hStatus;
HWND                g_hRich, g_hTab;
HWND                g_hMain;
HWND                g_hOutWnd    = NULL;
HWND                g_hOutwList  = NULL;

HMENU               g_hMainmenu, g_hFilemenu, g_hEditmenu, 
                    g_hToolsmenu, g_hViewmenu, g_hUsermenu;
HACCEL              g_hAccel;
HICON               g_hIcon, g_hSmallIcon;
HFONT               g_hFont      = NULL;

HINSTANCE           g_hInst;
HMODULE             g_hDll;
HIMAGELIST          g_hIml;

BOOL                g_binsmode   = TRUE;
BOOL                g_bmargin    = TRUE;
BOOL                g_bwrap      = FALSE;
BOOL                g_bcase      = FALSE;
BOOL                g_bwhole     = FALSE;
BOOL                g_bprompt    = TRUE;
BOOL                g_bfwd       = TRUE;
BOOL                g_bfall      = FALSE;
BOOL                g_ball       = FALSE;
BOOL                g_bontop     = FALSE;
BOOL                g_bcapture   = FALSE;
BOOL                g_bmenu_needs_update = TRUE;
BOOL                g_bnot_alone = FALSE;
volatile BOOL       g_thread_working = FALSE;

TCHAR               g_szbuf[TMAX_PATH];
TCHAR               g_sztextbuf[512];
TCHAR               g_szrepbuf[512];
TCHAR               g_szinifile[TMAX_PATH];
#ifdef HAVE_HTMLHELP
TCHAR               g_szchmfile[TMAX_PATH];
#endif

WNDPROC             g_rich_old_proc;
WNDPROC             g_outw_old_proc;
WNDPROC             g_hexw_old_proc;
WNDPROC             g_tab_old_proc;

INT_PTR             g_dwtabsize   = 4;

TCHAR               * g_szoldmenu;
int                 g_fontheight;
volatile int        g_curtab      = 0;
int                 g_fencoding   = E_UTF_8;
BOOL                g_bom_written = FALSE;

UINT                g_tid;
UINT_PTR            g_thandle;
INT_PTR             g_umenu_cnt;
INT_PTR             g_umenu_idx;
TAB_DATA            tab_data[MAX_TABS];
USER_MENU           user_menu[MAX_USER_MENUS];

EXEC_THREAD_DATA    etd;
FINDC_THREAD_DATA   ftd;
TABREP_THREAD_DATA  ttd;
FINDALL_THREAD_DATA atd;

/*-@@+@@--------------------------------------------------------------------*/
//       Function: WinMain 
/*--------------------------------------------------------------------------*/
//           Type: int WINAPI 
//    Param.    1: HINSTANCE hInstance    : 
//    Param.    2: HINSTANCE hPrevInstance: 
//    Param.    3: PSTR szCmdLine         : 
//    Param.    4: int iCmdShow           : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Main program loop
/*--------------------------------------------------------------------@@-@@-*/
int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR szCmdLine, int iCmdShow )
/*--------------------------------------------------------------------------*/
{
    HWND                    hWnd;
    MSG                     msg;
    WNDCLASSEX              wndclass;
    TCHAR                   ** arglist;
    DWORD                   x, y, w, h, sw, sh;
    int                     argc;
    RECT                    r;
    TCHAR                   temp[128];
    HMODULE                 hDetour = NULL;

    g_hInst                = hInstance;
    hDetour                = LoadLibrary ( DETOUR_DLL );
    g_hDll                 = LoadLibrary ( RICH_DLL );

    if ( !g_hDll )
    {
        // unable to find richedit/msft dll
        StringCchPrintf ( temp, ARRAYSIZE(temp), CCH_SPEC5, RICH_DLL ); 
        ShowMessage ( NULL, temp, MB_OK, IDI_APP );
        FreeLibrary ( hDetour );
        return 0;
    }

    InitCommonControls();
    g_bnot_alone = IsThereAnotherInstance ( app_classname );

    g_hIcon                = LoadIcon ( g_hInst, 
                                MAKEINTRESOURCE ( IDI_APP ) );

    g_hSmallIcon           = LoadIcon ( g_hInst, 
                                MAKEINTRESOURCE ( IDI_APP ) );

    g_hIml                 = InitImgList();

    g_hMainmenu            = LoadMenu ( g_hInst, 
                                MAKEINTRESOURCE ( IDR_MAINMENU ) );

    // save these for later, needed for menu status update
    g_hFilemenu            = GetSubMenu ( g_hMainmenu, IDMM_FILE );
    g_hEditmenu            = GetSubMenu ( g_hMainmenu, IDMM_EDIT );
    g_hToolsmenu           = GetSubMenu ( g_hMainmenu, IDMM_TOOLS );
    g_hViewmenu            = GetSubMenu ( g_hMainmenu, IDMM_VIEW );
    g_hUsermenu            = GetSubMenu ( g_hMainmenu, IDMM_USER );

    wndclass.cbSize        = sizeof ( WNDCLASSEX );
    wndclass.hInstance     = hInstance;
    wndclass.style         = WIN_CLASS;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hIcon         = g_hIcon;
    wndclass.hIconSm       = g_hSmallIcon;
    wndclass.hCursor       = LoadCursor ( NULL, IDC_ARROW );
    wndclass.hbrBackground = ( HBRUSH ) GetStockObject ( NULL_BRUSH );
    wndclass.lpszMenuName  = NULL; //we'll set it later
    // make only the first instance "magic" :-)
    wndclass.lpszClassName = g_bnot_alone ? app_classname2 : app_classname;

    if ( !RegisterClassEx ( &wndclass ) )
    {
        ShowMessage ( NULL, TEXT("Unable to register window class"), 
                        MB_OK, IDI_APP );

        FreeLibrary ( hDetour );
        DestroyMenu ( g_hMainmenu );
        return 0 ;
    }

    StringCchCopy ( g_szinifile, ARRAYSIZE(g_szinifile),
        IniFromModule ( g_hInst ) );

    #ifdef HAVE_HTMLHELP
    StringCchCopy ( g_szchmfile, ARRAYSIZE(g_szchmfile),
        ChmFromModule ( g_hInst ) );
    #endif
    
    g_hFont                = LoadFontFromInifile ( g_szinifile );

    if ( LoadWindowPosition ( &r, g_szinifile ) )
    {
        x = r.left;
        y = r.top;
        w = r.right - r.left;
        h = r.bottom - r.top;
    }
    else
    {
        sw = GetSystemMetrics ( SM_CXSCREEN );
        sh = GetSystemMetrics ( SM_CYSCREEN );

        w  = sw - ( sw >> 2 );
        h  = sh - ( sh >> 2 );
        x  = topxy ( w, sw );
        y  = topxy ( h, sh );
    }

    g_bmargin              = ( BOOL ) GetPrivateProfileInt ( TEXT("editor"),
                                         TEXT("margin"), 1, g_szinifile );
    g_bwrap                = ( BOOL ) GetPrivateProfileInt ( TEXT("editor"), 
                                        TEXT("wrap"), 0, g_szinifile );
    g_bontop               = ( BOOL ) GetPrivateProfileInt ( TEXT("window"), 
                                        TEXT("ontop"), 0, g_szinifile );

    hWnd = CreateWindowEx ( WS_EX_LEFT|WS_EX_ACCEPTFILES,
                            g_bnot_alone ? app_classname2 : app_classname,
                            TEXT(""),
                            WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
                            x,
                            y,
                            w,
                            h,
                            NULL,
                            NULL,
                            hInstance,
                            NULL
                          );

    if ( !hWnd )
    {
        ShowMessage ( NULL, TEXT("Could not create main window"),
                        MB_OK, IDI_APP );

        DeleteObject ( g_hFont );
        FreeLibrary ( hDetour );
        DestroyMenu ( g_hMainmenu );
        return 0;
    }

    SetMenu ( hWnd, g_hMainmenu );

    if ( g_bontop )
        SetWindowPos ( hWnd, HWND_TOPMOST,
            0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
    else
        SetWindowPos ( hWnd, HWND_NOTOPMOST,
            0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    if ( GetPrivateProfileInt ( TEXT("window"),
            TEXT("maximized"), 0, g_szinifile ) )
                ShowWindow ( hWnd, SW_MAXIMIZE );
    else
        ShowWindow ( hWnd, iCmdShow );

    UpdateWindow ( hWnd );
    g_hAccel                = LoadAccelerators ( g_hInst, 
                                MAKEINTRESOURCE ( IDR_ACCEL ) );
    g_hMain                 = hWnd;

    RtlZeroMemory ( g_sztextbuf, sizeof ( g_sztextbuf ) );
    RtlZeroMemory ( g_szrepbuf, sizeof ( g_szrepbuf ) );

    arglist = FILE_CommandLineToArgv ( GetCommandLine(), &argc );

    if ( argc >= 2 && arglist != NULL )
    {
        OpenFileListFromCMDL ( arglist, argc );

        RedrawWindow ( g_hTab, NULL, NULL, 
            RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
    }
    else
    {
        StringCchCopy ( g_szbuf, ARRAYSIZE(g_szbuf), TEXT("Untitled") );
        UpdateTitlebar ( g_hMain, g_szbuf, app_title );
    }

    while ( GetMessage ( &msg, NULL, 0, 0 ) )
    {
        if ( !TranslateAccelerator ( hWnd, g_hAccel, &msg ) )
        {
            if ( !IsWindow( g_hOutWnd ) || !IsDialogMessage (g_hOutWnd, &msg))
            {
                TranslateMessage ( &msg ) ;
                DispatchMessage ( &msg ) ;
            }
        }
        // let's try to do some iddle processing :)
        // LE: this works quite well until you load richedit with some heavy
        // shit, say multimega text, then the caret movement starts lagging
        // because of next lines
        /*if ( !PeekMessage ( &msg, NULL,0, 0, PM_NOREMOVE ) )
        {
            UpdateMenuStatus();
            UpdateEncodingMenus ( g_hFilemenu, tab_data[g_curtab].fencoding );
        }*/
    }

    DeleteObject ( g_hFont );
    ImageList_Destroy ( g_hIml );
    DestroyMenu ( g_hMainmenu );

    if ( arglist != NULL )
        GlobalFree ( arglist );

    if ( g_thandle != 0 )
        CloseHandle ( (HANDLE)g_thandle );

    FreeLibrary ( g_hDll );
    FreeLibrary ( hDetour );

    return msg.wParam;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SC_TabSubclassProc 
/*--------------------------------------------------------------------------*/
//           Type: LRESULT CALLBACK 
//    Param.    1: HWND hWnd     : 
//    Param.    2: UINT message  : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Tab control subclass procedure
/*--------------------------------------------------------------------@@-@@-*/
LRESULT CALLBACK SC_TabSubclassProc ( HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    switch ( message )
    {
        case WM_COMMAND:

            if ( LOWORD ( wParam ) == tab_data[g_curtab].child_id )
            {
                // if we're in some lengthy tab cvt op, pass on
                if ( HIWORD ( wParam ) == EN_CHANGE && !g_thread_working ) 
                {
                    UpdateModifiedStatus ( TRUE );
                    g_bmenu_needs_update = TRUE;
                }
                break;
            }
            break;

        case WM_LBUTTONDBLCLK:
            Menu_Properties ( g_hMain );
            break;

        case WM_MBUTTONUP: // simulate a shift+click on the tab header :-)
            PostMessage ( hWnd, WM_LBUTTONDOWN, 
                (WPARAM)MK_LBUTTON, lParam );

            PostMessage ( hWnd, WM_LBUTTONUP, 
                (WPARAM)MK_LBUTTON | MK_SHIFT, lParam );
            break;

        case WM_LBUTTONUP:
            // if we're in some lengthy tab cvt op, pass on
            if ( (wParam & MK_SHIFT) && !g_thread_working ) 
                RemoveTab ( hWnd );

            break;
    }

    return CallWindowProc ( g_tab_old_proc, hWnd, message, wParam, lParam );
}

#define MKCARET_HEIGHT(x) ((x+(x>>2))-(!(x&1)))

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SC_RichSubclassProc 
/*--------------------------------------------------------------------------*/
//           Type: LRESULT CALLBACK 
//    Param.    1: HWND hWnd     : 
//    Param.    2: UINT message  : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Richedit controls subclass procedure, mainly for caret and
//                 ctx menu :-)
/*--------------------------------------------------------------------@@-@@-*/
LRESULT CALLBACK SC_RichSubclassProc ( HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    POINT           pt;
    HMENU           hSub;
    LRESULT         res = 0;
    static BOOL     ctrl_pressed = FALSE;

    switch ( message )
    {
        // a bunch of msg's on which we set our caret
        case WM_SETFOCUS:
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_NOTIFY:
        case WM_PAINT:
        //case WM_CHAR:
        //case WM_UNICHAR:
        //case WM_KEYDOWN:
        //case WM_INPUTLANGCHANGE:
        //case WM_IME_COMPOSITION:
        case WM_HSCROLL:
        case WM_VSCROLL:
        case WM_SIZE:
            res = CallWindowProc ( g_rich_old_proc, 
                hWnd, message, wParam, lParam );
            // setting the custom caret (wider than default)
            Rich_CreateShowCaret ( hWnd, CARET_WIDTH, 
                MKCARET_HEIGHT(g_fontheight), g_binsmode ); 
            break;

        case WM_KEYUP:

            if ( wParam & VK_CONTROL )
                ctrl_pressed = FALSE;

            res = CallWindowProc ( g_rich_old_proc, hWnd, 
                message, wParam, lParam );
            break;

        case WM_KEYDOWN:

            if ( wParam == VK_INSERT )
                g_binsmode = !g_binsmode;

            if ( wParam & VK_CONTROL )
            {
                // checking the repeat rate
                //if ( (HIWORD(lParam) & KF_REPEAT) != 0 ) 
                // pass on spurious ctrl press by checking 
                // if prev. state was down
                //if ( (lParam & (1<<30))) 
                    ctrl_pressed = TRUE;
            }

            res = CallWindowProc ( g_rich_old_proc, hWnd, 
                message, wParam, lParam );
            Rich_CreateShowCaret ( hWnd, CARET_WIDTH, 
                MKCARET_HEIGHT(g_fontheight), g_binsmode );
            break;
        // you can comment this case to get back the original 
        // richedit "smooth" scroll (smooth my ass, lol), 
        // although I don't see why you'd want to do that...
        case WM_MOUSEWHEEL:

            if ( GET_WHEEL_DELTA_WPARAM(wParam) > 0 )
                SendMessage ( hWnd, EM_LINESCROLL, 0, -RICH_VERT_SCROLL );
            else
                SendMessage ( hWnd, EM_LINESCROLL, 0, RICH_VERT_SCROLL );

            res = TRUE;
            break;

        case WM_KILLFOCUS:
            res = CallWindowProc ( g_rich_old_proc, hWnd, 
                message, wParam, lParam );

            DestroyCaret();
            break;

        // simulate a lbutton down click to move the caret
        // but be polite and only if there's no selection :-)
        case WM_RBUTTONDOWN:

            if ( Rich_GetSelLen ( hWnd ) == 0 )
                PostMessage ( hWnd, WM_LBUTTONDOWN, wParam, lParam );

            res = CallWindowProc ( g_rich_old_proc, hWnd, 
                message, wParam, lParam );
            Rich_CreateShowCaret ( hWnd, CARET_WIDTH, 
                g_fontheight+4, g_binsmode );
            break;

        case WM_CONTEXTMENU:
            GetCursorPos ( &pt );

            if ( ctrl_pressed ) // ctrl+kb menu key - > open tools submenu
            {
                hSub = GetSubMenu ( g_hMainmenu, IDMM_TOOLS );
                //ShowMessage ( hWnd, TEXT("CTRL"), MB_OK, 0 );
                ctrl_pressed = FALSE;
            }
            else
                hSub = GetSubMenu ( g_hMainmenu, IDMM_EDIT );

            UpdateMenuStatus();

            TrackPopupMenuEx ( hSub, 
                TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, 
                    pt.x, pt.y, g_hMain, NULL );

            res = CallWindowProc ( g_rich_old_proc, hWnd, 
                message, wParam, lParam );

            Rich_CreateShowCaret ( hWnd, CARET_WIDTH, 
                g_fontheight+4, g_binsmode );
            break;

        default:
            res = CallWindowProc ( g_rich_old_proc, hWnd, 
                message, wParam, lParam );
            break;
    }

    return res;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SC_OutwSubclassProc 
/*--------------------------------------------------------------------------*/
//           Type: LRESULT CALLBACK 
//    Param.    1: HWND hWnd     : 
//    Param.    2: UINT message  : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Output Window subclass procedure
/*--------------------------------------------------------------------@@-@@-*/
LRESULT CALLBACK SC_OutwSubclassProc ( HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    static HMENU    hcpmenu, hsub;
    POINT           pt;
    DWORD           states[2] = { MF_GRAYED, MF_ENABLED };
    BOOL            state;

    switch ( message )
    {
        case WM_CONTEXTMENU:
            hcpmenu = LoadMenu ( g_hInst, MAKEINTRESOURCE ( IDR_OUTWMENU ) );

            if ( !hcpmenu )
                break;

            hsub    = GetSubMenu ( hcpmenu, 0 );
            state   = ( ListBox_GetCount ( hWnd ) != 0 );
            EnableMenuItem ( hsub, IDM_CPLINES, states[state] );
            EnableMenuItem ( hsub, IDM_OUTWCLEAR, states[state] );
            EnableMenuItem ( hsub, IDM_CP_SELALL, states[state] );
            state   = ( ListBox_GetSelCount ( hWnd ) > 0 );
            EnableMenuItem ( hsub, IDM_CPSEL, states[state] );
            GetCursorPos ( &pt );

            TrackPopupMenuEx ( hsub, 
                TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, 
                    pt.x, pt.y, hWnd, NULL );
            break;

        case WM_COMMAND:
            switch ( LOWORD ( wParam ) )
            {
                case IDM_CPLINES:
                    CopyListToClipboard ( g_hMain, hWnd );
                    break;

                case IDM_CPSEL:
                    CopyListSelToClipboard ( g_hMain, hWnd );
                    break;

                case IDM_CP_SELALL:
                    ListBox_SetSel ( hWnd, TRUE, -1 );
                    break;

                case IDM_OUTWCLEAR:
                    ListBox_ResetContent ( hWnd );
                    SetWindowText ( g_hOutWnd, 
                        TEXT("Output Window - 0 items") );
                    break;

                default:
                    break;

            }
            break;

        case WM_LBUTTONDBLCLK:
            CopyListToClipboard ( g_hMain, hWnd );
            break;

        case WM_DESTROY:

            if ( hcpmenu )
                DestroyMenu ( hcpmenu );

            break;

        default:
            break;
    }

    return CallWindowProc ( g_outw_old_proc, hWnd, 
        message, wParam, lParam );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SC_HexwSubclassProc 
/*--------------------------------------------------------------------------*/
//           Type: LRESULT CALLBACK 
//    Param.    1: HWND hWnd     : 
//    Param.    2: UINT message  : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Hexview Window subclass proc, for menus 
/*--------------------------------------------------------------------@@-@@-*/
LRESULT CALLBACK SC_HexwSubclassProc ( HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    static HMENU    hcpmenu, hsub;
    static BOOL     ctrl_pressed = FALSE;
    POINT           pt;
    DWORD           states[2] = { MF_GRAYED, MF_ENABLED };
    BOOL            state;

    switch ( message )
    {
        case WM_CONTEXTMENU:
            hcpmenu = LoadMenu ( g_hInst, MAKEINTRESOURCE ( IDR_OUTWMENU ) );

            if ( !hcpmenu )
                break;

            hsub = GetSubMenu ( hcpmenu, 0 );
            state = ( ListBox_GetCount ( hWnd ) != 0 );
            EnableMenuItem ( hsub, IDM_CPLINES, states[state] );
            GetCursorPos ( &pt );

            TrackPopupMenuEx ( hsub, 
                TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, 
                    pt.x, pt.y, hWnd, NULL );
            break;

        case WM_KEYUP:

            if ( wParam == VK_CONTROL )
                ctrl_pressed = FALSE;

            break;

        case WM_KEYDOWN:
            switch ( wParam )
            {
                case VK_CONTROL:
                    ctrl_pressed = TRUE;
                    break;

                case 0x43: // 'C'

                    if ( ctrl_pressed )
                        CopyListSelToClipboard ( g_hMain, hWnd );

                    break;

                case 0x41: // 'A'

                    if ( ctrl_pressed )
                        ListBox_SetSel ( hWnd, TRUE, -1 );

                    break;

                default:
                    break;

            }
            break;

        case WM_COMMAND:
            switch ( LOWORD ( wParam ) )
            {
                case IDM_CPLINES:
                    CopyListToClipboard ( g_hMain, hWnd );
                    break;

                case IDM_CPSEL:
                    CopyListSelToClipboard ( g_hMain, hWnd );
                    break;

                case IDM_CP_SELALL:
                    ListBox_SetSel ( hWnd, TRUE, -1 );
                    break;

                default:
                    break;

            }
            break;

        case WM_LBUTTONDBLCLK:
            CopyListToClipboard ( g_hMain, hWnd );
            break;

        case WM_DESTROY:

            if ( hcpmenu )
                DestroyMenu ( hcpmenu );

            break;

        default:
            break;
    }

    return CallWindowProc ( g_hexw_old_proc, hWnd,
        message, wParam, lParam );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: DLG_AboutDlgProc 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR CALLBACK 
//    Param.    1: HWND hDlg     : 
//    Param.    2: UINT uMsg     : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: wnd proc for Most important window =)
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CALLBACK DLG_AboutDlgProc ( HWND hDlg, UINT uMsg, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR   buf[64];

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDlg ( hDlg, g_hMain );
#ifdef UNICODE
            StringCchPrintf ( buf, ARRAYSIZE(buf), L"build: %ls", 
                FileVersionAsString ( NULL ));
#else
            StringCchPrintf ( buf, ARRAYSIZE(buf), "build: %s", 
                FileVersionAsString ( NULL ));
#endif
            SetDlgItemText ( hDlg, 1016, buf );
            break;

        case WM_CLOSE:
            EndDialog ( hDlg, 0 );
            break;

        case WM_COMMAND:
            if ( LOWORD ( wParam ) == IDCANCEL )
                SendMessage ( hDlg, WM_CLOSE, 0, 0 );
            break;

        default:
            return 0;
            break;  // why do you put a break after ret? 
                    // for the future when you'll make it a case =)
    }
    return 1;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: DLG_TabsDlgProc 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR CALLBACK 
//    Param.    1: HWND hDlg     : 
//    Param.    2: UINT uMsg     : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Wnd proc for cvt. tabs to spaces dlg
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CALLBACK DLG_TabsDlgProc ( HWND hDlg, UINT uMsg, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    BOOL    converted;
    UINT    result;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDlg ( hDlg, g_hMain );
            SendMessage ( GetDlgItem ( hDlg, IDC_TABSTOPS ), 
                EM_LIMITTEXT, 1, 0 );
            SetDlgItemInt ( hDlg, IDC_TABSTOPS, (UINT)lParam, FALSE );
            break;

        case WM_CLOSE:
            EndDialog ( hDlg, 0 );
            break;

        case WM_COMMAND:
            switch ( LOWORD ( wParam ) )
            {
                case IDCANCEL:
                    SendMessage ( hDlg, WM_CLOSE, 0, 0 );
                    break;

                case IDOK:
                    result = GetDlgItemInt ( hDlg, IDC_TABSTOPS, 
                        &converted, FALSE );

                    if ( ( result > 0 ) && ( result <= 8 ) && 
                        ( converted == TRUE ) )
                            EndDialog ( hDlg, result );
                    break;
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: DLG_FindDlgProc 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR CALLBACK 
//    Param.    1: HWND hDlg     : 
//    Param.    2: UINT uMsg     : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Wnd proc for the Search/Replace dlg box
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CALLBACK DLG_FindDlgProc ( HWND hDlg, UINT uMsg, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    DWORD   states[2] = { BST_UNCHECKED, BST_CHECKED };

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDlg ( hDlg, g_hMain );
            CheckDlgButton ( hDlg, IDC_CASE, states[g_bcase] );
            CheckDlgButton ( hDlg, IDC_WHOLE, states[g_bwhole] );
            CheckDlgButton ( hDlg, IDC_REPALL, states[g_ball] );
            CheckDlgButton ( hDlg, IDC_REPPROMPT, states[g_bprompt] );
            CheckDlgButton ( hDlg, IDC_FALL, states[g_bfall] );
            CheckDlgButton ( hDlg, IDC_SFWD, states[g_bfwd] );
            LoadCBBHistoryFromINI ( hDlg, IDC_FINDSTR, 
                TEXT("find history"), g_szinifile, FALSE );
            LoadCBBHistoryFromINI ( hDlg, IDC_REPSTR, 
                TEXT("replace history"), g_szinifile, FALSE );

            if ( (TCHAR *)lParam != NULL )
                if ( ((TCHAR *)lParam)[0] != TEXT('\0') )
                    SetDlgItemText ( hDlg, IDC_FINDSTR, (TCHAR *)lParam );

            break;

        case WM_CLOSE:
            EndDialog ( hDlg, 0 );
            break;

        case WM_COMMAND:
            if ( lParam )
            {
                if ( HIWORD ( wParam ) == BN_CLICKED )
                {
                    switch ( LOWORD ( wParam ) )
                    {
                        case IDC_CANCEL:
                            SendMessage ( hDlg, WM_CLOSE, 0, 0 );
                            break;

                        case IDC_FIND:
                            if ( SaveCBBHistoryToINI ( hDlg, IDC_FINDSTR,
                                    TEXT("find history"), g_szinifile ) )
                            {
                                GetDlgItemText ( hDlg, IDC_FINDSTR, 
                                    g_sztextbuf, ARRAYSIZE ( g_sztextbuf ) );

                                g_bcase = ( IsDlgButtonChecked 
                                    ( hDlg, IDC_CASE ) == BST_CHECKED );
                                g_bwhole = ( IsDlgButtonChecked 
                                    ( hDlg, IDC_WHOLE ) == BST_CHECKED );
                                g_ball = ( IsDlgButtonChecked 
                                    ( hDlg, IDC_REPALL ) == BST_CHECKED );
                                g_bprompt = ( IsDlgButtonChecked 
                                    ( hDlg, IDC_REPPROMPT ) == BST_CHECKED );
                                g_bfwd = ( IsDlgButtonChecked 
                                    ( hDlg, IDC_SFWD ) == BST_CHECKED );
                                g_bfall = ( IsDlgButtonChecked 
                                    ( hDlg, IDC_FALL ) == BST_CHECKED );

                                if ( SaveCBBHistoryToINI ( hDlg, IDC_REPSTR,
                                        TEXT("replace history"), g_szinifile ))
                                {
                                    GetDlgItemText ( hDlg, IDC_REPSTR,
                                        g_szrepbuf, ARRAYSIZE ( g_szrepbuf ));

                                    EndDialog ( hDlg, IDREPLACE );
                                    break;
                                }

                                EndDialog ( hDlg, (g_bfall ? IDFINDALL : IDFIND) );
                            }
                            break;
                    }
                }
            }
            if ( LOWORD ( wParam ) == IDCANCEL )
                SendMessage ( hDlg, WM_CLOSE, 0, 0 );
            break;

        default:
            return 0;
            break;
    }
    return 1;
}


/*-@@+@@--------------------------------------------------------------------*/
//       Function: DLG_GotoDlgProc 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR CALLBACK 
//    Param.    1: HWND hDlg     : 
//    Param.    2: UINT uMsg     : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Wnd proc for goto line dlg box
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CALLBACK DLG_GotoDlgProc ( HWND hDlg, UINT uMsg, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    DWORD   line;
    BOOL    success;
    HWND    hRich;

    hRich = tab_data[g_curtab].hchild;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDlg ( hDlg, g_hMain );
            SetDlgItemInt ( hDlg, IDC_LINES, 
                Rich_GetLineCount( hRich ), FALSE );
            SetDlgItemInt ( hDlg, IDC_CRTLINE, 
                Rich_GetCrtLine( hRich )+1, FALSE );
            break;

        case WM_CLOSE:
            EndDialog ( hDlg, 0 );
            break;

        case WM_COMMAND:
            if ( lParam )
            {
                if ( HIWORD(wParam) == BN_CLICKED )
                {
                    switch ( LOWORD(wParam) )
                    {
                        case IDC_CLOSE:
                            SendMessage ( hDlg, WM_CLOSE, 0, 0 );
                            break;

                        case IDC_GOTO:
                            line = GetDlgItemInt ( hDlg, IDC_DEST, 
                                &success, FALSE );
                            if ( success )
                            {
                                if ( line < Rich_GetLineCount( hRich )+1 )
                                {
                                    if ( line > 0 )
                                        line--;

                                    SendMessage ( hDlg, WM_CLOSE, 0, 0 );
                                    Rich_GotoLine ( hRich, line );
                                }
                            }
                            break;
                    }
                }
            }

            if ( LOWORD(wParam) == IDCANCEL )
                SendMessage ( hDlg, WM_CLOSE, 0, 0 );
            break;

        default:
            return 0;
            break;
    }
    return 1;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: DLG_EXTToolDlgProc 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR CALLBACK 
//    Param.    1: HWND hDlg     : 
//    Param.    2: UINT uMsg     : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Wnd proc for run external tool dlg box
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CALLBACK DLG_EXTToolDlgProc ( HWND hDlg, UINT uMsg,
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    HWND    hEd;
    TCHAR   temp[TMAX_PATH];
    TCHAR   item_txt[TMAX_PATH];
    TCHAR   * filespec;
    BOOL    capture;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDlg ( hDlg, g_hMain );
            capture = ( BOOL ) GetPrivateProfileInt ( TEXT("editor"), 
                TEXT("capture"), 0, g_szinifile );
            CheckDlgButton ( hDlg, IDC_CKREDIR, capture );
            GetModuleFileName ( NULL, temp, ARRAYSIZE ( temp ) );

            GetPrivateProfileString ( TEXT("editor"), 
                TEXT("process working dir"), 
                    FILE_Extract_path ( temp, FALSE ), 
                        item_txt, ARRAYSIZE(item_txt), g_szinifile );

            SetDlgItemText ( hDlg, IDC_WKDIR, item_txt );
            LoadCBBHistoryFromINI ( hDlg, IDC_CB, 
                TEXT("run history"), g_szinifile, TRUE );
            SetDlgItemText ( hDlg, 1024, macro_hlp );
            break;

        case WM_CLOSE:
            EndDialog ( hDlg, IDCANCEL );
            break;

        case WM_COMMAND:
            switch ( LOWORD ( wParam ) )
            {
                case IDCANCEL:
                    EndDialog ( hDlg, IDCANCEL );
                    break;

                case IDOK:
                    if ( SaveCBBHistoryToINI ( hDlg, IDC_CB, 
                            TEXT("run history"), g_szinifile ) )
                    {
                        RtlZeroMemory ( &etd, sizeof (EXEC_THREAD_DATA) );

                        etd.proc    = (PBPROC)ExecToolProc;

                        capture     = ( IsDlgButtonChecked 
                            ( hDlg, IDC_CKREDIR ) == BST_CHECKED );

                        etd.capture = capture;
                        etd.exectype= EXEC_EXTTOOL;
                        etd.lParam  = (LPARAM)g_hMain;
                        etd.wParam  = (WPARAM)g_hOutwList;

                        StringCchPrintf ( temp, ARRAYSIZE(temp), 
                            TEXT("%d"), (int)capture );

                        WritePrivateProfileString ( TEXT("editor"), 
                            TEXT("capture"), temp, g_szinifile );

                        hEd         = GetDlgItem ( hDlg, IDC_WKDIR );

                        if ( GetWindowTextLength ( hEd ) != 0 )
                        {
                            GetDlgItemText ( hDlg, IDC_WKDIR, 
                                temp, ARRAYSIZE ( temp ) );

                            // let's validate the path so we don't write junk
                            if ( PathFileExists ( temp ) )
                                StringCchCopy ( etd.curdir, 
                                    ARRAYSIZE ( etd.curdir ), temp );
                            else
                            {
                                filespec = (TCHAR *)FILE_Extract_path 
                                    ( tab_data[g_curtab].fpath, FALSE );
                                // expand macro ({p} = path to 
                                // current file in editor)
                                if ( (_tcsstr ( temp, TEXT("{p}")) != NULL)
                                    && (filespec != NULL))
                                        StringCchCopy ( etd.curdir, 
                                            ARRAYSIZE(etd.curdir), filespec );
                                // fallback to crt dir
                                else
                                {
                                    GetModuleFileName ( NULL, temp, 
                                        ARRAYSIZE ( temp ) );
                                    StringCchCopy ( etd.curdir, 
                                        ARRAYSIZE ( etd.curdir ), 
                                            FILE_Extract_path ( temp, FALSE ) );
                                }
                            }
                        }

                        WritePrivateProfileString ( TEXT("editor"), 
                            TEXT("process working dir"), etd.curdir, g_szinifile );
                        GetDlgItemText ( hDlg, IDC_CB, etd.cmd, 
                            ARRAYSIZE ( etd.cmd ) );
                        WritePrivateProfileString ( TEXT("editor"), 
                            TEXT("last_cmd"), etd.cmd, g_szinifile );
                        StringCchCopy ( etd.filename, 
                            ARRAYSIZE(etd.filename), tab_data[g_curtab].fpath );

                        EndDialog ( hDlg, IDOK );
                    }

                    break;
            }

            break;

        default:
            return 0;
            break;
    }

    return 1;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: DLG_OUTviewDlgProc 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR CALLBACK 
//    Param.    1: HWND hDlg     : 
//    Param.    2: UINT uMsg     : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Wnd proc for the outwindow dlg
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CALLBACK DLG_OUTviewDlgProc ( HWND hDlg, UINT uMsg, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    RECT        r;
    TCHAR       temp[256];
    int         ic;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            g_hOutwList     = GetDlgItem ( hDlg, IDC_LIST );
            g_outw_old_proc = ( WNDPROC ) SetWindowLongPtr ( g_hOutwList, 
                GWLP_WNDPROC, ( LONG_PTR ) SC_OutwSubclassProc ); //subclass

            if ( LoadDlgPosition ( &r, g_szinifile, TEXT("outwindow") ) )
                MoveWindow ( hDlg, r.left, r.top, r.right-r.left, 
                    r.bottom-r.top, FALSE );

            ListBox_SetSel ( g_hOutwList, TRUE, 0 );

            break;

        case WM_SIZE:
            GetClientRect ( hDlg, &r );
            MoveWindow ( GetDlgItem ( hDlg, IDC_LIST ), r.left, r.top, r.right, 
                r.bottom, TRUE );
            break;

        case WM_CLOSE:
            ShowWindow ( hDlg, SW_HIDE );
            break;

        case WM_DESTROY:
            SaveDlgPosition ( hDlg, g_szinifile, TEXT("outwindow") );
            break;

        case WM_SHOWWINDOW:

            if ( lParam == 0 && wParam == TRUE )
            {
                ic = ListBox_GetCount ( g_hOutwList );
                StringCchPrintf ( temp, ARRAYSIZE(temp), 
                    TEXT("Output Window - %d items"), ic );
                SetWindowText ( hDlg, temp );
                ListBox_SetTopIndex ( g_hOutwList, ic-1 );
            }

        default:
            return 0;
            break;
    }
    return 1;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: OutwUpdateTitle 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hWnd     : outw window
//    Param.    2: int listitems : how many items we have in the out lbox
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Sets the capture output window title to something :-)
/*--------------------------------------------------------------------@@-@@-*/
void OutwUpdateTitle ( HWND hWnd, int listitems )
/*--------------------------------------------------------------------------*/
{
    TCHAR temp[128];

    StringCchPrintf ( temp, ARRAYSIZE(temp), 
        TEXT("Output Window - %d items"), listitems );

    SetWindowText ( g_hOutWnd, temp );
}

/*void Listbox_SelectOne ( HWND hList, int item )
{
    ListBox_SetSel ( hList, FALSE, -1 );
    ListBox_SetSel ( hList, TRUE, item );
}*/

/*-@@+@@--------------------------------------------------------------------*/
//       Function: WndProc 
/*--------------------------------------------------------------------------*/
//           Type: LRESULT CALLBACK 
//    Param.    1: HWND hWnd     : 
//    Param.    2: UINT message  : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Wnd proc for the main window
/*--------------------------------------------------------------------@@-@@-*/
LRESULT CALLBACK WndProc ( HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    switch ( message )
    {
        #ifdef HAVE_HTMLHELP
        case WM_HELP:
            MainWND_OnHelp ( hWnd, wParam, lParam );
            break;
        #endif
        case WM_MOVE:
            SizeOutWnd ( hWnd );
            break;

        case WM_ENDFALL:
            MainWND_OnENDFALL ( hWnd, wParam, lParam );
            break;

        case WM_UPDFALL:
            MainWND_OnUPDFALL ( hWnd, wParam, lParam );
            break;
        
        case WM_BUFFALL:
            MainWND_OnBUFFALL ( hWnd, wParam, lParam );
            break;

        case WM_ENDEXECTOOL:
            MainWND_OnENDEXECTOOL ( hWnd, wParam, lParam );
            break;

        case WM_BUFEXECTOOL:
            MainWND_OnBUFEXECTOOL ( hWnd, wParam, lParam );
            break;

        case WM_ENDCVTABS:
            MainWND_OnENDCVTABS ( hWnd, wParam, lParam );
            break;

        case WM_UPDCVTABS:
            MainWND_OnUPDCVTABS ( hWnd, wParam, lParam );
            break;

        case WM_ENDCFDECL:
            MainWND_OnENDCFDECL ( hWnd, wParam, lParam );
            break;

        case WM_UPDCFDECL:
            MainWND_OnUPDCFDECL ( hWnd, wParam, lParam );
            break;

        case WM_BUFCFDECL:
            MainWND_OnBUFCFDECL ( hWnd, wParam, lParam );
            break;

        case WM_ENDCFDEFS:
            MainWND_OnENDCFDEFS ( hWnd, wParam, lParam );
            break;

        case WM_UPDCFDEFS:
            MainWND_OnUPDCFDEFS ( hWnd, wParam, lParam );
            break;

        case WM_BUFCFDEFS:
            MainWND_OnBUFCFDEFS ( hWnd, wParam, lParam );
            break;

        case WM_ENDCFDECLSEL:
            MainWND_OnENDCFDECLSEL ( hWnd, wParam, lParam );
            break;

        case WM_UPDCFDECLSEL:
            MainWND_OnUPDCFDECLSEL ( hWnd, wParam, lParam );
            break;

        case WM_BUFCFDECLSEL:
            MainWND_OnBUFCFDECLSEL ( hWnd, wParam, lParam );
            break;

        case WM_ENDCFDEFSSEL:
            MainWND_OnENDCFDEFSSEL ( hWnd, wParam, lParam );
            break;

        case WM_UPDCFDEFSSEL:
            MainWND_OnUPDCFDEFSSEL ( hWnd, wParam, lParam );
            break;

        case WM_BUFCFDEFSSEL:
            MainWND_OnBUFCFDEFSSEL ( hWnd, wParam, lParam );
            break;

        case WM_COPYDATA:
            return MainWND_OnCopyData ( hWnd, wParam, lParam );
            break;

        case WM_DROPFILES:
            MainWND_OnDropFiles ( hWnd, wParam, lParam );
            break;

        case WM_MENUSELECT: // menu description on statusbar
            SB_MenuHelp ( g_hInst, g_hStatus, (DWORD)wParam );
            SendMessage ( g_hStatus, WM_PAINT, 0, 0 );
            break;

        case WM_CREATE:
            MainWND_OnCreate ( hWnd, wParam, lParam );
            break;

        case WM_SIZE:
            MainWND_OnSize ( hWnd, wParam, lParam );
            break;

        case WM_SETFOCUS:
            SetFocus ( tab_data[g_curtab].hchild );
            break;

        case WM_INITMENUPOPUP:
            MainWND_OnInitMenuPopup ( hWnd, wParam, lParam );
            break;

        case WM_NOTIFY:
            MainWND_OnNotify ( hWnd, wParam, lParam );
            break;

        case WM_COMMAND:
            MainWND_OnCommand ( hWnd, wParam, lParam );
            break;

        case WM_CLOSE:
            if ( QuitSaveCheck() )
            {
                SaveWindowPosition ( hWnd, g_szinifile );
                DragAcceptFiles ( hWnd, FALSE ); // no more Satan
                DestroyWindow ( hWnd );
                DestroyWindow ( g_hOutWnd );
                PostThreadMessage ( g_tid, WM_RAGEQUIT, 0, 0 );
            }
            break;

        case WM_DESTROY:
            PostQuitMessage ( 0 ) ;
            break;

        default:
            return DefWindowProc ( hWnd, message, wParam, lParam );
            break;
    }

    return 0;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: UpdateMenuStatus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Enable menus based on various conditions
/*--------------------------------------------------------------------@@-@@-*/
void UpdateMenuStatus ( void )
/*--------------------------------------------------------------------------*/
{
    BOOL       state;
    DWORD      states[4] = { MF_GRAYED, MF_ENABLED, MF_UNCHECKED, MF_CHECKED };
    HWND       hRich;

    // get out if a thread task is in progress
    if ( g_thread_working )
        return;

    hRich = tab_data[g_curtab].hchild;

    state = ( ( tab_data[g_curtab].fpath[0] != 0 ) 
        && ( lstrcmp ( tab_data[g_curtab].fpath, TEXT("Untitled") ) != 0 ) );

    EnableMenuItem ( g_hToolsmenu, IDM_VEXPLORE, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_PROP, states[state] );

    state = ( state && ( !Rich_GetModified( hRich ) ) );
    EnableMenuItem ( g_hEditmenu, IDM_REFR, states[state] );

    state = Rich_CanPaste ( hRich );
    EnableMenuItem ( g_hEditmenu, IDM_PASTE, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_ECLIP, states[state] );

    state = Rich_CanUndo ( hRich );
    EnableMenuItem ( g_hEditmenu, IDM_UNDO, states[state] );

    state = Rich_CanRedo ( hRich );
    EnableMenuItem ( g_hEditmenu, IDM_REDO, states[state] );

    CheckMenuItem ( g_hEditmenu, IDM_MARGIN, states[2+g_bmargin] );
    CheckMenuItem ( g_hEditmenu, IDM_WRAP, states[2+g_bwrap] );
    CheckMenuItem ( g_hViewmenu, IDM_ONTOP, states[2+g_bontop] );

    // if no more updating, exit early, 
    // since this may be executing on idle state
    if ( !g_bmenu_needs_update )
        return;

    state = ( GetWindowTextLength ( hRich ) != 0 );
    EnableMenuItem ( g_hEditmenu, IDM_GOTO, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_FIND, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_SELALL, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_MERGE, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_FINDSEL, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_TABS, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_PRINT, states[state] );//IDM_CCOM
    EnableMenuItem ( g_hToolsmenu, IDM_CCOM, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_CDEFS_ALL, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_CDECL_ALL, states[state] );

    state = ( Rich_GetSelLen ( hRich ) > 0 );
    EnableMenuItem ( g_hEditmenu, IDM_COPY, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_CUT, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_DEL, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_SAVESEL, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_FINDSEL, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_VIEWHEX, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_UPCASE, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_LOWCASE, states[state] );

    EnableMenuItem ( g_hToolsmenu, IDM_CDEFS_SEL, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_CDECL_SEL, states[state] );

    g_bmenu_needs_update = FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: DLG_OpenDlg 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hParent    : parent to OpenDlg
//    Param.    2: TCHAR * filename: buffer to put selection to
//    Param.    3: int size        : buffer length, in chars 
//    Param.    4: BOOL whole      : replace the whole richedit content or
//                                   selection only
//    Param.    5: BOOL newtab     : open in a new tab
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Display the OpenFile dlg
/*--------------------------------------------------------------------@@-@@-*/
BOOL DLG_OpenDlg ( HWND hParent, TCHAR * filename, 
    int size, BOOL whole, BOOL newtab )
/*--------------------------------------------------------------------------*/
{
    OPENFILENAME    ofn;
    DWORD           dlgstyle;
    int             encoding;
    BOOL            result = TRUE;
    HWND            hRich;

    RtlZeroMemory ( &ofn, sizeof ( ofn ) );
    dlgstyle = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;

    // allow multiselect if we don't do merge
    if ( whole )
        dlgstyle |= OFN_ALLOWMULTISELECT;

    ofn.lStructSize     = sizeof ( ofn );
    ofn.hInstance       = g_hInst;
    ofn.hwndOwner       = hParent;
    ofn.Flags           = dlgstyle;
    ofn.lpstrFilter     = opn_filter;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = filename;
    ofn.nMaxFile        = size;
    ofn.lpstrTitle      = opn_title;
    ofn.lpstrDefExt     = opn_defext;

    if ( GetOpenFileName ( &ofn ) )
    {
        SB_StatusSetSimple ( g_hStatus, TRUE );
        SB_StatusSetText ( g_hStatus, 255|SBT_NOBORDERS, TEXT("opening...") );

        if ( whole )
        {
            OpenFileListFromOFD ( filename, ofn.nFileOffset, newtab );
        }
        else // Merge file, no multiselect
        {
            hRich = tab_data[g_curtab].hchild;
            encoding = ENC_FileCheckEncoding ( filename );

            if ( encoding == 0 )
            {
                SB_StatusSetSimple ( g_hStatus, FALSE );
                SB_StatusResizePanels ( g_hStatus );
                return FALSE;
            }

            result = Rich_SelStreamin ( hRich, NULL, 
                (EDITSTREAMCALLBACK)STR_StreamInProc, encoding, filename );

            if ( result )
                TD_UpdateTabDataEntry ( tab_data, TD_FENCOD, 
                    g_curtab, 0, encoding, 0, 0, NULL );
        }

        SB_StatusSetSimple ( g_hStatus, FALSE );
        SB_StatusResizePanels ( g_hStatus );
    }

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: DLG_SaveDlg 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hParent    : parent to SaveDlg
//    Param.    2: TCHAR * filename: buffer with the filename
//    Param.    3: int size        : buffer size in chars
//    Param.    4: BOOL whole      : save whole file or selection only
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Displays the SaveFile dlg
/*--------------------------------------------------------------------@@-@@-*/
BOOL DLG_SaveDlg ( HWND hParent, TCHAR * filename, int size, BOOL whole )
/*--------------------------------------------------------------------------*/
{
    OPENFILENAME    ofn;
    DWORD           dlgstyle;
    float           file_size;
    BOOL            result = TRUE;
    TCHAR           temp[128];
    HWND            hRich;

    RtlZeroMemory ( &ofn, sizeof ( ofn ) );

    dlgstyle = OFN_EXPLORER|OFN_PATHMUSTEXIST|
                OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

    ofn.lStructSize     = sizeof ( ofn );
    ofn.hInstance       = g_hInst;
    ofn.hwndOwner       = hParent;
    ofn.Flags           = dlgstyle;
    ofn.lpstrFilter     = opn_filter;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = filename;
    ofn.nMaxFile        = size;
    ofn.lpstrTitle      = sav_title;
    ofn.lpstrDefExt     = opn_defext;

    if ( GetSaveFileName ( &ofn ) )
    {
        hRich = tab_data[g_curtab].hchild;
        SB_StatusSetSimple ( g_hStatus, TRUE );
        SB_StatusSetText ( g_hStatus, 255|SBT_NOBORDERS, TEXT("saving...") );
        g_bom_written = FALSE;

        if ( whole )
        {
            result = Rich_Streamout ( hRich, NULL, 
                (EDITSTREAMCALLBACK)STR_StreamOutProc, 
                    tab_data[g_curtab].fencoding, filename );

            if ( result )
            {
                TD_UpdateTabDataEntry ( tab_data, TD_FNAME|TD_FPATH, 
                    g_curtab, 0, 0, 0, 0, filename );

                Tab_SetText ( g_hTab, g_curtab, tab_data[g_curtab].fname );
                Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
                UpdateTitlebar (g_hMain, tab_data[g_curtab].fpath, app_title);
                Rich_SetModified ( hRich, FALSE );
                SB_StatusSetText ( g_hStatus, 1, TEXT("") );
                SB_StatusSetText ( g_hStatus, 2, TEXT("") );
                file_size = (float)FILE_GetFileSize ( filename );
                StringCchPrintf ( temp, ARRAYSIZE(temp), CCH_SPEC1,
                    ENC_EncTypeToStr ( tab_data[g_curtab].fencoding ),
                    (( file_size > 4096 ) ? (file_size/1024) : file_size ),
                    (( file_size > 4096 ) ? TEXT("kbytes") : TEXT("bytes")));

                SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, temp );
            }
        }
        else
            result = Rich_SelStreamout ( hRich, NULL, 
                (EDITSTREAMCALLBACK)STR_StreamOutProc, 
                    tab_data[g_curtab].fencoding, filename );

	    SB_StatusSetSimple ( g_hStatus, FALSE );
        SB_StatusResizePanels ( g_hStatus );
    }

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: STR_StreamInProc 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR CALLBACK 
//    Param.    1: DWORD_PTR dwCookie: custom data sent by EM_STREAMIN msg.
//    Param.    2: LPBYTE pbBuff     : buffer with text that richy sends to us
//                                     at every streamin iteration 
//    Param.    3: LONG cb           : buffer capacity, in bytes
//    Param.    4: LONG * pcb        : pointer to var to hold the actually 
//                                     read bytes
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Callback for the EM_STREAMIN richedit message, responsible 
//                 for text decoding and populating the control with text
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CALLBACK STR_StreamInProc ( DWORD_PTR dwCookie, LPBYTE pbBuff, 
    LONG cb, LONG * pcb )
/*--------------------------------------------------------------------------*/
{
    STREAM_AUX_DATA         * psdata;
    HWND                    hpbar;
    HANDLE                  hfile;
    DWORD                   fsize, csize, isize, chars;
    size_t                  ilen;
    void                    * data;
    static BOOL             bom_skipped = FALSE;

    psdata  = ( STREAM_AUX_DATA * )dwCookie;
    hpbar   = psdata->hpbar;
    hfile   = psdata->hfile;
    fsize   = psdata->fsize;

    // reserve for worst case :-)
    data = alloc_and_zero_mem ( ALIGN_4K((cb * 4) + 32) );    

    if ( data == NULL )
        return TRUE;

    switch ( psdata->fencoding )
    {
        case E_DONT_BOTHER:
        case E_ANSI:
            // read only cb/2 bytes, since outbuf will be max. cb bytes
            isize = cb >> 1;
            ReadFile ( ( HANDLE )hfile, data, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 ) // have we actually read anything?
            {
                csize = *pcb;
                // tell MultiByteToWideChar to return the
                // actual # of widechars in our buffer
                chars = MultiByteToWideChar ( CP_ACP, 0, data, csize,
                    NULL, 0 );
                // got it, do the actual conversion
                MultiByteToWideChar ( CP_ACP, 0, data, csize,
                    (WCHAR *)pbBuff, chars );
                chars <<= 1;    // bytes = chars * 2
                isize = *pcb;
                // tell richedit streamin function
                // the actual # of bytes processed
                *pcb = chars;   
            }
            break;

        case E_UTF_8:
            // read only cb/2 bytes, since outbuf will be max. cb bytes
            isize = cb >> 1;
            ReadFile ( ( HANDLE )hfile, data, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 ) // have we actually read anything?
            {
                csize = *pcb;
                // tell MultiByteToWideChar to return
                // the actual # of widechars in our buffer
                chars = MultiByteToWideChar ( CP_UTF8, 0, data, csize, 
                    NULL, 0 );
                // got it, do the actual conversion
                MultiByteToWideChar ( CP_UTF8, 0, data, csize, 
                    (WCHAR *)pbBuff, chars );
                chars <<= 1;    // bytes = chars * 2
                isize = *pcb;
                // tell richedit streamin function
                // the actual # of bytes processed
                *pcb = chars;   
            }
            break;

        case E_UTF_8_BOM:
            isize = cb >> 1;
            ReadFile ( ( HANDLE )hfile, data, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 )
            {
                csize = *pcb;

                if ( bom_skipped )
                {
                    chars = MultiByteToWideChar ( CP_UTF8, 0, data, csize,
                        NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, data, csize, 
                        (WCHAR *)pbBuff, chars );
                }
                else
                {
                    // skip 3 bytes of the UTF8 BOM and adjust count
                    csize -= 3; 
                    chars = MultiByteToWideChar ( CP_UTF8, 0, 
                        ((char *)(data)+3), csize, NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, 
                        ((char *)(data)+3), csize, (WCHAR *)pbBuff, chars );
                    bom_skipped = TRUE;
                }

                chars <<= 1;
                isize = *pcb;
                *pcb = chars;
            }
            else
                bom_skipped = FALSE;
            break;

        case E_UTF_16_LE:
            isize = cb;
            // just read in, since richedit is in unimode
            ReadFile ( ( HANDLE )hfile, pbBuff, isize, (LPDWORD)pcb, NULL ); 
            break;

        case E_UTF_16_BE:
            isize = cb;
            ReadFile ( ( HANDLE )hfile, data, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 )
                // byte swap isize *WCHARS*
                ENC_ChangeEndian ( (WCHAR *)pbBuff, 
                    (const WCHAR *)data, isize >> 1 );
            break;

        case E_UTF_16_LE_BOM:
            isize = cb;
            ReadFile ( ( HANDLE )hfile, data, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 )
            {
                csize = *pcb;

                if ( bom_skipped )
                {
                    // copy byte by byte, since it's LE
                    RtlCopyMemory ( pbBuff, data, csize ); 
                }
                else
                {
                    csize -= 2; // skip BOM and adjust byte count
                    RtlCopyMemory ( pbBuff, ((char *)(data)+2), csize );
                    bom_skipped = TRUE;
                }

                *pcb = csize;
            }
            else
                bom_skipped = FALSE;
            break;

        case E_UTF_16_BE_BOM:
            isize = cb;
            ReadFile ( ( HANDLE )hfile, data, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 )
            {
                csize = *pcb;

                if ( bom_skipped )
                {
                    ENC_ChangeEndian ( (WCHAR *)pbBuff, 
                        (const WCHAR *)data, csize >> 1 );
                }
                else
                {
                    csize -= 1; // skip BOM (1 WCHAR) and adjust count
                    ENC_ChangeEndian ( (WCHAR *)pbBuff, 
                        ((const WCHAR *)(data) + 1 ), csize >> 1 );
                    bom_skipped = TRUE;
                }

                *pcb = csize;
            }
            else
                bom_skipped = FALSE;
            break;

        case E_UTF_32_LE:
            isize = cb >> 2;
            isize <<= 2;
            ReadFile ( ( HANDLE )hfile, pbBuff, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 )
            {
                csize = *pcb;
                ilen = ENC_Utf32Utf8_LE ( (const char *)pbBuff, 
                    (int)csize, data, (int)csize );

                chars = MultiByteToWideChar ( CP_UTF8, 0, data, ilen,
                    NULL, 0 );
                MultiByteToWideChar ( CP_UTF8, 0, data, ilen, 
                    (WCHAR *)pbBuff, chars );

                chars <<= 1;
                isize = *pcb;
                *pcb = chars;
            }
            break;

        case E_UTF_32_BE:
            isize = cb >> 2;
            isize <<= 2;
            ReadFile ( ( HANDLE )hfile, pbBuff, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 )
            {
                csize = *pcb;
                ilen = ENC_Utf32Utf8_BE ( (const char *)pbBuff, 
                    (int)csize, data, (int)csize );

                chars = MultiByteToWideChar ( CP_UTF8, 0, data, ilen, 
                    NULL, 0 );
                MultiByteToWideChar ( CP_UTF8, 0, data, ilen, 
                    (WCHAR *)pbBuff, chars );

                chars <<= 1;
                isize = *pcb;
                *pcb = chars;
            }
            break;

        case E_UTF_32_BE_BOM:
            // read at DWORD boundaries
            // ( being utf32, you need to get a whole # of DWORDS,
            // otherwise you break the magic :-) )
            isize = cb >> 2;
            isize <<= 2;
            ReadFile ( ( HANDLE )hfile, pbBuff, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 )
            {
                csize = *pcb;
                ilen = ENC_Utf32Utf8_BE ( (const char *)pbBuff, 
                    (int)csize, data, (int)csize );

                if ( bom_skipped )
                {
                    chars = MultiByteToWideChar ( CP_UTF8, 0, data, ilen, 
                        NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, data, ilen, 
                        (WCHAR *)pbBuff, chars );
                }
                else
                {
                    ilen -= 3;
                    chars = MultiByteToWideChar ( CP_UTF8, 0, 
                        ((char *)(data)+3), ilen, NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, 
                        ((char *)(data)+3), ilen, (WCHAR *)pbBuff, chars );
                    bom_skipped = TRUE;
                }

                chars <<= 1;
                isize = *pcb;
                *pcb = chars;
            }
            else
                bom_skipped = FALSE;
            break;

        case E_UTF_32_LE_BOM:
            // read at DWORD boundaries
            // ( being utf32, you need to get a whole # of DWORDS,
            // otherwise you break the magic :-) )
            isize = cb >> 2;
            isize <<= 2;
            ReadFile ( ( HANDLE )hfile, pbBuff, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 )
            {
                csize = *pcb;
                ilen = ENC_Utf32Utf8_LE ( (const char *)pbBuff, 
                    (int)csize, data, (int)csize );

                if ( bom_skipped )
                {
                    chars = MultiByteToWideChar ( CP_UTF8, 0, data, 
                        ilen, NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, data, 
                        ilen, (WCHAR *)pbBuff, chars );
                }
                else
                {
                    ilen -= 3;
                    chars = MultiByteToWideChar ( CP_UTF8, 0, 
                        ((char *)(data)+3), ilen, NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, 
                        ((char *)(data)+3), ilen, (WCHAR *)pbBuff, chars );
                    bom_skipped = TRUE;
                }

                chars <<= 1;
                isize = *pcb;
                *pcb = chars;
            }
            else
                bom_skipped = FALSE;
            break;

        default: // if we're here, only lord knows what we got =)
            free_mem ( data );
            return TRUE;
            break;

    }

    if ( hpbar )
    {
        SendMessage ( hpbar, PBM_SETRANGE32, 0, (LPARAM) fsize );
        SendMessage ( hpbar, PBM_SETSTEP, ( WPARAM ) isize, 0 );
        SendMessage ( hpbar, PBM_STEPIT, 0, 0 );
    }

    free_mem ( data );

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: STR_StreamOutProc 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR CALLBACK 
//    Param.    1: DWORD_PTR dwCookie: custom data sent by EM_STREAMOUT msg.
//    Param.    2: LPBYTE pbBuff     : buffer with data to save snt to us 
//                                     by richy
//    Param.    3: LONG cb           : buffer capacity, in bytes
//    Param.    4: LONG * pcb        : bytes actually written
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Callback for the EM_STREAMOUT richedit message, responsible 
//                 for text encoding and saving to a file
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CALLBACK STR_StreamOutProc ( DWORD_PTR dwCookie, LPBYTE pbBuff, 
    LONG cb, LONG * pcb )
/*--------------------------------------------------------------------------*/
{
    STREAM_AUX_DATA         * psdata;
    HWND                    hpbar;
    HANDLE                  hfile;
    DWORD                   fsize, isize, b, bytes, i, dw;
    void                    * data;

    psdata  = ( STREAM_AUX_DATA * )dwCookie;
    hpbar   = psdata->hpbar;
    hfile   = psdata->hfile;
    fsize   = psdata->fsize;

    // reserve for worst case :-)
    data = alloc_and_zero_mem ( ALIGN_4K((cb * 4) + 32) ); 

    if ( data == NULL )
        return TRUE;

    switch ( psdata->fencoding )
    {
        case E_DONT_BOTHER:
        case E_ANSI:
            isize = cb >> 1;
            bytes = WideCharToMultiByte ( GetACP(), 0, 
                (const WCHAR *)pbBuff, isize, NULL, 0, NULL, NULL );
            WideCharToMultiByte ( GetACP(), 0, 
                (const WCHAR *)pbBuff, isize, data, bytes, NULL, NULL );
            WriteFile ( hfile, data, bytes, (LPDWORD)pcb, NULL );
            *pcb = cb;
            break;

        case E_UTF_8:
            isize = cb >> 1;
            bytes = WideCharToMultiByte ( CP_UTF8, 0, 
                (const WCHAR *)pbBuff, isize, NULL, 0, NULL, NULL );
            WideCharToMultiByte ( CP_UTF8, 0, 
                (const WCHAR *)pbBuff, isize, data, bytes, NULL, NULL );
            WriteFile ( hfile, data, bytes, (LPDWORD)pcb, NULL );
            *pcb = cb;
            break;

        case E_UTF_8_BOM:
            isize = cb >> 1;
            bytes = WideCharToMultiByte ( CP_UTF8, 0, 
                (const WCHAR *)pbBuff, isize, NULL, 0, NULL, NULL );
            WideCharToMultiByte ( CP_UTF8, 0, 
                (const WCHAR *)pbBuff, isize, data, bytes, NULL, NULL );

            if ( !g_bom_written )
            {
                WriteFile ( hfile, UTF_8_BOM, 3, &b, NULL );//UTF_8_BOM
                g_bom_written = TRUE;
            }

            WriteFile ( hfile, data, bytes, (LPDWORD)pcb, NULL );
            *pcb = cb;
            break;

        case E_UTF_16_LE:
            isize = cb >> 1;
            // just write out, since richedit is in unimode
            WriteFile ( hfile, pbBuff, cb, (LPDWORD)pcb, NULL ); 
            break;

        case E_UTF_16_BE:
            isize = cb >> 1;
            ENC_ChangeEndian ( (WCHAR *)data, (const WCHAR *)pbBuff, isize );
            // byte swap isize *WCHARS*
            WriteFile ( hfile, data, cb, (LPDWORD)pcb, NULL );
            break;

        case E_UTF_16_LE_BOM:
            isize = cb >> 1;

            if ( !g_bom_written )
            {
                WriteFile ( hfile, UTF_16_LE_BOM, 2, &b, NULL ); 
                g_bom_written = TRUE;
            }
            // UTF_16_LE_BOM
            WriteFile ( hfile, pbBuff, cb, (LPDWORD)pcb, NULL );
            break;

        case E_UTF_16_BE_BOM:
            isize = cb >> 1;
            ENC_ChangeEndian ( (WCHAR *)data, (const WCHAR *)pbBuff, isize );

            if ( !g_bom_written )
            {
                WriteFile ( hfile, UTF_16_BE_BOM, 2, &b, NULL );//UTF_16_BE_BOM
                g_bom_written = TRUE;
            }
            WriteFile ( hfile, data, cb, (LPDWORD)pcb, NULL );
            break;

        case E_UTF_32_LE:
            isize = cb >> 2;
            isize <<= 2;

            for ( i = 0; i < isize; i++ )
                ((DWORD *)data)[i] = ((WCHAR *)pbBuff)[i];

            WriteFile ( hfile, data, cb << 1, (LPDWORD)pcb, NULL );
            *pcb = cb;
            isize >>= 1;
            break;

        case E_UTF_32_BE:
            isize = cb >> 2;
            isize <<= 2;

            for ( i = 0; i < isize; i++ )
            {
                dw = 0;
                dw = ((WCHAR *)pbBuff)[i];
                ((DWORD *)data)[i] = ENC_Uint32_to_BE ( &dw );
            }

            WriteFile ( hfile, data, cb << 1, (LPDWORD)pcb, NULL );
            *pcb = cb;
            isize >>= 1;
            break;

        case E_UTF_32_LE_BOM:
            // let's write at DWORD boundary
            isize = cb >> 2;
            isize <<= 2;

            if ( !g_bom_written )
            {
                WriteFile ( hfile, UTF_32_LE_BOM, 4, &b, NULL );//UTF_32_LE_BOM
                g_bom_written = TRUE;
            }

            for ( i = 0; i < isize; i++ )
                ((DWORD *)data)[i] = ((WCHAR *)pbBuff)[i];

            WriteFile ( hfile, data, cb << 1, (LPDWORD)pcb, NULL );
            *pcb = cb;
            isize >>= 1;
            break;

        case E_UTF_32_BE_BOM:
            isize = cb >> 2;
            isize <<= 2;

            if ( !g_bom_written )
            {
                WriteFile ( hfile, UTF_32_BE_BOM, 4, &b, NULL );//UTF_32_BE_BOM
                g_bom_written = TRUE;
            }

            for ( i = 0; i < isize; i++ )
            {
                dw = 0;
                dw = ((WCHAR *)pbBuff)[i];
                ((DWORD *)data)[i] = ENC_Uint32_to_BE ( &dw );
            }

            WriteFile ( hfile, data, cb << 1, (LPDWORD)pcb, NULL );
            *pcb = cb;
            isize >>= 1;
            break;

        default:
            free_mem ( data );
            return TRUE;
            break;

    }

    if ( hpbar )
    {
        SendMessage ( hpbar, PBM_SETRANGE32, 0, (LPARAM)fsize );
        SendMessage ( hpbar, PBM_SETSTEP, ( WPARAM ) isize, 0 );
        SendMessage ( hpbar, PBM_STEPIT, 0, 0 );
    }

    free_mem ( data );

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Prompt_to_save 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Ask user to save umsaved tabs
/*--------------------------------------------------------------------@@-@@-*/
BOOL Prompt_to_save ( void )
/*--------------------------------------------------------------------------*/
{
    int     result;
    TCHAR   temp[512];

    if ( Rich_GetModified( tab_data[g_curtab].hchild ) )
    {
        #ifdef UNICODE
            StringCchPrintf ( temp, ARRAYSIZE(temp), 
            L"Do you wish to save \"%ls\" ?", tab_data[g_curtab].fpath );
        #else
            StringCchPrintf ( temp, ARRAYSIZE(temp), 
            "Do you wish to save \"%s\" ?", tab_data[g_curtab].fpath );
        #endif

        result = ShowMessage ( g_hMain, temp, MB_YESNOCANCEL, IDI_APP );

        if ( result == IDYES )
        {
            Menu_Save ( g_hMain );
        }
        else
        {
            if ( result == IDCANCEL )
                return FALSE;
        }
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: UpdateTitlebar 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hWnd            : 
//    Param.    2: const TCHAR * fname  : 
//    Param.    3: const TCHAR * wtitle : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Update main wnd title bar with filename and about :-)
/*--------------------------------------------------------------------@@-@@-*/
void UpdateTitlebar ( HWND hWnd, const TCHAR * fname, const TCHAR * wtitle )
/*--------------------------------------------------------------------------*/
{
    TCHAR       temp[TMAX_PATH];

    #ifdef UNICODE
        StringCchPrintf ( temp, ARRAYSIZE(temp), 
        L"%ls - %ls", fname, wtitle );
    #else
        StringCchPrintf ( temp, ARRAYSIZE(temp), 
        "%s - %s", fname, wtitle );
    #endif
    SetWindowText ( hWnd, temp );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_WipeClipboard 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : clippy owner
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Clear the clipboard
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_WipeClipboard ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    if ( OpenClipboard ( hWnd ) )
    {
        EmptyClipboard();
        CloseClipboard();
        return TRUE;
    }

    return FALSE;
}


/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_NewInstance 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Spawn yet another copy
/*--------------------------------------------------------------------@@-@@-*/
void Menu_NewInstance ( void )
/*--------------------------------------------------------------------------*/
{
    TCHAR    temp[TMAX_PATH];

    if ( GetModuleFileName ( NULL, temp, ARRAYSIZE ( temp ) ) )
        SimpleExec ( temp, FILE_Extract_path ( temp, FALSE ) );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_MergeFile 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : owner
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Insert a file at cursor
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_MergeFile ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    TCHAR        temp[TMAX_PATH], temp2[TMAX_PATH];
    int          old_encoding;
    BOOL         result;

    temp[0]      = TEXT('\0'); // for GetOpenFileName
    old_encoding = 0;
    result       = TRUE;

    // save old encoding, if any
    if ( tab_data[g_curtab].fencoding <= 
        E_ANSI && tab_data[g_curtab].fencoding >= E_UTF_8 )
            old_encoding = tab_data[g_curtab].fencoding;

    if ( !DLG_OpenDlg ( hWnd, temp, ARRAYSIZE ( temp ), FALSE, FALSE ) )
    {
        StringCchPrintf ( temp2, ARRAYSIZE(temp2), CCH_SPEC2, temp );
        ShowMessage ( hWnd, temp2, MB_OK, IDI_APP );
        result = FALSE;
    }

    if ( old_encoding != tab_data[g_curtab].fencoding )
    {
        StringCchPrintf ( temp2, ARRAYSIZE(temp2),
            CCH_SPEC4,
            ENC_EncTypeToStr ( old_encoding ),
            ENC_EncTypeToStr ( tab_data[g_curtab].fencoding ) );

        ShowMessage ( hWnd, temp2, MB_OK, IDI_APP );
    }
    // restore encoding
    if ( old_encoding > 0 )
        tab_data[g_curtab].fencoding = old_encoding;

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_SaveSelection 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hparent : owner
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Save selection to a file
/*--------------------------------------------------------------------@@-@@-*/
void Menu_SaveSelection ( HWND hparent )
/*--------------------------------------------------------------------------*/
{
    TCHAR    temp[TMAX_PATH], temp2[TMAX_PATH];

    temp[0] = 0;
    if ( !DLG_SaveDlg ( hparent, temp, ARRAYSIZE ( temp ), FALSE ) )
    {
        #ifdef UNICODE
            StringCchPrintf ( temp2, ARRAYSIZE(temp2), 
            L"Error saving selection to %ls", temp );
        #else
            StringCchPrintf ( temp2, ARRAYSIZE(temp2), 
            "Error saving selection to %s", temp );
        #endif
        ShowMessage ( hparent, temp2, MB_OK, IDI_APP );
    }
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FindAllProc 
/*--------------------------------------------------------------------------*/
//           Type: BOOL CALLBACK 
//    Param.    1: TCHAR * linetext: text from the line where text was found
//    Param.    2: DWORD line      : hit line
//    Param.    3: LPARAM lParam   : custom data
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 01.10.2020
//    DESCRIPTION: Callback for the Rich_FindAll function;
//                 Return FALSE to break operation.
/*--------------------------------------------------------------------@@-@@-*/
BOOL CALLBACK FindAllProc 
    ( BOOL valid, TCHAR * linetext, DWORD line, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    MSG msg;

    if ( PeekMessage ( &msg, (HWND)-1, WM_APP+100, WM_APP+200, PM_REMOVE ) )
        return FALSE;

    if (valid)
    {
        SendMessage ( (HWND)lParam, WM_UPDFALL, 
            (WPARAM)line+1, (LPARAM)0 );
        SendMessage ( (HWND)lParam, WM_BUFFALL, 
            (WPARAM)linetext, (LPARAM)line+1 );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FindTabsProc 
/*--------------------------------------------------------------------------*/
//           Type: BOOL CALLBACK 
//    Param.    1: DWORD tabstops: how many spaces in a tab
//    Param.    2: DWORD donetabs: how many tabs processed to the moment
//    Param.    3: LPARAM lParam : custom data
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Callback for the Rich_ConvertTabsToSpaces function; 
//                 return FALSE to break operation
/*--------------------------------------------------------------------@@-@@-*/
BOOL CALLBACK FindTabsProc ( DWORD tabstops, DWORD donetabs, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    MSG msg;

    if ( PeekMessage ( &msg, (HWND)-1, WM_APP+100, WM_APP+200, PM_REMOVE ) )
        return FALSE;

    if ( (donetabs % 16) == 0 ) // let's not kill the message pump
        SendMessage ( (HWND)lParam, WM_UPDCVTABS, 
            (WPARAM)tabstops, (LPARAM)donetabs );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FindCDefsProc 
/*--------------------------------------------------------------------------*/
//           Type: BOOL CALLBACK 
//    Param.    1: BOOL valid    : is a valid line or just ckeck for abort
//    Param.    2: DWORD lines   : total lines to process
//    Param.    3: DWORD crtline : current line
//    Param.    4: WPARAM wParam : custom data (buffer with function name)
//    Param.    5: LPARAM lParam : custom data (handle to main window)
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Next few ones are callbacks for the CF_FindCFDefinitions
//                 / CF_FindCFDeclarations functions; 
//                 return FALSE to abort op.
/*--------------------------------------------------------------------@@-@@-*/
BOOL CALLBACK FindCDefsProc ( BOOL valid, DWORD lines, 
    DWORD crtline, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    MSG msg;

    if ( PeekMessage ( &msg, (HWND)-1, WM_APP+100, WM_APP+200, PM_REMOVE ) )
        return FALSE;

    if ( valid )
    {
        SendMessage ( (HWND)lParam, WM_UPDCFDEFS, 
            (WPARAM)lines, (LPARAM)crtline );
        SendMessage ( (HWND)lParam, WM_BUFCFDEFS, 
            (WPARAM)wParam, (LPARAM)crtline );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FindCDefsSelProc 
/*--------------------------------------------------------------------------*/
//           Type: BOOL CALLBACK 
//    Param.    1: BOOL valid    : 
//    Param.    2: DWORD lines   : 
//    Param.    3: DWORD crtline : 
//    Param.    4: WPARAM wParam : 
//    Param.    5: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: 
/*--------------------------------------------------------------------@@-@@-*/
BOOL CALLBACK FindCDefsSelProc ( BOOL valid, DWORD lines, 
    DWORD crtline, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    MSG msg;

    if ( PeekMessage ( &msg, (HWND)-1, WM_APP+100, WM_APP+200, PM_REMOVE ) )
        return FALSE;

    if ( valid )
    {
        SendMessage ( (HWND)lParam, WM_UPDCFDEFSSEL, 
            (WPARAM)lines, (LPARAM)crtline );
        SendMessage ( (HWND)lParam, WM_BUFCFDEFSSEL, 
            (WPARAM)wParam, (LPARAM)crtline );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FindCDeclsProc 
/*--------------------------------------------------------------------------*/
//           Type: BOOL CALLBACK 
//    Param.    1: BOOL valid    : 
//    Param.    2: DWORD lines   : 
//    Param.    3: DWORD crtline : 
//    Param.    4: WPARAM wParam : 
//    Param.    5: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: 
/*--------------------------------------------------------------------@@-@@-*/
BOOL CALLBACK FindCDeclsProc ( BOOL valid, DWORD lines, 
    DWORD crtline, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    MSG msg;

    if ( PeekMessage ( &msg, (HWND)-1, WM_APP+100, WM_APP+200, PM_REMOVE ) )
        return FALSE;

    if ( valid )
    {
        SendMessage ( (HWND)lParam, WM_UPDCFDECL, 
            (WPARAM)lines, (LPARAM)crtline );
        SendMessage ( (HWND)lParam, WM_BUFCFDECL, 
            (WPARAM)wParam, (LPARAM)crtline );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FindCDeclsSelProc 
/*--------------------------------------------------------------------------*/
//           Type: BOOL CALLBACK 
//    Param.    1: BOOL valid    : 
//    Param.    2: DWORD lines   : 
//    Param.    3: DWORD crtline : 
//    Param.    4: WPARAM wParam : 
//    Param.    5: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: 
/*--------------------------------------------------------------------@@-@@-*/
BOOL CALLBACK FindCDeclsSelProc ( BOOL valid, DWORD lines, 
    DWORD crtline, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    MSG msg;

    if ( PeekMessage ( &msg, (HWND)-1, WM_APP+100, WM_APP+200, PM_REMOVE ) )
        return FALSE;

    if ( valid )
    {
        SendMessage ( (HWND)lParam, WM_UPDCFDECLSEL, 
            (WPARAM)lines, (LPARAM)crtline );
        SendMessage ( (HWND)lParam, WM_BUFCFDECLSEL, 
            (WPARAM)wParam, (LPARAM)crtline );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ExecToolProc 
/*--------------------------------------------------------------------------*/
//           Type: BOOL CALLBACK 
//    Param.    1: const TCHAR * s: captured output line to add to list 
//    Param.    2: LPARAM lParam  : custom data (outw listbox hwnd)
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Callback for the ExecAndCapture function; return FALSE to
//                 abort op.; called when a line of captured output is ready.
/*--------------------------------------------------------------------@@-@@-*/
BOOL CALLBACK ExecToolProc ( const TCHAR * s, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    MSG msg;

    if ( PeekMessage ( &msg, (HWND)-1, WM_APP+100, WM_APP+200, PM_REMOVE ) )
        return FALSE;

    if ( s != NULL && s[0] != TEXT('\0') )
    {
        SendMessage ( (HWND)lParam, WM_BUFEXECTOOL, (WPARAM)s, (LPARAM)0 );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: DLG_OutputWnd_Modeless 
/*--------------------------------------------------------------------------*/
//           Type: HWND 
//    Param.    1: HWND hWnd     : parent for the output window
//    Param.    2: LPARAM lParam : custom data
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Create the output window as a modeless dlg box
/*--------------------------------------------------------------------@@-@@-*/
HWND DLG_OutputWnd_Modeless ( HWND hWnd, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    return CreateDialogParam ( g_hInst, MAKEINTRESOURCE ( IDD_OUT ), 
        hWnd, DLG_OUTviewDlgProc, lParam );
}


/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_HexView 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Hex View menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_HexView ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    void        * buf;
    CHARRANGE   chr;
    INT_PTR     size, alloc_size;
    HEX_DATA    hd;
    HWND        hRich;

    hRich = tab_data[g_curtab].hchild;

    Rich_GetSel ( hRich, &chr );

    if ( chr.cpMax == -1 ) //select all
        size    = GetWindowTextLength ( hRich );
    else
        size    = chr.cpMax - chr.cpMin;

    alloc_size  = size*sizeof(TCHAR);
    buf         = alloc_and_zero_mem ( ALIGN_4K(alloc_size) );

    if ( buf == NULL )
        return FALSE;

    Rich_GetSelText ( hRich, buf, size );
    hd.hg       = buf;
    hd.dwlength = (DWORD)size;
    DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_HEX ), 
        hWnd, DLG_HEXViewDlgProc, ( LPARAM )&hd );

    return free_mem ( buf );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: DLG_HEXViewDlgProc 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR CALLBACK 
//    Param.    1: HWND hDlg     : 
//    Param.    2: UINT uMsg     : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Dlg proc for the Hex View dlg box.
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CALLBACK DLG_HEXViewDlgProc ( HWND hDlg, UINT uMsg, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    RECT        r;
    HWND        hlist;
    TCHAR       * data;
    DWORD       data_size;
    TCHAR       temp[256];
    int         ic;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            hlist = GetDlgItem ( hDlg, IDC_HEXLIST );
            g_hexw_old_proc = ( WNDPROC ) 
                SetWindowLongPtr ( hlist, GWLP_WNDPROC, 
                    ( LONG_PTR ) SC_HexwSubclassProc ); 

            if ( LoadDlgPosition ( &r, g_szinifile, TEXT("hexviewer") ) )
                MoveWindow ( hDlg, r.left, r.top, r.right-r.left, 
                    r.bottom-r.top, FALSE );

            CenterDlg ( hDlg, g_hMain );
            BeginDraw ( hlist );

            __try
            {
                data = (TCHAR *)(((HEX_DATA *)lParam)->hg);
                data_size = ((HEX_DATA *)lParam)->dwlength;
                HexDump ( data, data_size, 
                    (HEXDUMPPROC)HexDumpProc, (LPARAM)hlist );
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                ShowMessage ( hDlg, 
                    TEXT("Invalid data received :("), MB_OK, IDI_APP );
            }

            EndDraw ( hlist );
            ic = ListBox_GetCount ( hlist );
            SetFocus ( hlist );
            ListBox_SetSel ( hlist, TRUE, 0 );
            StringCchPrintf ( temp, ARRAYSIZE(temp), 
                TEXT("Hexadecimal View - %d items"), ic );
            SetWindowText ( hDlg, temp );
            break;

        case WM_SIZE:
            GetClientRect ( hDlg, &r );
            MoveWindow ( GetDlgItem ( hDlg, IDC_HEXLIST ), 
                r.left, r.top, r.right, r.bottom, TRUE );
            break;

        case WM_CLOSE:
            SaveDlgPosition ( hDlg, g_szinifile, TEXT("hexviewer") );
            EndDialog ( hDlg, 0 );
            break;

        case WM_COMMAND:

            if ( (LOWORD ( wParam )) == IDCANCEL )
                SendMessage ( hDlg, WM_CLOSE, 0, 0 );

            break;

        default:
            return 0;
            break;
    }
    return 1;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_RefreshFromDisk 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Refresh from disk menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_RefreshFromDisk ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    DWORD   line;
    float   file_size;
    TCHAR   temp[TMAX_PATH];
    HWND    hRich;

    hRich = tab_data[g_curtab].hchild;

    line = Rich_GetCrtLine( hRich );
    SB_StatusSetSimple ( g_hStatus, TRUE );
    SB_StatusSetText ( g_hStatus, 255|SBT_NOBORDERS, TEXT("opening...") );

    if ( Rich_Streamin ( hRich, NULL, 
            (EDITSTREAMCALLBACK)STR_StreamInProc, tab_data[g_curtab].fencoding,
                tab_data[g_curtab].fpath ) )
                    Rich_GotoLine ( hRich, line );
    else
    {
        StringCchPrintf ( temp, ARRAYSIZE(temp), 
            CCH_SPEC2, tab_data[g_curtab].fpath );

        ShowMessage ( hWnd, temp, MB_OK, IDI_APP );
        SB_StatusSetSimple ( g_hStatus, FALSE );
        return FALSE;
    }

    SB_StatusSetSimple ( g_hStatus, FALSE );
    file_size = (float)FILE_GetFileSize ( tab_data[g_curtab].fpath );
    StringCchPrintf ( temp, ARRAYSIZE(temp), CCH_SPEC,
                ENC_EncTypeToStr ( tab_data[g_curtab].fencoding ),
                ((file_size > 4096) ? (file_size / 1024) : file_size),
                ((file_size > 4096) ? TEXT("kbytes") : TEXT("bytes")));

    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, temp );
    Rich_SetModified ( hRich, FALSE );
    Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
    SB_StatusSetText ( g_hStatus, 1, TEXT("") );
    SB_StatusSetText ( g_hStatus, 2, TEXT("") );
    SB_StatusResizePanels ( g_hStatus );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CvtEnableMenus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL state : enabled/disabled
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Enable/disable menus for the tab convert op.
/*--------------------------------------------------------------------@@-@@-*/
void CvtEnableMenus ( BOOL state )
/*--------------------------------------------------------------------------*/
{
    HMENU       hsub;
    INT_PTR     i;
    DWORD       states[2] = { MF_GRAYED, MF_ENABLED };

    hsub = GetSubMenu ( g_hMainmenu, IDMM_FILE ); // file
    EnableMenuItem ( hsub, IDM_NEW, states[state] );
    EnableMenuItem ( hsub, IDM_OPEN, states[state] );
    EnableMenuItem ( hsub, IDM_SAVE, states[state] );
    EnableMenuItem ( hsub, IDM_SAVEAS, states[state] );
    EnableMenuItem ( hsub, IDM_NEWINST, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_ANSI, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_EDIT ); // edit
    EnableMenuItem ( hsub, IDM_UNDO, states[state] );
    EnableMenuItem ( hsub, IDM_REDO, states[state] );
    EnableMenuItem ( hsub, IDM_CUT, states[state] );
    EnableMenuItem ( hsub, IDM_COPY, states[state] );
    EnableMenuItem ( hsub, IDM_PASTE, states[state] );
    EnableMenuItem ( hsub, IDM_DEL, states[state] );
    EnableMenuItem ( hsub, IDM_SELALL, states[state] );
    EnableMenuItem ( hsub, IDM_FIND, states[state] );
    EnableMenuItem ( hsub, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( hsub, IDM_GOTO, states[state] );
    EnableMenuItem ( hsub, IDM_WRAP, states[state] );
    EnableMenuItem ( hsub, IDM_MARGIN, states[state] );
    EnableMenuItem ( hsub, IDM_VIEWHEX, states[state] );
    EnableMenuItem ( hsub, IDM_SAVESEL, states[state] );
    EnableMenuItem ( hsub, IDM_MERGE, states[state] );
    EnableMenuItem ( hsub, IDM_REFR, states[state] );
    EnableMenuItem ( hsub, IDM_FONT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_TOOLS ); // tools
    EnableMenuItem ( hsub, IDM_VEXPLORE, states[state] );
    EnableMenuItem ( hsub, IDM_RUN, states[state] );
    EnableMenuItem ( hsub, IDM_RUNLAST, states[state] );//IDM_RUNLAST
    EnableMenuItem ( hsub, IDM_WINFIND, states[state] );
    EnableMenuItem ( hsub, IDM_PRINT, states[state] );
    EnableMenuItem ( hsub, IDM_PROP, states[state] );
    EnableMenuItem ( hsub, IDM_CLEAR, states[state] );
    EnableMenuItem ( hsub, IDM_ECLIP, states[state] );
    EnableMenuItem ( hsub, IDM_COLOR, states[state] );
    EnableMenuItem ( hsub, IDM_FGCOLOR, states[state] );
    EnableMenuItem ( hsub, IDM_UPCASE, states[state] );
    EnableMenuItem ( hsub, IDM_LOWCASE, states[state] );
    EnableMenuItem ( hsub, IDM_RUNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_CCOM, states[state] );
    //EnableMenuItem ( hsub, IDM_TABS, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_SEL, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_VIEW ); // view
    EnableMenuItem ( hsub, IDM_ONTOP, states[state] );
    //EnableMenuItem ( hsub, IDM_VIEWOUT, states[state] );//IDM_CCOM

    hsub = GetSubMenu ( g_hMainmenu, IDMM_USER ); // view

    for ( i = 0; i < g_umenu_cnt; i++ )
        EnableMenuItem ( hsub, (UINT)(IDM_USERSTART+i), states[state] );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CFDeclEnableMenus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL state : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Enable disable menus for the Find C ... operation
/*--------------------------------------------------------------------@@-@@-*/
void CFDeclEnableMenus ( BOOL state )
/*--------------------------------------------------------------------------*/
{
    HMENU       hsub;
    INT_PTR     i;
    DWORD       states[2] = { MF_GRAYED, MF_ENABLED };

    hsub = GetSubMenu ( g_hMainmenu, IDMM_FILE ); // file
    EnableMenuItem ( hsub, IDM_NEW, states[state] );
    EnableMenuItem ( hsub, IDM_OPEN, states[state] );
    EnableMenuItem ( hsub, IDM_SAVE, states[state] );
    EnableMenuItem ( hsub, IDM_SAVEAS, states[state] );
    EnableMenuItem ( hsub, IDM_NEWINST, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_ANSI, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_EDIT ); // edit
    EnableMenuItem ( hsub, IDM_UNDO, states[state] );
    EnableMenuItem ( hsub, IDM_REDO, states[state] );
    EnableMenuItem ( hsub, IDM_CUT, states[state] );
    EnableMenuItem ( hsub, IDM_COPY, states[state] );
    EnableMenuItem ( hsub, IDM_PASTE, states[state] );
    EnableMenuItem ( hsub, IDM_DEL, states[state] );
    EnableMenuItem ( hsub, IDM_SELALL, states[state] );
    EnableMenuItem ( hsub, IDM_FIND, states[state] );
    EnableMenuItem ( hsub, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( hsub, IDM_GOTO, states[state] );
    EnableMenuItem ( hsub, IDM_WRAP, states[state] );
    EnableMenuItem ( hsub, IDM_MARGIN, states[state] );
    EnableMenuItem ( hsub, IDM_VIEWHEX, states[state] );
    EnableMenuItem ( hsub, IDM_SAVESEL, states[state] );
    EnableMenuItem ( hsub, IDM_MERGE, states[state] );
    EnableMenuItem ( hsub, IDM_REFR, states[state] );
    EnableMenuItem ( hsub, IDM_FONT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_TOOLS ); // tools
    EnableMenuItem ( hsub, IDM_VEXPLORE, states[state] );
    EnableMenuItem ( hsub, IDM_RUN, states[state] );
    EnableMenuItem ( hsub, IDM_RUNLAST, states[state] );
    EnableMenuItem ( hsub, IDM_WINFIND, states[state] );
    EnableMenuItem ( hsub, IDM_PRINT, states[state] );
    EnableMenuItem ( hsub, IDM_PROP, states[state] );
    EnableMenuItem ( hsub, IDM_CLEAR, states[state] );
    EnableMenuItem ( hsub, IDM_ECLIP, states[state] );
    EnableMenuItem ( hsub, IDM_COLOR, states[state] );
    EnableMenuItem ( hsub, IDM_FGCOLOR, states[state] );
    EnableMenuItem ( hsub, IDM_UPCASE, states[state] );
    EnableMenuItem ( hsub, IDM_LOWCASE, states[state] );
    EnableMenuItem ( hsub, IDM_RUNEXT, states[state] );

    EnableMenuItem ( hsub, IDM_CCOM, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_ALL, states[state] );
    //EnableMenuItem ( hsub, IDM_CDECL_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_TABS, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_VIEW ); // view
    EnableMenuItem ( hsub, IDM_ONTOP, states[state] );
    //EnableMenuItem ( hsub, IDM_VIEWOUT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_USER ); // view

    for ( i = 0; i < g_umenu_cnt; i++ )
        EnableMenuItem ( hsub, (UINT)(IDM_USERSTART+i), states[state] );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CFDefsEnableMenus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL state : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Enable/disable menus for Find C Function Defs
/*--------------------------------------------------------------------@@-@@-*/
void CFDefsEnableMenus ( BOOL state )
/*--------------------------------------------------------------------------*/
{
    HMENU       hsub;
    INT_PTR     i;
    DWORD       states[2] = { MF_GRAYED, MF_ENABLED };

    hsub = GetSubMenu ( g_hMainmenu, IDMM_FILE ); // file
    EnableMenuItem ( hsub, IDM_NEW, states[state] );
    EnableMenuItem ( hsub, IDM_OPEN, states[state] );
    EnableMenuItem ( hsub, IDM_SAVE, states[state] );
    EnableMenuItem ( hsub, IDM_SAVEAS, states[state] );
    EnableMenuItem ( hsub, IDM_NEWINST, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_ANSI, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_EDIT ); // edit
    EnableMenuItem ( hsub, IDM_UNDO, states[state] );
    EnableMenuItem ( hsub, IDM_REDO, states[state] );
    EnableMenuItem ( hsub, IDM_CUT, states[state] );
    EnableMenuItem ( hsub, IDM_COPY, states[state] );
    EnableMenuItem ( hsub, IDM_PASTE, states[state] );
    EnableMenuItem ( hsub, IDM_DEL, states[state] );
    EnableMenuItem ( hsub, IDM_SELALL, states[state] );
    EnableMenuItem ( hsub, IDM_FIND, states[state] );
    EnableMenuItem ( hsub, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( hsub, IDM_GOTO, states[state] );
    EnableMenuItem ( hsub, IDM_WRAP, states[state] );
    EnableMenuItem ( hsub, IDM_MARGIN, states[state] );
    EnableMenuItem ( hsub, IDM_VIEWHEX, states[state] );
    EnableMenuItem ( hsub, IDM_SAVESEL, states[state] );
    EnableMenuItem ( hsub, IDM_MERGE, states[state] );
    EnableMenuItem ( hsub, IDM_REFR, states[state] );
    EnableMenuItem ( hsub, IDM_FONT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_TOOLS ); // tools
    EnableMenuItem ( hsub, IDM_VEXPLORE, states[state] );
    EnableMenuItem ( hsub, IDM_RUN, states[state] );
    EnableMenuItem ( hsub, IDM_RUNLAST, states[state] );//IDM_RUNLAST
    EnableMenuItem ( hsub, IDM_WINFIND, states[state] );
    EnableMenuItem ( hsub, IDM_PRINT, states[state] );
    EnableMenuItem ( hsub, IDM_PROP, states[state] );
    EnableMenuItem ( hsub, IDM_CLEAR, states[state] );
    EnableMenuItem ( hsub, IDM_ECLIP, states[state] );
    EnableMenuItem ( hsub, IDM_COLOR, states[state] );
    EnableMenuItem ( hsub, IDM_FGCOLOR, states[state] );
    EnableMenuItem ( hsub, IDM_UPCASE, states[state] );
    EnableMenuItem ( hsub, IDM_LOWCASE, states[state] );
    EnableMenuItem ( hsub, IDM_RUNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_CCOM, states[state] );
    //EnableMenuItem ( hsub, IDM_CDEFS_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_TABS, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_VIEW ); // view
    EnableMenuItem ( hsub, IDM_ONTOP, states[state] );
    //EnableMenuItem ( hsub, IDM_VIEWOUT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_USER ); // view

    for ( i = 0; i < g_umenu_cnt; i++ )
        EnableMenuItem ( hsub, (UINT)(IDM_USERSTART+i), states[state] );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CFDeclSelEnableMenus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL state : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
void CFDeclSelEnableMenus ( BOOL state )
/*--------------------------------------------------------------------------*/
{
    HMENU       hsub;
    INT_PTR     i;
    DWORD       states[2] = { MF_GRAYED, MF_ENABLED };

    hsub = GetSubMenu ( g_hMainmenu, IDMM_FILE ); // file
    EnableMenuItem ( hsub, IDM_NEW, states[state] );
    EnableMenuItem ( hsub, IDM_OPEN, states[state] );
    EnableMenuItem ( hsub, IDM_SAVE, states[state] );
    EnableMenuItem ( hsub, IDM_SAVEAS, states[state] );
    EnableMenuItem ( hsub, IDM_NEWINST, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_ANSI, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_EDIT ); // edit
    EnableMenuItem ( hsub, IDM_UNDO, states[state] );
    EnableMenuItem ( hsub, IDM_REDO, states[state] );
    EnableMenuItem ( hsub, IDM_CUT, states[state] );
    EnableMenuItem ( hsub, IDM_COPY, states[state] );
    EnableMenuItem ( hsub, IDM_PASTE, states[state] );
    EnableMenuItem ( hsub, IDM_DEL, states[state] );
    EnableMenuItem ( hsub, IDM_SELALL, states[state] );
    EnableMenuItem ( hsub, IDM_FIND, states[state] );
    EnableMenuItem ( hsub, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( hsub, IDM_GOTO, states[state] );
    EnableMenuItem ( hsub, IDM_WRAP, states[state] );
    EnableMenuItem ( hsub, IDM_MARGIN, states[state] );
    EnableMenuItem ( hsub, IDM_VIEWHEX, states[state] );
    EnableMenuItem ( hsub, IDM_SAVESEL, states[state] );
    EnableMenuItem ( hsub, IDM_MERGE, states[state] );
    EnableMenuItem ( hsub, IDM_REFR, states[state] );
    EnableMenuItem ( hsub, IDM_FONT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_TOOLS ); // tools
    EnableMenuItem ( hsub, IDM_VEXPLORE, states[state] );
    EnableMenuItem ( hsub, IDM_RUN, states[state] );
    EnableMenuItem ( hsub, IDM_RUNLAST, states[state] );//IDM_RUNLAST
    EnableMenuItem ( hsub, IDM_WINFIND, states[state] );
    EnableMenuItem ( hsub, IDM_PRINT, states[state] );
    EnableMenuItem ( hsub, IDM_PROP, states[state] );
    EnableMenuItem ( hsub, IDM_CLEAR, states[state] );
    EnableMenuItem ( hsub, IDM_ECLIP, states[state] );
    EnableMenuItem ( hsub, IDM_COLOR, states[state] );
    EnableMenuItem ( hsub, IDM_FGCOLOR, states[state] );
    EnableMenuItem ( hsub, IDM_UPCASE, states[state] );
    EnableMenuItem ( hsub, IDM_LOWCASE, states[state] );
    EnableMenuItem ( hsub, IDM_RUNEXT, states[state] );

    EnableMenuItem ( hsub, IDM_CCOM, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_SEL, states[state] );
    //EnableMenuItem ( hsub, IDM_CDECL_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_TABS, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_VIEW ); // view
    EnableMenuItem ( hsub, IDM_ONTOP, states[state] );
    //EnableMenuItem ( hsub, IDM_VIEWOUT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_USER ); // view

    for ( i = 0; i < g_umenu_cnt; i++ )
        EnableMenuItem ( hsub, (UINT)(IDM_USERSTART+i), states[state] );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CFDefsSelEnableMenus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL state : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: 
/*--------------------------------------------------------------------@@-@@-*/
void CFDefsSelEnableMenus ( BOOL state )
/*--------------------------------------------------------------------------*/
{
    HMENU       hsub;
    INT_PTR     i;
    DWORD       states[2] = { MF_GRAYED, MF_ENABLED };

    hsub = GetSubMenu ( g_hMainmenu, IDMM_FILE ); // file
    EnableMenuItem ( hsub, IDM_NEW, states[state] );
    EnableMenuItem ( hsub, IDM_OPEN, states[state] );
    EnableMenuItem ( hsub, IDM_SAVE, states[state] );
    EnableMenuItem ( hsub, IDM_SAVEAS, states[state] );
    EnableMenuItem ( hsub, IDM_NEWINST, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_ANSI, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_EDIT ); // edit
    EnableMenuItem ( hsub, IDM_UNDO, states[state] );
    EnableMenuItem ( hsub, IDM_REDO, states[state] );
    EnableMenuItem ( hsub, IDM_CUT, states[state] );
    EnableMenuItem ( hsub, IDM_COPY, states[state] );
    EnableMenuItem ( hsub, IDM_PASTE, states[state] );
    EnableMenuItem ( hsub, IDM_DEL, states[state] );
    EnableMenuItem ( hsub, IDM_SELALL, states[state] );
    EnableMenuItem ( hsub, IDM_FIND, states[state] );
    EnableMenuItem ( hsub, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( hsub, IDM_GOTO, states[state] );
    EnableMenuItem ( hsub, IDM_WRAP, states[state] );
    EnableMenuItem ( hsub, IDM_MARGIN, states[state] );
    EnableMenuItem ( hsub, IDM_VIEWHEX, states[state] );
    EnableMenuItem ( hsub, IDM_SAVESEL, states[state] );
    EnableMenuItem ( hsub, IDM_MERGE, states[state] );
    EnableMenuItem ( hsub, IDM_REFR, states[state] );
    EnableMenuItem ( hsub, IDM_FONT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_TOOLS ); // tools
    EnableMenuItem ( hsub, IDM_VEXPLORE, states[state] );
    EnableMenuItem ( hsub, IDM_RUN, states[state] );
    EnableMenuItem ( hsub, IDM_RUNLAST, states[state] );//IDM_RUNLAST
    EnableMenuItem ( hsub, IDM_WINFIND, states[state] );
    EnableMenuItem ( hsub, IDM_PRINT, states[state] );
    EnableMenuItem ( hsub, IDM_PROP, states[state] );
    EnableMenuItem ( hsub, IDM_CLEAR, states[state] );
    EnableMenuItem ( hsub, IDM_ECLIP, states[state] );
    EnableMenuItem ( hsub, IDM_COLOR, states[state] );
    EnableMenuItem ( hsub, IDM_FGCOLOR, states[state] );
    EnableMenuItem ( hsub, IDM_UPCASE, states[state] );
    EnableMenuItem ( hsub, IDM_LOWCASE, states[state] );
    EnableMenuItem ( hsub, IDM_RUNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_CCOM, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_ALL, states[state] );
    //EnableMenuItem ( hsub, IDM_CDEFS_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_TABS, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_VIEW ); // view
    EnableMenuItem ( hsub, IDM_ONTOP, states[state] );
    //EnableMenuItem ( hsub, IDM_VIEWOUT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_USER ); // view

    for ( i = 0; i < g_umenu_cnt; i++ )
        EnableMenuItem ( hsub, (UINT)(IDM_USERSTART+i), states[state] );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ETEnableMenus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL state : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Enable/disable menus when running external tool
/*--------------------------------------------------------------------@@-@@-*/
void ETEnableMenus ( BOOL state )
/*--------------------------------------------------------------------------*/
{
    HMENU       hsub;
    INT_PTR     i;
    DWORD       states[2] = { MF_GRAYED, MF_ENABLED };

    hsub = GetSubMenu ( g_hMainmenu, IDMM_FILE ); // file
    EnableMenuItem ( hsub, IDM_NEW, states[state] );
    EnableMenuItem ( hsub, IDM_OPEN, states[state] );
    EnableMenuItem ( hsub, IDM_SAVE, states[state] );
    EnableMenuItem ( hsub, IDM_SAVEAS, states[state] );
    EnableMenuItem ( hsub, IDM_NEWINST, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_ANSI, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_EDIT ); // edit
    EnableMenuItem ( hsub, IDM_UNDO, states[state] );
    EnableMenuItem ( hsub, IDM_REDO, states[state] );
    EnableMenuItem ( hsub, IDM_CUT, states[state] );
    EnableMenuItem ( hsub, IDM_COPY, states[state] );
    EnableMenuItem ( hsub, IDM_PASTE, states[state] );
    EnableMenuItem ( hsub, IDM_DEL, states[state] );
    EnableMenuItem ( hsub, IDM_SELALL, states[state] );
    EnableMenuItem ( hsub, IDM_FIND, states[state] );
    EnableMenuItem ( hsub, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( hsub, IDM_GOTO, states[state] );
    EnableMenuItem ( hsub, IDM_WRAP, states[state] );
    EnableMenuItem ( hsub, IDM_MARGIN, states[state] );
    EnableMenuItem ( hsub, IDM_VIEWHEX, states[state] );
    EnableMenuItem ( hsub, IDM_SAVESEL, states[state] );
    EnableMenuItem ( hsub, IDM_MERGE, states[state] );
    EnableMenuItem ( hsub, IDM_REFR, states[state] );
    EnableMenuItem ( hsub, IDM_FONT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_TOOLS ); // tools
    EnableMenuItem ( hsub, IDM_VEXPLORE, states[state] );
    EnableMenuItem ( hsub, IDM_RUN, states[state] );
    EnableMenuItem ( hsub, IDM_RUNLAST, states[state] );//IDM_RUNLAST
    EnableMenuItem ( hsub, IDM_WINFIND, states[state] );
    EnableMenuItem ( hsub, IDM_PRINT, states[state] );
    EnableMenuItem ( hsub, IDM_PROP, states[state] );
    EnableMenuItem ( hsub, IDM_CLEAR, states[state] );
    EnableMenuItem ( hsub, IDM_ECLIP, states[state] );
    EnableMenuItem ( hsub, IDM_COLOR, states[state] );
    EnableMenuItem ( hsub, IDM_FGCOLOR, states[state] );
    EnableMenuItem ( hsub, IDM_UPCASE, states[state] );
    EnableMenuItem ( hsub, IDM_LOWCASE, states[state] );
    //EnableMenuItem ( hsub, IDM_RUNEXT, states[state] );

    EnableMenuItem ( hsub, IDM_CCOM, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_TABS, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_VIEW ); // view
    EnableMenuItem ( hsub, IDM_ONTOP, states[state] );
    //EnableMenuItem ( hsub, IDM_VIEWOUT, states[state] );//IDM_CCOM

    hsub = GetSubMenu ( g_hMainmenu, IDMM_USER ); // view

    for ( i = 0; i < g_umenu_cnt; i++ )
        EnableMenuItem ( hsub, (UINT)(IDM_USERSTART+i), states[state] );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FAllEnableMenus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL state : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 01.10.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
void FAllEnableMenus ( BOOL state )
/*--------------------------------------------------------------------------*/
{
    HMENU       hsub;
    INT_PTR     i;
    DWORD       states[2] = { MF_GRAYED, MF_ENABLED };

    hsub = GetSubMenu ( g_hMainmenu, IDMM_FILE ); // file
    EnableMenuItem ( hsub, IDM_NEW, states[state] );
    EnableMenuItem ( hsub, IDM_OPEN, states[state] );
    EnableMenuItem ( hsub, IDM_SAVE, states[state] );
    EnableMenuItem ( hsub, IDM_SAVEAS, states[state] );
    EnableMenuItem ( hsub, IDM_NEWINST, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_ANSI, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_EDIT ); // edit
    EnableMenuItem ( hsub, IDM_UNDO, states[state] );
    EnableMenuItem ( hsub, IDM_REDO, states[state] );
    EnableMenuItem ( hsub, IDM_CUT, states[state] );
    EnableMenuItem ( hsub, IDM_COPY, states[state] );
    EnableMenuItem ( hsub, IDM_PASTE, states[state] );
    EnableMenuItem ( hsub, IDM_DEL, states[state] );
    EnableMenuItem ( hsub, IDM_SELALL, states[state] );
    //EnableMenuItem ( hsub, IDM_FIND, states[state] );
    EnableMenuItem ( hsub, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( hsub, IDM_GOTO, states[state] );
    EnableMenuItem ( hsub, IDM_WRAP, states[state] );
    EnableMenuItem ( hsub, IDM_MARGIN, states[state] );
    EnableMenuItem ( hsub, IDM_VIEWHEX, states[state] );
    EnableMenuItem ( hsub, IDM_SAVESEL, states[state] );
    EnableMenuItem ( hsub, IDM_MERGE, states[state] );
    EnableMenuItem ( hsub, IDM_REFR, states[state] );
    EnableMenuItem ( hsub, IDM_FONT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_TOOLS ); // tools
    EnableMenuItem ( hsub, IDM_VEXPLORE, states[state] );
    EnableMenuItem ( hsub, IDM_RUN, states[state] );
    EnableMenuItem ( hsub, IDM_RUNLAST, states[state] );//IDM_RUNLAST
    EnableMenuItem ( hsub, IDM_WINFIND, states[state] );
    EnableMenuItem ( hsub, IDM_PRINT, states[state] );
    EnableMenuItem ( hsub, IDM_PROP, states[state] );
    EnableMenuItem ( hsub, IDM_CLEAR, states[state] );
    EnableMenuItem ( hsub, IDM_ECLIP, states[state] );
    EnableMenuItem ( hsub, IDM_COLOR, states[state] );
    EnableMenuItem ( hsub, IDM_FGCOLOR, states[state] );
    EnableMenuItem ( hsub, IDM_UPCASE, states[state] );
    EnableMenuItem ( hsub, IDM_LOWCASE, states[state] );
    EnableMenuItem ( hsub, IDM_RUNEXT, states[state] );

    EnableMenuItem ( hsub, IDM_CCOM, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_TABS, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_VIEW ); // view
    EnableMenuItem ( hsub, IDM_ONTOP, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_USER ); // view

    for ( i = 0; i < g_umenu_cnt; i++ )
        EnableMenuItem ( hsub, (UINT)(IDM_USERSTART+i), states[state] );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ETLastEnableMenus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL state : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
void ETLastEnableMenus ( BOOL state )
/*--------------------------------------------------------------------------*/
{
    HMENU       hsub;
    INT_PTR     i;
    DWORD       states[2] = { MF_GRAYED, MF_ENABLED };

    hsub = GetSubMenu ( g_hMainmenu, IDMM_FILE ); // file
    EnableMenuItem ( hsub, IDM_NEW, states[state] );
    EnableMenuItem ( hsub, IDM_OPEN, states[state] );
    EnableMenuItem ( hsub, IDM_SAVE, states[state] );
    EnableMenuItem ( hsub, IDM_SAVEAS, states[state] );
    EnableMenuItem ( hsub, IDM_NEWINST, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_ANSI, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_EDIT ); // edit
    EnableMenuItem ( hsub, IDM_UNDO, states[state] );
    EnableMenuItem ( hsub, IDM_REDO, states[state] );
    EnableMenuItem ( hsub, IDM_CUT, states[state] );
    EnableMenuItem ( hsub, IDM_COPY, states[state] );
    EnableMenuItem ( hsub, IDM_PASTE, states[state] );
    EnableMenuItem ( hsub, IDM_DEL, states[state] );
    EnableMenuItem ( hsub, IDM_SELALL, states[state] );
    EnableMenuItem ( hsub, IDM_FIND, states[state] );
    EnableMenuItem ( hsub, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( hsub, IDM_GOTO, states[state] );
    EnableMenuItem ( hsub, IDM_WRAP, states[state] );
    EnableMenuItem ( hsub, IDM_MARGIN, states[state] );
    EnableMenuItem ( hsub, IDM_VIEWHEX, states[state] );
    EnableMenuItem ( hsub, IDM_SAVESEL, states[state] );
    EnableMenuItem ( hsub, IDM_MERGE, states[state] );
    EnableMenuItem ( hsub, IDM_REFR, states[state] );
    EnableMenuItem ( hsub, IDM_FONT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_TOOLS ); // tools
    EnableMenuItem ( hsub, IDM_VEXPLORE, states[state] );
    EnableMenuItem ( hsub, IDM_RUN, states[state] );
    EnableMenuItem ( hsub, IDM_WINFIND, states[state] );
    EnableMenuItem ( hsub, IDM_PRINT, states[state] );
    EnableMenuItem ( hsub, IDM_PROP, states[state] );
    EnableMenuItem ( hsub, IDM_CLEAR, states[state] );
    EnableMenuItem ( hsub, IDM_ECLIP, states[state] );
    EnableMenuItem ( hsub, IDM_COLOR, states[state] );
    EnableMenuItem ( hsub, IDM_FGCOLOR, states[state] );
    EnableMenuItem ( hsub, IDM_UPCASE, states[state] );
    EnableMenuItem ( hsub, IDM_LOWCASE, states[state] );
    EnableMenuItem ( hsub, IDM_RUNEXT, states[state] );
    
    EnableMenuItem ( hsub, IDM_CCOM, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_TABS, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_VIEW ); // view
    EnableMenuItem ( hsub, IDM_ONTOP, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_USER ); // view

    for ( i = 0; i < g_umenu_cnt; i++ )
        EnableMenuItem ( hsub, (UINT)(IDM_USERSTART+i), states[state] );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_Save 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Save menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_Save ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    TCHAR           temp[TMAX_PATH], temp2[TMAX_PATH];
    HWND            hRich;
    float           file_size;

    hRich = tab_data[g_curtab].hchild;
    StringCchCopy ( temp, ARRAYSIZE(temp), TEXT("Untitled") );

    if ( lstrcmp ( tab_data[g_curtab].fpath, TEXT("Untitled") ) == 0 )
    {
        if ( !DLG_SaveDlg ( hWnd, temp, ARRAYSIZE ( temp ), TRUE  ) )
        {
            StringCchPrintf ( temp2, ARRAYSIZE(temp2), CCH_SPEC3, temp );
            ShowMessage ( hWnd, temp2, MB_OK, IDI_APP );
            return FALSE;
        }
    }
    else
    {
        SB_StatusSetSimple ( g_hStatus, TRUE );
        SB_StatusSetText ( g_hStatus, 255|SBT_NOBORDERS, TEXT("saving...") );
        g_bom_written = FALSE;

        if ( !Rich_Streamout ( hRich, NULL, 
                (EDITSTREAMCALLBACK)STR_StreamOutProc, 
                    tab_data[g_curtab].fencoding, tab_data[g_curtab].fpath ) )
        {
            StringCchPrintf ( temp2, ARRAYSIZE(temp2), 
                CCH_SPEC3, tab_data[g_curtab].fpath );
            ShowMessage ( hWnd, temp2, MB_OK, IDI_APP );
            SB_StatusSetSimple ( g_hStatus, FALSE );
            SB_StatusSetText ( g_hStatus, 1, TEXT("") );
            SB_StatusSetText ( g_hStatus, 2, TEXT("") );
            SB_StatusResizePanels ( g_hStatus );
            return FALSE;
        }
        SB_StatusSetSimple ( g_hStatus, FALSE );
        Rich_SetModified ( hRich, FALSE );
        Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
        SB_StatusSetText ( g_hStatus, 1, TEXT("") );
        SB_StatusSetText ( g_hStatus, 2, TEXT("") );
        file_size = (float)FILE_GetFileSize ( tab_data[g_curtab].fpath );
        StringCchPrintf ( temp, ARRAYSIZE (temp), CCH_SPEC1,
            ENC_EncTypeToStr ( tab_data[g_curtab].fencoding ),
            ((file_size > 4096) ? (file_size / 1024) : file_size),
            ((file_size > 4096) ? TEXT("kbytes") : TEXT("bytes")) );

        SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, temp );
        SB_StatusResizePanels ( g_hStatus );
    }
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_Saveas 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Save as menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_Saveas ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    TCHAR   temp [TMAX_PATH];
    TCHAR   temp2[TMAX_PATH];

    StringCchCopy ( temp, ARRAYSIZE(temp), tab_data[g_curtab].fpath );

    if ( !DLG_SaveDlg ( hWnd, temp, ARRAYSIZE ( temp ), TRUE ) )
    {
        StringCchPrintf ( temp2, ARRAYSIZE(temp2), CCH_SPEC3, temp );
        ShowMessage ( hWnd, temp2, MB_OK, IDI_APP );
        return FALSE;
    }
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_New 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: New menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_New ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    int          tidx, tcount, lastsel;
    HWND         hRich;
    RECT         wr;

    tcount      = Tab_GetCount ( g_hTab );
    if ( tcount >= MAX_TABS )
    {
        ShowMessage ( hWnd, 
            TEXT("Maximum tab page limit reached"), MB_OK, IDI_APP );

        return FALSE;
    }

    BeginDraw ( g_hMain );
    tidx        = Tab_NewTabPage ( g_hTab, tcount, TEXT("Untitled") );

    if ( tidx == -1 )
    {
        ShowMessage ( hWnd, 
            TEXT("Unable to create tab page :("), MB_OK, IDI_APP );

        return FALSE;
    }

    hRich       = Rich_NewRichedit ( g_hInst, &g_rich_old_proc, 
        SC_RichSubclassProc, hWnd, IDC_RICH + tidx, &g_hFont, 
            g_bwrap, g_bmargin, g_szinifile );

    if ( hRich == NULL )
    {
        ShowMessage ( hWnd, 
            TEXT("Unable to create tab page :("), MB_OK, IDI_APP );

        return FALSE;
    }

    g_curtab    = tidx;
    TD_UpdateTabDataEntry ( tab_data, TD_ALL, g_curtab, 
        IDC_RICH + tidx, E_UTF_8, SW_SHOW, hRich, NULL );
    lastsel     = Tab_SetCurSel ( g_hTab, tidx );
    ShowWindow ( tab_data[lastsel].hchild, SW_HIDE );
    tab_data[lastsel].nCmdShow = SW_HIDE;
    GetClientRect ( g_hMain, &wr );
    SendMessage ( g_hMain, WM_SIZE, 0, MAKELPARAM ( wr.right, wr.bottom ) );
    ShowWindow ( g_hTab, SW_SHOWNORMAL );
    SetFocus ( hRich );
    RtlZeroMemory ( g_sztextbuf, sizeof ( g_sztextbuf ) );
    Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
    UpdateTitlebar ( g_hMain, tab_data[g_curtab].fpath, app_title );
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, TEXT("") );
    SB_StatusSetText ( g_hStatus, 1, TEXT("") );
    SB_StatusSetText ( g_hStatus, 2, TEXT("") );
    UpdateCaretPos ( tab_data[g_curtab].hchild, TRUE );
    EndDraw ( g_hMain );
    RedrawWindow ( g_hMain, NULL, NULL, 
        RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_ToggleMargin 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Toggle selection margin; such hassle for most 
//                 useless feature :-))
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_ToggleMargin ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    BOOL            mod;
    RECT            wr, tr, rstat;
    size_t          i, tcount;
    int             img_state[2] = {TIDX_SAV, TIDX_MOD};

    BeginDraw ( g_hMain );
    g_bmargin = !g_bmargin;
    tcount = Tab_GetCount ( g_hTab );
    SendMessage ( g_hStatus, WM_SIZE, 0, 0 );
    GetClientRect ( g_hStatus, &rstat );
    GetClientRect ( g_hMain, &wr );
    MoveWindow ( g_hTab, wr.left, wr.top, wr.right-wr.left, 
        wr.bottom - rstat.bottom, FALSE );
    GetClientRect ( g_hTab, &tr );
    SendMessage ( g_hTab, TCM_ADJUSTRECT, 0, (LPARAM)&tr );

    if ( tcount > 1 )
    {
        for ( i = 0; i < tcount; i++ )
        {
            mod = Rich_GetModified( tab_data[i].hchild );
            Rich_ToggleSelBar ( g_hInst, &g_rich_old_proc, 
                SC_RichSubclassProc, &(tab_data[i].hchild), 
                    hWnd, tab_data[i].child_id, &g_hFont, g_bwrap, 
                        g_bmargin, g_szinifile );
            ShowWindow ( tab_data[i].hchild, tab_data[i].nCmdShow );
            MoveWindow ( tab_data[i].hchild, tr.left, tr.top, 
                tr.right-tr.left, tr.bottom-tr.top, TRUE );
            Rich_SetModified ( tab_data[i].hchild, mod );
            Tab_SetImg ( g_hTab, i, img_state[mod] );
        }
    }
     else
    {
        mod = Rich_GetModified( tab_data[g_curtab].hchild );
        Rich_ToggleSelBar ( g_hInst, &g_rich_old_proc, 
            SC_RichSubclassProc, &(tab_data[g_curtab].hchild), 
                hWnd, tab_data[g_curtab].child_id, &g_hFont, g_bwrap, 
                    g_bmargin, g_szinifile );
        ShowWindow ( tab_data[g_curtab].hchild, tab_data[g_curtab].nCmdShow );
        MoveWindow ( tab_data[g_curtab].hchild, tr.left, tr.top, 
            tr.right-tr.left, tr.bottom-tr.top, TRUE );
        Rich_SetModified ( tab_data[g_curtab].hchild, mod );
        Tab_SetImg ( g_hTab, g_curtab, img_state[mod] );
    }

    if ( g_bmargin )
        WritePrivateProfileString ( TEXT("editor"), 
            TEXT("margin"), TEXT("1"), g_szinifile );
    else
        WritePrivateProfileString ( TEXT("editor"), 
            TEXT("margin"), TEXT("0"), g_szinifile );

    mod = Rich_GetModified( tab_data[g_curtab].hchild );
    ShowWindow ( g_hTab, SW_SHOWNORMAL );
    SetFocus ( tab_data[g_curtab].hchild );
    UpdateCaretPos ( tab_data[g_curtab].hchild, TRUE );

    if ( mod )
        UpdateModifiedStatus(TRUE);

    EndDraw ( g_hMain );
    RedrawWindow ( g_hMain, NULL, NULL, 
        RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_Find 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Find menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_Find ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    INT_PTR     result;
    HWND        hRich;
    TCHAR       seltext[256];

    if ( g_thread_working )
    {
        g_thread_working = FALSE;
        PostThreadMessage ( g_tid, WM_ABTFALL, 0, 0 );
        return FALSE;
    }

    hRich = tab_data[g_curtab].hchild;
    Rich_GetSelText ( hRich, seltext, ARRAYSIZE(seltext) );

    result = DialogBoxParam ( g_hInst, 
        MAKEINTRESOURCE ( IDD_FIND ), hWnd, 
            DLG_FindDlgProc, ( LPARAM ) seltext );

    switch ( result )
    {
        case IDFIND:
            Rich_FindReplaceText ( hRich, hWnd, g_sztextbuf, 
                NULL, g_bcase, g_bwhole, FALSE, g_bfwd );
            break;

        case IDFINDALL:
            Menu_FindAll ( hWnd );
            break;

        case IDREPLACE:
            if ( g_ball )
            {
                if ( !g_bprompt )
                {
                    BeginDraw ( hRich );
                    ShowCursor ( FALSE );
                }

                while ( Rich_FindReplaceText ( hRich, hWnd, 
                    g_sztextbuf, g_szrepbuf, g_bcase, 
                        g_bwhole, g_bprompt, g_bfwd ) );

                if ( !g_bprompt )
                {
                    EndDraw ( hRich );
                    ShowCursor ( TRUE );
                }
            }
            else
                Rich_FindReplaceText ( hRich, hWnd, g_sztextbuf, g_szrepbuf,
                    g_bcase, g_bwhole, g_bprompt, g_bfwd );
            break;
    }
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_ConvertTabs 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Convert tabs menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_ConvertTabs ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    INT_PTR      result;

    if ( g_thread_working )
    {
        g_thread_working = FALSE;
        PostThreadMessage ( g_tid, WM_ABTCVTABS, 0, 0 );
        return FALSE;
    }

    result = DialogBoxParam ( g_hInst, 
        MAKEINTRESOURCE ( IDD_TABS ), hWnd, 
            DLG_TabsDlgProc, ( LPARAM ) g_dwtabsize );

    if ( result > 0 )
    {
        g_dwtabsize         = result;
        g_thread_working    = TRUE;

        ttd.hRich           = tab_data[g_curtab].hchild;
        ttd.proc            = FindTabsProc;
        ttd.lParam          = (LPARAM)g_hMain;
        ttd.dwTabs          = (DWORD)g_dwtabsize;

        EnableWindow ( g_hTab, FALSE );
        BeginDraw ( tab_data[g_curtab].hchild );
        g_szoldmenu = GetMenuItemText ( g_hToolsmenu, IDM_TABS );
        SetMenuItemText ( g_hToolsmenu, IDM_TABS, 
            TEXT("C&onverting tabs to spaces... click to abort") );

        CvtEnableMenus ( FALSE );
        SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
            TEXT("Searching for tabs...") );

        if ( g_thandle )
        {
            CloseHandle ( (HANDLE)g_thandle );
            g_thandle = 0;
        }
        
        OutwInsertTimeMarker ( g_hOutwList, 
            TEXT(">>>> Process started at "), NULL );

        g_thandle = _beginthreadex ( NULL, 0, 
            Thread_ConvertTabsToSpaces, &ttd, 0, &g_tid );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_FindAll 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 01.10.2020
//    DESCRIPTION: Find all
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_FindAll ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    g_thread_working    = TRUE;

    atd.hRich           = tab_data[g_curtab].hchild;
    atd.proc            = FindAllProc;
    atd.lParam          = (LPARAM)g_hMain;
    atd.ftext           = g_sztextbuf;
    atd.bcase           = g_bcase;
    atd.bfwd            = g_bfwd;
    atd.bwhole          = g_bwhole;

    EnableWindow ( g_hTab, FALSE );
    BeginDraw ( tab_data[g_curtab].hchild );

    g_szoldmenu = GetMenuItemText ( g_hEditmenu, IDM_FIND );
    SetMenuItemText ( g_hEditmenu, IDM_FIND, 
        TEXT("Searching... click to abort") );

    FAllEnableMenus ( FALSE );

    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
        TEXT("Searching ...") );

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }
    
    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT(">>>> Process started at "), NULL );

    g_thandle = _beginthreadex ( NULL, 0, 
        Thread_FindAll, &atd, 0, &g_tid );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_About 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: :)
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_About ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_ABOUT ), 
        hWnd, DLG_AboutDlgProc, ( LPARAM ) hWnd );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_SetFont 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Set font menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_SetFont ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    BOOL       mod;
    HWND       hRich;
    size_t     tcount, i;
    int        img_state[2] = {TIDX_SAV, TIDX_MOD};

    tcount = Tab_GetCount ( g_hTab );

    if ( tcount > 1 ) // do we have more than one tab open?
    {
        // aye, set font for all richedits, but only save settings once
        mod = Rich_GetModified( tab_data[0].hchild );
        Rich_SetFont ( tab_data[0].hchild, hWnd, &g_hFont, g_szinifile );
        Rich_SetModified ( tab_data[0].hchild, mod );
        Tab_SetImg ( g_hTab, 0, img_state[mod] );

        for ( i = 1; i < tcount; i++ )
        {
            mod = Rich_GetModified( tab_data[i].hchild );
            Rich_SetFontNoCF ( tab_data[i].hchild, g_hFont );
            Rich_SetFGColor ( tab_data[i].hchild, 
                hWnd, CC_INIRESTORE, g_szinifile );
            Rich_SetModified ( tab_data[i].hchild, mod );
            Tab_SetImg ( g_hTab, i, img_state[mod] );
        }
    }
    else // no, get the only one rich
    {
        hRich = tab_data[g_curtab].hchild;
        mod = Rich_GetModified ( hRich );
        Rich_SetFont ( hRich, hWnd, &g_hFont, g_szinifile );
        Rich_SetModified ( hRich, mod );
        Tab_SetImg ( g_hTab, g_curtab, img_state[mod] );
    }
    SB_StatusSetText ( g_hStatus, 1, TEXT("") );
    SB_StatusSetText ( g_hStatus, 2, TEXT("") );
    SB_StatusResizePanels ( g_hStatus );
    g_fontheight = Rich_GetFontHeight ( tab_data[g_curtab].hchild );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_Print 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Print menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_Print ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    BOOL        mod;
    HWND        hRich;

    hRich = tab_data[g_curtab].hchild;

    // temporarily set richedit with black text on white bg :-)
    mod = Rich_GetModified( hRich );
    BeginDraw ( hRich );
    Rich_SetBGColor ( hRich, hWnd, RGB ( 255, 255, 255 ), NULL );
    Rich_SetFGColor ( hRich, hWnd, RGB ( 0, 0, 0 ), NULL );

    __try
    {
        Rich_Print ( hRich, hWnd, 
            FILE_Extract_filename ( tab_data[g_curtab].fpath ) );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ShowMessage ( hWnd, 
            TEXT("Error printing"), MB_OK, IDI_APP );
    }

    Rich_SetBGColor ( hRich, hWnd, CC_INIRESTORE, g_szinifile );
    Rich_SetFGColor ( hRich, hWnd, CC_INIRESTORE, g_szinifile );
    Rich_SetModified ( hRich, mod );

    if ( mod )
        Tab_SetImg ( g_hTab, g_curtab, TIDX_MOD );
    else
        Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );

    EndDraw ( hRich );
    SB_StatusSetText ( g_hStatus, 1, TEXT("") );
    SB_StatusSetText ( g_hStatus, 2, TEXT("") );
    SB_StatusResizePanels ( g_hStatus );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_SetFGColor 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION:  Set FG color menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_SetFGColor ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    BOOL       mod;
    size_t     tcount, i;
    HWND       hRich;
    int        img_state[2] = {TIDX_SAV, TIDX_MOD};

    tcount = Tab_GetCount ( g_hTab );

    if ( tcount > 1 ) // do we have more than one tab open?
    {
        mod = Rich_GetModified( tab_data[0].hchild );
        // pass invalid color to force pickcolordlg
        Rich_SetFGColor ( tab_data[0].hchild, hWnd, 
            CC_USERDEFINED, g_szinifile );
        Rich_SetModified ( tab_data[0].hchild, mod );
        Tab_SetImg ( g_hTab, 0, img_state[mod] );

        for ( i = 1; i < tcount; i++ )
        {
            mod = Rich_GetModified( tab_data[i].hchild );
            // set the already saved values for all other tabs
            Rich_SetFGColor ( tab_data[i].hchild, hWnd, 
                CC_INIRESTORE, g_szinifile );
            Rich_SetModified ( tab_data[i].hchild, mod );
            Tab_SetImg ( g_hTab, i, img_state[mod] );
        }
    }
    else
    {
        hRich = tab_data[g_curtab].hchild;
        mod = Rich_GetModified( hRich );
        BeginDraw ( hRich );
        // pass invalid color to force pickcolordlg
        Rich_SetFGColor ( hRich, hWnd, CC_USERDEFINED, g_szinifile ); 
        Rich_SetModified ( hRich, mod );
        Tab_SetImg ( g_hTab, g_curtab, img_state[mod] );
        EndDraw ( hRich );
    }
    SB_StatusSetText ( g_hStatus, 1, TEXT("") );
    SB_StatusSetText ( g_hStatus, 2, TEXT("") );
    SB_StatusResizePanels ( g_hStatus );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_SetBGColor 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Set BG color menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_SetBGColor ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    size_t     tcount, i;
    BOOL       result;

    tcount = Tab_GetCount ( g_hTab );

    if ( tcount > 1 ) // do we have more than one tab open?
    {
        // pass invalid color to force pickcolordlg
        result = Rich_SetBGColor ( tab_data[0].hchild, hWnd, 
            CC_USERDEFINED, g_szinifile );
        // set the already saved values for all other tabs
        for ( i = 1; i < tcount; i++ )
            Rich_SetBGColor ( tab_data[i].hchild, hWnd, 
                CC_INIRESTORE, g_szinifile );
    }
    else
    {
        // pass invalid color to force pickcolordlg
        result = Rich_SetBGColor ( tab_data[g_curtab].hchild, hWnd, 
            CC_USERDEFINED, g_szinifile );
    }

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_RunExtTool 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Run external tools menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_RunExtTool ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    if ( g_thread_working )
    {
        g_thread_working    = FALSE;
        PostThreadMessage ( g_tid, WM_ABTEXECTOOL, 0, 0 );
        return FALSE;
    }

    if ( DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_EXT ), hWnd, 
            DLG_EXTToolDlgProc, ( LPARAM ) hWnd ) != IDCANCEL )
    {
        g_thread_working    = TRUE;
        g_szoldmenu         = GetMenuItemText ( g_hToolsmenu, IDM_RUNEXT );
        SetMenuItemText ( g_hToolsmenu, IDM_RUNEXT, 
            TEXT("Executing external tool... click to abort") );
        ETEnableMenus ( FALSE );

        if ( g_thandle )
        {
            CloseHandle ( (HANDLE)g_thandle );
            g_thandle = 0;
        }
        
        ListBox_ResetContent ( g_hOutwList );
        OutwInsertTimeMarker ( g_hOutwList, 
            TEXT(">>>> Process started at "), NULL );

        g_thandle = _beginthreadex ( NULL, 0, 
            Thread_ExecTool, &etd, 0, &g_tid );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: UpdateEncodingMenus 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HMENU menu   : 
//    Param.    2: int encoding : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Update encoding menus as needed (select file encoding)
/*--------------------------------------------------------------------@@-@@-*/
BOOL UpdateEncodingMenus ( HMENU menu, int encoding )
/*--------------------------------------------------------------------------*/
{
    int         enc;
    int         menu_idx[13] = { 6, 7, 14, 15, 9, 10,
                                16, 17, 11, 12, 19,
                                19, 19 };
/*
6       "Save as UTF8"
7       "Save as UTF8 (BOM)"
9       "Save as UTF16-LE"
10      "Save as UTF16-BE"
11      "Save as UTF16-LE (BOM)"
12      "Save as UTF16-BE (BOM)"
14      "Save as UTF32-LE"
15      "Save as UTF32-BE"
16      "Save as UTF32-LE (BOM)"
17      "Save as UTF32-BE (BOM)"
19      "Save as ANSI"
*/
    enc = ( encoding > E_ANSI && encoding < E_UTF_8  ) ? E_UTF_8 : encoding;

    if ( menu == NULL  )
        return FALSE;

    // 6 = UTF8, 19 = ANSI
    return CheckMenuRadioItem ( menu, 6, 19, 
        menu_idx[enc-E_UTF_8], MF_BYPOSITION ); 
}


/*-@@+@@--------------------------------------------------------------------*/
//       Function: TD_UpdateTabDataEntry 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: TAB_DATA * pdata   : array with TABDATA elements
//    Param.    2: DWORD flags        : what to update
//    Param.    3: int index          : index in the array
//    Param.    4: int childid        : rich ID
//    Param.    5: int encoding       : encoding type
//    Param.    6: int cmdshow        : show command
//    Param.    7: HWND hchild        : rich hwnd
//    Param.    8: const TCHAR * path : path to opened file
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Update a TAB_DATA entry element desired members
/*--------------------------------------------------------------------@@-@@-*/
BOOL TD_UpdateTabDataEntry ( TAB_DATA * pdata, DWORD flags, int index, 
    int childid, int encoding, int cmdshow, HWND hchild, const TCHAR * path )
/*--------------------------------------------------------------------------*/
{
    const TCHAR   * temp = NULL;

    if ( index > MAX_TABS-1 )
        return FALSE;

    if ( path != NULL )
        temp = FILE_Extract_full_fname ( path );

    if ( flags == TD_ALL )
    {
        pdata[index].child_id   = childid;
        pdata[index].hchild     = hchild;
        pdata[index].tab_index  = index;
        pdata[index].fencoding  = encoding;
        pdata[index].nCmdShow   = cmdshow;

        if ( path != NULL )
        {
            StringCchCopy ( pdata[index].fpath, 
                ARRAYSIZE(pdata[index].fpath ), path );
            StringCchCopy ( pdata[index].fname, 
                ARRAYSIZE(pdata[index].fname ), 
                    ( temp == NULL ) ? path : temp );
        }
        else
        {
            StringCchCopy ( pdata[index].fpath, 
                ARRAYSIZE(pdata[index].fpath), TEXT("Untitled") );
            StringCchCopy ( pdata[index].fname, 
                ARRAYSIZE(pdata[index].fname), TEXT("Untitled") );
        }
        return TRUE;
    }
    else
    {
        if ( flags & TD_TINDEX ) pdata[index].tab_index     = index;
        if ( flags & TD_CHLDID ) pdata[index].child_id      = childid;
        if ( flags & TD_FENCOD ) pdata[index].fencoding     = encoding;
        if ( flags & TD_CHLDHW ) pdata[index].hchild        = hchild;
        if ( flags & TD_FCMDSW ) pdata[index].nCmdShow      = cmdshow;
        if ( flags & TD_FPATH )
        {
            if ( path != NULL )
                StringCchCopy ( pdata[index].fpath, 
                    ARRAYSIZE(pdata[index].fpath), path );
            else
                StringCchCopy ( pdata[index].fpath, 
                    ARRAYSIZE(pdata[index].fpath), TEXT("Untitled") );
        }
        if ( flags & TD_FNAME )
        {
            if ( path != NULL )
                StringCchCopy ( pdata[index].fname, 
                    ARRAYSIZE(pdata[index].fname), 
                        ( temp == NULL ) ? path : temp );
            else
                StringCchCopy ( pdata[index].fname, 
                    ARRAYSIZE(pdata[index].fname), TEXT("Untitled") );
        }

        return TRUE;
    }
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: TD_RebuildTabData 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: TAB_DATA * pdata: array with TAB_DATA elems
//    Param.    2: int ex_element  : deleted elem
//    Param.    3: int elements    : elements in array
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Rebuild a contiguous array of TAB_DATA elems 
//                 after a deletion
/*--------------------------------------------------------------------@@-@@-*/
BOOL TD_RebuildTabData ( TAB_DATA * pdata, int ex_element, int elements )
/*--------------------------------------------------------------------------*/
{
    TAB_DATA    * ptemp;
    int         i_src, i_dest;
    size_t      data_size, alloc_size;
    BOOL        result;

    if ( (pdata == NULL) || (ex_element >= elements) )
        return FALSE;

    result      = TRUE;
    data_size   = sizeof ( TAB_DATA ) * elements;
    // allocate in 4k chunks
    alloc_size  = ALIGN_4K(data_size);
    // alloc enough mem to hold the actual data                
    ptemp       = alloc_and_zero_mem ( alloc_size );

    if ( ptemp == NULL )
        return FALSE;

    __try
    {
        // copy all entries, wipe destination
        RtlCopyMemory ( ptemp, pdata, data_size );                  
        RtlZeroMemory ( pdata, data_size );                         

        // write back, minus the ex_element
        for ( i_src = 0, i_dest = 0; i_src < elements; i_src++ )    
        {
            if ( i_src == ex_element )
                continue;

            RtlCopyMemory ( &pdata[i_dest], &ptemp[i_src], 
                sizeof ( TAB_DATA ) );

            i_dest++;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        result = FALSE;
    }

    free_mem ( ptemp );

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: UpdateCaretPos 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hrich         : richedit control
//    Param.    2: BOOL resize_panels : resize SB panels?
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Show caret position in the statusbar
/*--------------------------------------------------------------------@@-@@-*/
void UpdateCaretPos ( HWND hrich, BOOL resize_panels )
/*--------------------------------------------------------------------------*/
{
    CARET_POS      cp;
    TCHAR          temp[64];

    Rich_GetCaretPos( hrich, &cp );
    StringCchPrintf ( temp, ARRAYSIZE(temp), 
        TEXT(" ln %8lu, col %4lu"), cp.c_line+1, cp.c_col+1 );
    SB_StatusSetText ( g_hStatus, 0, temp );

    if ( resize_panels )
        SB_StatusResizePanels ( g_hStatus );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_WWrap 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Change wordwrap on the active tab
/*--------------------------------------------------------------------@@-@@-*/
void Menu_WWrap ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    size_t     tcount, i;
    RECT       wr;

    tcount  = Tab_GetCount ( g_hTab );
    g_bwrap = !g_bwrap;
    GetClientRect ( g_hMain, &wr );
    BeginDraw ( g_hMain );

    if ( tcount > 1 ) // do we have more than one tab open?
    {
        Rich_WordWrap ( tab_data[0].hchild, g_bwrap );
        for ( i = 1; i < tcount; i++ )
            Rich_WordWrap ( tab_data[i].hchild, g_bwrap );
    }
    else // no, get the only one rich
        Rich_WordWrap ( tab_data[g_curtab].hchild, g_bwrap );

    if ( g_bwrap )
        WritePrivateProfileString ( 
            TEXT("editor"), TEXT("wrap"), TEXT("1"), g_szinifile );
    else
        WritePrivateProfileString ( 
            TEXT("editor"), TEXT("wrap"), TEXT("0"), g_szinifile );

    EndDraw ( g_hMain );
    RedrawWindow ( g_hMain, NULL, NULL, 
        RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: UpdateModifiedStatus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL resize_panels : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Show file modified status in the statusbar
/*--------------------------------------------------------------------@@-@@-*/
void UpdateModifiedStatus ( BOOL resize_panels )
/*--------------------------------------------------------------------------*/
{
    TCHAR   temp[TMAX_PATH];

    if ( Rich_GetModified ( tab_data[g_curtab].hchild ) )
    {
        SB_StatusSetText ( g_hStatus, 1, TEXT("Modified") );
        StringCchPrintf ( temp, ARRAYSIZE(temp), 
            TEXT("Current size %lu chars"), 
                GetWindowTextLength ( tab_data[g_curtab].hchild ) );
        SB_StatusSetText ( g_hStatus, 2, temp );
        Tab_SetImg ( g_hTab, g_curtab, TIDX_MOD );
    }
    else
    {
        SB_StatusSetText ( g_hStatus, 1, TEXT("") );
        SB_StatusSetText ( g_hStatus, 2, TEXT("") );
        Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
    }

    if ( resize_panels )
        SB_StatusResizePanels ( g_hStatus );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: UpdateEncodingStatus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL resize_panels : Resize SB panels?
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Show encoding in the statusbar
/*--------------------------------------------------------------------@@-@@-*/
void UpdateEncodingStatus ( BOOL resize_panels )
/*--------------------------------------------------------------------------*/
{
    TCHAR   temp[TMAX_PATH];
    float   file_size;

    if ( lstrcmp ( tab_data[g_curtab].fpath, TEXT("Untitled") ) != 0 )
    {
        file_size = (float)FILE_GetFileSize ( tab_data[g_curtab].fpath );
        StringCchPrintf ( temp, ARRAYSIZE(temp), CCH_SPEC,
            ENC_EncTypeToStr ( tab_data[g_curtab].fencoding ),
            ((file_size > 4096) ? (file_size / 1024) : file_size),
            ((file_size > 4096) ? TEXT("kbytes") : TEXT("bytes")));

        SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, temp );
    }
    else
        SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, TEXT("") );

    if ( resize_panels )
        SB_StatusResizePanels ( g_hStatus );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_Open 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Open a file in the same tab
/*--------------------------------------------------------------------@@-@@-*/
void Menu_Open ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    TCHAR       * temp;
    TCHAR       * temp2;
    int         tcount;
    BOOL        newtab = FALSE;

    temp        = (TCHAR *)alloc_and_zero_mem ( ALIGN_4K(OFN_BUF_SIZE) );
    temp2       = (TCHAR *)alloc_and_zero_mem ( ALIGN_4K(OFN_BUF_SIZE) );

    if ( temp == NULL || temp2 == NULL )
    {
        ShowMessage ( hWnd, 
            TEXT("Error allocating buffers for Open File dialog :-/"), 
                MB_OK, IDI_APP );
        return;
    }

    tcount      = Tab_GetCount ( g_hTab );
    temp[0]     = TEXT('\0');

    if ( tcount == MAX_TABS ) // all open
    {
        if ( !Prompt_to_save() )
        {
            free_mem ( temp );
            free_mem ( temp2 );
            return;
        }
    }
    else
    {
        if ( Rich_GetModified ( tab_data[g_curtab].hchild ) || 
            ( lstrcmp ( tab_data[g_curtab].fpath, TEXT("Untitled") ) != 0 ) )

            newtab = TRUE;
    }

    DLG_OpenDlg ( hWnd, temp, OFN_BUF_SIZE / sizeof (TCHAR), TRUE, newtab );

    free_mem ( temp );
    free_mem ( temp2 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_Quit 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Quit menu handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_Quit ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hWnd, WM_CLOSE, 0, 0 );
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: QuitSaveCheck 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Do we need to save before bye bye?
/*--------------------------------------------------------------------@@-@@-*/
BOOL QuitSaveCheck ( void )
/*--------------------------------------------------------------------------*/
{
    int        tcount, i, old_tab;
    BOOL       result = FALSE;

    tcount = Tab_GetCount ( g_hTab );
    old_tab = g_curtab;

    for ( i = 0; i < tcount; i++ )
    {
        g_curtab = i;
        result = Prompt_to_save();
        if ( !result )
        {
            g_curtab = old_tab;
            SetFocus ( tab_data[g_curtab].hchild );
            return FALSE;
        }
    }

    g_curtab = old_tab;

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: RemoveTab 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hTab : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Remove a tab from a tab control
/*--------------------------------------------------------------------@@-@@-*/
BOOL RemoveTab ( HWND hTab )
/*--------------------------------------------------------------------------*/
{
    int    tab_index;

    if ( ( Tab_GetCount ( hTab ) == 1 ) || !Prompt_to_save() )
        return FALSE;

    tab_index = g_curtab;
    BeginDraw ( g_hMain ); // here we go again

    if ( !Tab_DeletePage ( hTab, tab_index ) )
    {
        EndDraw ( g_hMain );
        RedrawWindow ( g_hMain, NULL, NULL, 
            RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
        return FALSE;
    }

    DestroyWindow ( tab_data[tab_index].hchild );
    TD_RebuildTabData ( tab_data, tab_index, ARRAYSIZE(tab_data) );
    g_curtab = Tab_GetCount ( hTab ) - 1;
    Tab_SetCurSel ( hTab, g_curtab );
    ShowWindow ( tab_data[g_curtab].hchild, SW_SHOW );
    tab_data[g_curtab].nCmdShow = SW_SHOW;
    SetFocus ( tab_data[g_curtab].hchild );
    UpdateCaretPos ( tab_data[g_curtab].hchild, FALSE );
    UpdateModifiedStatus ( FALSE );
    UpdateEncodingStatus ( TRUE );
    UpdateTitlebar ( g_hMain, tab_data[g_curtab].fpath, app_title );
    EndDraw ( g_hMain );
    RedrawWindow ( g_hMain, NULL, NULL, 
        RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
    // hello flicker my ol' frieeennn'
    // you've come to kick my ass againnnn
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnENDCVTABS 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Custom WM_ENDCVTABS handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnENDCVTABS ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    g_thread_working = FALSE;
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, TEXT("Done!") );
    EnableWindow ( g_hTab, TRUE );

    UpdateModifiedStatus ( TRUE );

    SetMenuItemText ( g_hToolsmenu, IDM_TABS, g_szoldmenu );
    EndDraw ( tab_data[g_curtab].hchild );
    CvtEnableMenus ( TRUE );
    g_bmenu_needs_update = TRUE;

    OutwInsertTimeMarker ( g_hOutwList, TEXT(">>>> Process ended at "), NULL );

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnUPDCVTABS 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_UPDCVTABS
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnUPDCVTABS ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR temp[128];
    
    StringCchPrintf ( temp, ARRAYSIZE(temp), 
        TEXT("Converting tabs to %zu spaces, "
            "hold on to your butt... %zu tabs converted"), wParam, lParam );

    return SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, temp );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnENDCFDECL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_ENDCFDECL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnENDCFDECL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int ic;

    g_thread_working = FALSE;
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
        (( wParam != 0 ) ? TEXT("Done!") : TEXT("Done, nothing found!")) );
    EnableWindow ( g_hTab, TRUE );

    UpdateModifiedStatus ( TRUE );

    SetMenuItemText ( g_hToolsmenu, IDM_CDECL_ALL, g_szoldmenu );
    CFDeclEnableMenus ( TRUE );
    g_bmenu_needs_update = TRUE;

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT("/*---------- Command ended at "), TEXT(" ----------*/") );

    if ( wParam )
    {
        ic = ListBox_GetCount ( g_hOutwList );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
        OutwUpdateTitle ( g_hOutWnd, ic );

        if ( !IsWindowVisible ( g_hOutWnd ) )
            ShowWindow ( g_hOutWnd, SW_SHOW );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnUPDCFDECL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_UPDCFDECL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnUPDCFDECL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR temp[128];
    
    StringCchPrintf ( temp, ARRAYSIZE(temp), 
        TEXT("Searching for C function "
            "declarations... line %zu of %zu"), lParam, wParam );

    return SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, temp );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnENDCFDEFS 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_ENDCFDEFS
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnENDCFDEFS ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int ic;

    g_thread_working = FALSE;
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
        (( wParam != 0 ) ? TEXT("Done!") : TEXT("Done, nothing found!")) );
    EnableWindow ( g_hTab, TRUE );

    UpdateModifiedStatus ( TRUE );

    SetMenuItemText ( g_hToolsmenu, IDM_CDEFS_ALL, g_szoldmenu );
    CFDefsEnableMenus ( TRUE );
    g_bmenu_needs_update = TRUE;

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT("/*---------- Command ended at "), TEXT(" ----------*/") );

    if ( wParam )
    {
        ic = ListBox_GetCount ( g_hOutwList );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
        OutwUpdateTitle ( g_hOutWnd, ic );

        if ( !IsWindowVisible ( g_hOutWnd ) )
            ShowWindow ( g_hOutWnd, SW_SHOW );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnUPDCFDEFS 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_UPDCFDEFS
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnUPDCFDEFS ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR temp[128];

    StringCchPrintf ( temp, ARRAYSIZE(temp), 
        TEXT("Searching for C function "
            "definitions... line %zu of %zu"), lParam, wParam );
    return SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, temp );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnENDCFDEFSSEL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_ENDCFDEFSSEL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnENDCFDEFSSEL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int ic;

    g_thread_working = FALSE;
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
        (( wParam != 0 ) ? TEXT("Done!") : TEXT("Done, nothing found!")) );
    EnableWindow ( g_hTab, TRUE );

    UpdateModifiedStatus ( TRUE );

    SetMenuItemText ( g_hToolsmenu, IDM_CDEFS_SEL, g_szoldmenu );
    CFDefsSelEnableMenus ( TRUE );
    g_bmenu_needs_update = TRUE;

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT("/*---------- Command ended at "), TEXT(" ----------*/") );

    if ( wParam )
    {
        ic = ListBox_GetCount ( g_hOutwList );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
        OutwUpdateTitle ( g_hOutWnd, ic );

        if ( !IsWindowVisible ( g_hOutWnd ) )
            ShowWindow ( g_hOutWnd, SW_SHOW );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnUPDCFDEFSSEL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_UPDCFDEFSSEL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnUPDCFDEFSSEL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR temp[128];
    
    StringCchPrintf ( temp, ARRAYSIZE(temp), 
        TEXT("Searching for C function "
            "definitons... line %zu of %zu"), lParam, wParam );

    return SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, temp );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnENDCFDECLSEL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_ENDCFDECLSEL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnENDCFDECLSEL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int ic;

    g_thread_working = FALSE;
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
        (( wParam != 0 ) ? TEXT("Done!") : TEXT("Done, nothing found!")) );
    EnableWindow ( g_hTab, TRUE );

    UpdateModifiedStatus ( TRUE );

    SetMenuItemText ( g_hToolsmenu, IDM_CDECL_SEL, g_szoldmenu );
    CFDeclSelEnableMenus ( TRUE );
    g_bmenu_needs_update = TRUE;

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT("/*---------- Command ended at "), TEXT(" ----------*/") );

    if ( wParam )
    {
        ic = ListBox_GetCount ( g_hOutwList );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
        OutwUpdateTitle ( g_hOutWnd, ic );

        if ( !IsWindowVisible ( g_hOutWnd ) )
            ShowWindow ( g_hOutWnd, SW_SHOW );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnUPDCFDECLSEL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_UPDCFDECLSEL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnUPDCFDECLSEL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR temp[128];
    
    StringCchPrintf ( temp, ARRAYSIZE(temp), 
        TEXT("Searching for C function "
            "declarations... line %zu of %zu"), lParam, wParam );

    return SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, temp );    
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnENDEXECTOOL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_ENDEXECTOOL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnENDEXECTOOL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR               temp[TMAX_PATH];
    EXEC_THREAD_DATA    * petd;
    int                 ic;

    g_thread_working    = FALSE;
    petd                = (EXEC_THREAD_DATA *)lParam;

    switch ( petd->exectype )
    {
        case EXEC_EXTTOOL:
            ETEnableMenus ( TRUE );
            SetMenuItemText ( g_hToolsmenu, IDM_RUNEXT, g_szoldmenu );
            break;

        case EXEC_LASTCMD:
            ETLastEnableMenus ( TRUE );
            SetMenuItemText ( g_hToolsmenu, IDM_RUNLAST, g_szoldmenu );
            break;

        case EXEC_USERMNU:
            ETUserEnableMenus ( TRUE );
            SetMenuItemText ( g_hUsermenu, 
                (int)(IDM_USERSTART+g_umenu_idx), g_szoldmenu );
            break;

        default:
            break;
    }

    g_bmenu_needs_update = TRUE;

    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT(">>>> Process ended at "), NULL );

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    if ( wParam )
    {
        if ( etd.capture )
        {   
            ic = ListBox_GetCount ( g_hOutwList );
            ListBox_SetTopIndex ( g_hOutwList, ic-1 );
            OutwUpdateTitle ( g_hOutWnd, ic );

            if ( !IsWindowVisible ( g_hOutWnd ) )
                ShowWindow ( g_hOutWnd, SW_SHOW );
        }

        return TRUE;
    }
    else
    {
        if ( petd != NULL )
        {
        #ifdef UNICODE
            StringCchPrintf ( temp, ARRAYSIZE(temp), 
                L"Error creating process with commandline %ls", petd->cmd );
        #else
            StringCchPrintf ( temp, ARRAYSIZE(temp), 
                "Error creating process with commandline %s", petd->cmd );
        #endif
            ShowMessage ( hWnd, temp, MB_OK, IDI_APP );
        }
        else
            ShowMessage ( hWnd, 
                TEXT("Invalid thread data received :("), MB_OK, IDI_APP );

        return FALSE;
    }
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnENDFALL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 01.10.2020
//    DESCRIPTION: WM_ENDFALL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnENDFALL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int ic;

    g_thread_working = FALSE;
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
        (( wParam != 0 ) ? TEXT("Done!") : TEXT("Done, nothing found!")) );
    EnableWindow ( g_hTab, TRUE );

    UpdateModifiedStatus ( TRUE );

    SetMenuItemText ( g_hEditmenu, IDM_FIND, g_szoldmenu );
    EndDraw ( tab_data[g_curtab].hchild );

    FAllEnableMenus ( TRUE );
    g_bmenu_needs_update = TRUE;

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    OutwInsertTimeMarker ( g_hOutwList, TEXT(">>>> Process ended at "), NULL );

    if ( wParam )
    {
        ic = ListBox_GetCount ( g_hOutwList );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
        OutwUpdateTitle ( g_hOutWnd, ic );

        if ( !IsWindowVisible ( g_hOutWnd ) )
            ShowWindow ( g_hOutWnd, SW_SHOW );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnUPDFALL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 01.10.2020
//    DESCRIPTION: WM_UPDFALL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnUPDFALL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR temp[128];
    
    StringCchPrintf ( temp, ARRAYSIZE(temp), 
        TEXT("Searching ... line %zu"), wParam );

    return SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, temp );    
}

#ifdef HAVE_HTMLHELP
/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnHelp 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: hWnd   : 
//    Param.    2: wParam : 
//    Param.    3: lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 27.05.2021
//    DESCRIPTION: WM_HELP
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnHelp ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    HELPINFO    * pHi;
    DWORD_PTR   dwCtrlId;

    pHi = (HELPINFO *)lParam;

    if ( pHi == NULL )
        return FALSE;
    
    dwCtrlId = pHi->iCtrlId;

    // for the html help purposes, force the same ctrlid for all opened
    // editor tabs (display the same help page).

    if ( (dwCtrlId > IDC_RICH) && (dwCtrlId <= (IDC_RICH + MAX_TABS)) )
        dwCtrlId = IDC_RICH;

    // call HtmlHelp with the menu/control ID (as in resource.h)
    // so we have context help; you must, however, build the CHM
    // with [MAP] and [ALIAS] sections that contains these ID's.
    // Put a CHM file with the same name and in the same folder 
    // as the executable.

    HtmlHelp ( hWnd, g_szchmfile, HH_HELP_CONTEXT, dwCtrlId );

    return TRUE;
}
#endif

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnDropFiles 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_DROPFILES event handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnDropFiles ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int         file_count, i, tab_count,
                free_slots, encoding;
    TCHAR       temp[128];
    HWND        hRich;
    BOOL        result;

    tab_count = Tab_GetCount ( g_hTab );

    if ( tab_count == MAX_TABS )
        return FALSE;  // silently go away if we're full on tabs

    result      = FALSE;
    hRich       = NULL;
    free_slots  = MAX_TABS - tab_count;
    file_count  = DragQueryFile ( (HANDLE) wParam, 0xFFFFFFFF, NULL, 0 );
    // have we dragged more than we can chew :-) ?
    file_count  = ( file_count > free_slots ) ? free_slots : file_count;

    SB_StatusSetSimple ( g_hStatus, TRUE );
    SB_StatusSetText ( g_hStatus, 255|SBT_NOBORDERS, TEXT("opening...") );

    for ( i = 0; i < file_count; i++ )
    {
        DragQueryFile ( (HANDLE) wParam, i, temp, ARRAYSIZE(temp) );
        encoding = ENC_FileCheckEncoding ( temp );

        if ( encoding == 0 )
            continue; // give the remaining others a chance :-)

        g_fencoding = encoding;

        if ( !Menu_New ( g_hTab ) )
            return FALSE;

        hRich = tab_data[g_curtab].hchild;
        result = Rich_Streamin ( hRich, NULL, 
            (EDITSTREAMCALLBACK)STR_StreamInProc, encoding, temp );

        if ( result )
        {
            Rich_SetModified ( hRich, FALSE );
            TD_UpdateTabDataEntry ( tab_data, 
                TD_FNAME|TD_FPATH|TD_FENCOD, g_curtab, 0, 
                    encoding, 0, 0, temp );
            Tab_SetText ( g_hTab, g_curtab, tab_data[g_curtab].fname );
            Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
        }
    }

    UpdateTitlebar ( g_hMain, tab_data[g_curtab].fpath, app_title );
    SB_StatusSetSimple ( g_hStatus, FALSE );
    UpdateModifiedStatus ( FALSE );
    UpdateEncodingStatus ( FALSE );
    UpdateCaretPos ( hRich, TRUE );
    DragFinish ( (HANDLE) wParam );

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnNotify 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Process WM_NOTIFY events for the main window
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnNotify ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int    tab_index;

    if ( g_thread_working )
        return FALSE;

    switch (((LPNMHDR)lParam)->code)
    {
        case TCN_SELCHANGING: // about to change tab...
            tab_index = Tab_GetCurSel ( g_hTab );
            // remove the god damn flicker >:-/ part 1.. 
            // feeze main window when about to change tabs
            BeginDraw ( hWnd ); 
            // I may be incompetent, but resizing windows 
            // with children flicker free is waay too much
            if (tab_index != -1)  
            {
                ShowWindow ( tab_data[tab_index].hchild, SW_HIDE );
                tab_data[tab_index].nCmdShow = SW_HIDE;
            }
            break;

        case TCN_SELCHANGE: // just changed tab
            tab_index = Tab_GetCurSel ( g_hTab );
            if (tab_index != -1)
            {
                g_curtab = tab_index;
                ShowWindow ( tab_data[tab_index].hchild, SW_SHOW );
                tab_data[tab_index].nCmdShow = SW_SHOW;
                SetFocus ( tab_data[tab_index].hchild );
                UpdateTitlebar ( g_hMain, 
                    tab_data[g_curtab].fpath, app_title );
                UpdateCaretPos ( tab_data[tab_index].hchild, FALSE );
                UpdateModifiedStatus ( FALSE );
                UpdateEncodingStatus ( TRUE );
            }
            // remove the god damn flicker, part 2... 
            // unfreeze the main window
            EndDraw ( hWnd ); 
            // ...and this is needed to force an update
            RedrawWindow ( g_hMain, NULL, NULL, 
                RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN ); 
            break;

        case EN_SELCHANGE:

            /*if ( g_thread_working )
                return FALSE;*/

            UpdateCaretPos ( tab_data[g_curtab].hchild, TRUE );
            g_bmenu_needs_update = TRUE;
            break;
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnCreate 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Process WM_CREATE events for the main window
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnCreate ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    g_hStatus      = SB_MakeStatusBar ( hWnd, IDC_STATUS );
    g_hTab         = Tab_NewTab ( hWnd );
    Tab_SetImageList ( g_hTab, g_hIml );
    g_tab_old_proc = ( WNDPROC ) 
        SetWindowLongPtr ( g_hTab, GWLP_WNDPROC, 
            ( LONG_PTR ) SC_TabSubclassProc );
    g_curtab       = Tab_NewTabPage ( g_hTab, 0, TEXT("Untitled") );

    if ( g_curtab == -1 )
    {
        ShowMessage ( hWnd, TEXT("Unable to create tab page :("), MB_OK, 0 );
        SendMessage ( hWnd, WM_CLOSE, 0, 0 );
        return FALSE;
    }

    // load and init the user menus, if any
    g_umenu_cnt     = LoadUserMenu ( g_hUsermenu, user_menu, g_szinifile );

    // it would seem appropriate to have a "old wnd proc" array to hold 
    // the original wndproc for each richedit ctrl.
    // so, wrote it that way originally. But it turns out that, for richedit, 
    // it's always the same procedure so we keep just an old proc for all 
    g_hRich         = Rich_NewRichedit ( g_hInst, &g_rich_old_proc, 
        SC_RichSubclassProc, g_hTab, IDC_RICH, &g_hFont, 
            g_bwrap, g_bmargin, g_szinifile );
    UpdateCaretPos ( g_hRich, TRUE );
    g_fontheight    = Rich_GetFontHeight ( g_hRich );
    TD_UpdateTabDataEntry ( tab_data, TD_ALL, 
        g_curtab, IDC_RICH, g_fencoding, SW_SHOW, g_hRich, NULL );
    Tab_SetPadding ( g_hTab, 10, 5 );
    // yes, this is actually needed .. otherwise 
    // Tab_SetPadding has no effect until next SetText :-))
    Tab_SetText ( g_hTab, g_curtab, tab_data[g_curtab].fname ); 
    Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
    DragAcceptFiles ( hWnd, TRUE ); // signal we accept Satan :-)

    g_hOutWnd       = DLG_OutputWnd_Modeless 
        ( hWnd, (LPARAM)TEXT("Output Window") );
    
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SizeOutWnd 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hParent : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Resize outwindow when parent is resized
/*--------------------------------------------------------------------@@-@@-*/
void SizeOutWnd ( HWND hParent )
/*--------------------------------------------------------------------------*/
{
    RECT            mw;
    int             frame_width, sb_height; // window frame wdth, 
                                            // statusbar height

    GetWindowRect ( hParent, &mw );

    frame_width     = GetSystemMetrics ( SM_CXFRAME );
    frame_width     *= 2;
    sb_height       = 23 + frame_width + 1;

    MoveWindow ( g_hOutWnd, mw.left+frame_width, 
        mw.bottom-(250+sb_height), 
            mw.right-mw.left-2*frame_width, 250, TRUE );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnSize 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Process WM_SIZE events for the main window
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnSize ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    RECT    rstat, tr;
    size_t  i;

    SendMessage ( g_hStatus, WM_SIZE, 0, 0 );
    GetClientRect ( g_hStatus, &rstat );
    MoveWindow ( g_hTab, 0, 0, LOWORD ( lParam ), 
        HIWORD ( lParam ) - rstat.bottom, FALSE );

    GetClientRect ( g_hTab, &tr );
    SendMessage ( g_hTab, TCM_ADJUSTRECT, 0, (LPARAM)&tr );

    for ( i = 0; i < (size_t)Tab_GetCount ( g_hTab ); i++ )
        MoveWindow ( tab_data[i].hchild, tr.left, tr.top, 
            tr.right-tr.left, tr.bottom-tr.top, TRUE );

    SizeOutWnd ( hWnd );

    // redraw SB and tab control... lord almighty :-)
    RedrawWindow ( g_hStatus, NULL, NULL, 
        RDW_ERASE|RDW_FRAME|RDW_INVALIDATE );
    RedrawWindow ( g_hTab, NULL, NULL, 
        RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN ); 

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnInitMenuPopup 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_INITPOPUP handler
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnInitMenuPopup ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    switch ( LOWORD ( lParam ) )
    {
        case IDMM_FILE:
            // the handle from wParam is unreliable,
            // since LOWORD ( lParam ) is 0 for both 
            // the file menu and the popup edit menu
            UpdateEncodingMenus ( g_hFilemenu, tab_data[g_curtab].fencoding );
            break;

        case IDMM_EDIT:
        case IDMM_TOOLS:
        case IDMM_VIEW:
            UpdateMenuStatus();
            break;
    }
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnCommand 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: process WM_COMMAND events for the main window
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnCommand ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    CHARRANGE       chr;
    TCHAR           temp[CF_MAX_LEN];

    if ( !lParam )
    {
        // are we in user defined range?
        if ( (LOWORD ( wParam ) >= IDM_USERSTART) && 
            (LOWORD ( wParam ) < IDM_USERSTART+MAX_USER_MENUS) )
        {
            g_umenu_idx = LOWORD ( wParam ) - IDM_USERSTART;
            Menu_RunUserCmd ( g_umenu_idx );
            return TRUE;
        }

        switch ( LOWORD ( wParam ) )
        {
            case IDM_RUNLAST:
                Menu_RunLastCmd ( hWnd );
                break;

            case IDM_NEW:
                Menu_New ( g_hTab );
                break;

            case IDM_NEWINST:
                Menu_NewInstance();
                break;

            case IDM_OPEN:
                Menu_Open ( hWnd );
                break;

            case IDM_MERGE:
                Menu_MergeFile ( hWnd );
                break;

            case IDM_SAVESEL:

                if (( g_fencoding > E_ANSI ) && ( g_fencoding < E_UTF_8 ))
                    g_fencoding = E_UTF_8;

                Menu_SaveSelection ( hWnd );
                break;

            case IDM_SAVE:
                Menu_Save ( hWnd );
                break;

            case IDM_SAVEAS:
                Menu_Saveas ( hWnd );
                break;

            case IDM_QUIT:
                Menu_Quit ( hWnd );
                break;

            case IDM_CUT:
                SendMessage ( tab_data[g_curtab].hchild, WM_CUT, 0, 0 );
                break;

            case IDM_COPY:
                SendMessage ( tab_data[g_curtab].hchild, WM_COPY, 0, 0 );
                break;

            case IDM_PASTE:
                SendMessage ( tab_data[g_curtab].hchild, 
                    EM_PASTESPECIAL, CF_UNICODETEXT, 0 );
                break;

            case IDM_DEL:
                SendMessage ( tab_data[g_curtab].hchild, 
                    EM_REPLACESEL, TRUE, 0 );
                break;

            case IDM_UNDO:
                SendMessage ( tab_data[g_curtab].hchild, 
                    EM_UNDO, 0, 0 );
                break;

            case IDM_REDO:
                SendMessage ( tab_data[g_curtab].hchild, 
                    EM_REDO, 0, 0 );
                break;

            case IDM_SELALL:
                chr.cpMin = 0;
                chr.cpMax = -1;
                SendMessage ( tab_data[g_curtab].hchild, 
                    EM_EXSETSEL, 0, (LPARAM)&chr );
                break;

            case IDM_MARGIN:
                Menu_ToggleMargin ( g_hTab );
                break;

            case IDM_WRAP:
                Menu_WWrap ( hWnd );
                break;

            case IDM_ONTOP:
                Menu_Ontop ( hWnd );
                break;

            case IDM_GOTO:
                DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_GOTO ), 
                    hWnd, DLG_GotoDlgProc, ( LPARAM ) hWnd );
                break;

            case IDM_FIND:
                Menu_Find ( hWnd );
                break;

            case IDM_TABS:
                Menu_ConvertTabs ( hWnd );
                break;

            case IDM_FINDNEXT:

                if ( g_sztextbuf[0] != TEXT('\0') )
                    Rich_FindReplaceText ( tab_data[g_curtab].hchild,
                        hWnd, g_sztextbuf, NULL, g_bcase, 
                            g_bwhole, FALSE, g_bfwd );

                break;

            case IDM_FINDSEL:
                // find fwd
                Rich_FindNext ( tab_data[g_curtab].hchild, hWnd, TRUE ); 
                break;

            case IDM_FINDSEL_BK:
                // find bkwd
                Rich_FindNext ( tab_data[g_curtab].hchild, hWnd, FALSE ); 
                break;

            case IDM_ABOUT:
                Menu_About ( hWnd );
                break;

            #ifdef HAVE_HTMLHELP
            case IDM_HELP:
                HtmlHelp ( hWnd, g_szchmfile, HH_HELP_CONTEXT, IDM_HELP );
                break;
            #endif

            case IDM_CCOM:

                if ( CF_GetUserSelection ( tab_data[g_curtab].hchild, 
                        temp, ARRAYSIZE(temp) ) )
                            CF_CommentCFunction ( 
                                tab_data[g_curtab].hchild, temp );

                break;

            case IDM_FONT:
                Menu_SetFont ( hWnd );
                break;

            case IDM_CLEAR:
                SHAddToRecentDocs ( SHARD_PATH, NULL );
                break;

            case IDM_ECLIP:
                Menu_WipeClipboard ( hWnd );
                break;

            case IDM_RUN:
                RunFileDlg ( hWnd, g_hIcon, NULL, NULL, NULL, 0 );
                break;

            case IDM_WINFIND:
                SHFindFiles ( NULL, NULL );
                break;

            case IDM_PRINT:
                Menu_Print ( hWnd );
                break;

            case IDM_PROP:
                Menu_Properties ( hWnd );
                break;

            case IDM_VEXPLORE:
                Menu_OpenInBrowser ( hWnd );
                break;

            case IDM_COLOR:
                Menu_SetBGColor ( hWnd );
                break;

            case IDM_FGCOLOR:
                Menu_SetFGColor ( hWnd );
                break;

            case IDM_UPCASE:
                Rich_ConvertCase ( tab_data[g_curtab].hchild, FALSE );
                break;

            case IDM_LOWCASE:
                Rich_ConvertCase ( tab_data[g_curtab].hchild, TRUE );
                break;

            case IDM_RUNEXT:
                Menu_RunExtTool ( hWnd );
                break;

            case IDM_VIEWOUT:

                if ( IsWindowVisible ( g_hOutWnd ) )
                    ShowWindow ( g_hOutWnd, SW_HIDE );
                else
                {
                    ShowWindow ( g_hOutWnd, SW_SHOW );
                    SetFocus ( tab_data[g_curtab].hchild );
                }

                break;

            case IDM_VIEWHEX:
                Menu_HexView ( hWnd );
                break;

            case IDM_REFR:
                Menu_RefreshFromDisk ( hWnd );
                break;

            case IDM_UTF8:
                g_fencoding = E_UTF_8;
                tab_data[g_curtab].fencoding = E_UTF_8;
                break;

            case IDM_UTF8_BOM:
                g_fencoding = E_UTF_8_BOM;
                tab_data[g_curtab].fencoding = E_UTF_8_BOM;
                break;

            case IDM_UTF16_LE:
                g_fencoding = E_UTF_16_LE;
                tab_data[g_curtab].fencoding = E_UTF_16_LE;
                break;

            case IDM_UTF16_BE:
                g_fencoding = E_UTF_16_BE;
                tab_data[g_curtab].fencoding = E_UTF_16_BE;
                break;

            case IDM_UTF16_LE_BOM:
                g_fencoding = E_UTF_16_LE_BOM;
                tab_data[g_curtab].fencoding = E_UTF_16_LE_BOM;
                break;

            case IDM_UTF16_BE_BOM:
                g_fencoding = E_UTF_16_BE_BOM;
                tab_data[g_curtab].fencoding = E_UTF_16_BE_BOM;
                break;

            case IDM_UTF32_LE:
                g_fencoding = E_UTF_32_LE;
                tab_data[g_curtab].fencoding = E_UTF_32_LE;
                break;

            case IDM_UTF32_BE:
                g_fencoding = E_UTF_32_BE;
                tab_data[g_curtab].fencoding = E_UTF_32_BE;
                break;

            case IDM_UTF32_LE_BOM:
                g_fencoding = E_UTF_32_LE_BOM;
                tab_data[g_curtab].fencoding = E_UTF_32_LE_BOM;
                break;

            case IDM_UTF32_BE_BOM:
                g_fencoding = E_UTF_32_BE_BOM;
                tab_data[g_curtab].fencoding = E_UTF_32_BE_BOM;
                break;

            case IDM_ANSI:
                g_fencoding = E_ANSI;
                tab_data[g_curtab].fencoding = E_ANSI;
                break;

            case IDM_TAB_UP:
                CycleTabs ( hWnd, CT_UP );
                break;

            case IDM_TAB_DOWN:
                CycleTabs ( hWnd, CT_DOWN );
                break;

            case IDM_CDEFS_ALL:
                Menu_CFDefAll ( hWnd );
                break;

            case IDM_CDECL_ALL:
                Menu_CFDeclAll ( hWnd );
                break;

            case IDM_CDEFS_SEL:
                Menu_CFDefSel ( hWnd );
                break;

            case IDM_CDECL_SEL:
                Menu_CFDeclSel ( hWnd );
                break;
        }
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: OpenFileListFromCMDL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: TCHAR ** filelist: TCHAR ** filelist: array of 
//                                    pointers to fnames
//    Param.    2: int files        : elements in array
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Take a list of filenames and send them to available tabs
/*--------------------------------------------------------------------@@-@@-*/
BOOL OpenFileListFromCMDL ( TCHAR ** filelist, int files )
/*--------------------------------------------------------------------------*/
{
    size_t      i;
    int         encoding, free_slots;
    BOOL        result = FALSE;
    HWND        hRich = NULL;
    BOOL        newtab = FALSE;

    if ( filelist == NULL )
        return FALSE;

    // see how many tabs we have left
    free_slots  = MAX_TABS - Tab_GetCount ( g_hTab );

    if ( free_slots == 0 )
        return FALSE;

    newtab = ( Rich_GetModified ( tab_data[g_curtab].hchild ) ) || 
        ( lstrcmp ( tab_data[g_curtab].fpath, TEXT("Untitled") ) != 0 );

    // up if we're going to use the default as well
    if ( !newtab )
        free_slots++;

    SB_StatusSetSimple ( g_hStatus, TRUE );
    SB_StatusSetText ( g_hStatus, 255|SBT_NOBORDERS, TEXT("opening...") );

    for ( i = 1; i < (size_t)files; i++ ) // filelist[0] is the program.exe
    {
        encoding = ENC_FileCheckEncoding ( filelist[i] );

        // skip if some garbage (or future command param :-) )
        if ( encoding == 0 ) 
            continue;

        // occupy the first tab if "untitled" and not modified
        // but create tabs afterwards
        if ( (i > 1) && (newtab == FALSE) )
            newtab = TRUE;

        if ( newtab )
            if ( !Menu_New ( g_hTab ) )
                break;

        hRich = tab_data[g_curtab].hchild;
        result = Rich_Streamin ( hRich, NULL, 
            (EDITSTREAMCALLBACK)STR_StreamInProc, encoding, filelist[i] );

        if ( result )
        {
            Rich_SetModified ( hRich, FALSE );
            TD_UpdateTabDataEntry ( tab_data, 
                TD_FNAME|TD_FPATH|TD_FENCOD, g_curtab, 
                    0, encoding, 0, 0, filelist[i] );
            Tab_SetText ( g_hTab, g_curtab, tab_data[g_curtab].fname );
            Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
        }

        if ( i == free_slots )
            break;
    }
    UpdateTitlebar ( g_hMain, tab_data[g_curtab].fpath, app_title );
    SB_StatusSetSimple ( g_hStatus, FALSE );
    UpdateModifiedStatus ( FALSE );
    UpdateEncodingStatus ( FALSE );
    UpdateCaretPos ( hRich, TRUE );

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: OpenFileListFromOFD 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: TCHAR * filelist: file list as sent by GetOpenFilename
//    Param.    2: int fofset      : tchars form start to first filename
//    Param.    3: BOOL newtab     : use the default tab as well, or not
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Open a file list in tabs                                                                                      
//                 filelist is ofn.lpstrFile, as returned by GetOpenFilename, 
//                 and has the following structure: first string 
//                 (NULL terminated) is the crt. dir; next ones are the 
//                 selected filenames (NULL terminated as well); the end is 
//                 marked by a double NULL; fofset is ofn.nFileOffset and 
//                 represents how many tchars  (zero based) do we count from 
//                 the start of filelist to the beginning of the first 
//                 filename 
/*--------------------------------------------------------------------@@-@@-*/
BOOL OpenFileListFromOFD ( TCHAR * filelist, int fofset, BOOL newtab )
/*--------------------------------------------------------------------------*/
{
    int         i, j, len,
                encoding, free_slots;
    TCHAR       curdir[TMAX_PATH];
    TCHAR       fname[TMAX_PATH];
    TCHAR       temp[TMAX_PATH];
    HWND        hRich;
    BOOL        result = FALSE;

    len         = fofset;
    i           = fofset;
    j           = 0;
    hRich       = NULL;

    // how many free tabs do we have
    free_slots  = MAX_TABS - Tab_GetCount ( g_hTab );

    if ( free_slots == 0 )
        return FALSE;

    // up if we're going to use the default as well
    if ( !newtab )
        free_slots++;

    // save crt. dir
    StringCchCopyN ( curdir, ARRAYSIZE(curdir), filelist, len );

    // doctor crt. dir to always terminate in '\'
    while ( !curdir[len] )
        len--;

    if ( curdir[len] != TEXT('\\') )
        StringCchCat ( curdir, ARRAYSIZE(curdir), TEXT("\\") );

    do
    {
        // form a full path and check encoding
        StringCchCopy ( fname, ARRAYSIZE(fname), curdir );
        StringCchCat ( fname, ARRAYSIZE(fname), (TCHAR*)(&(filelist[i])) );
        encoding = ENC_FileCheckEncoding ( fname );

        // skip possible crap =)
        if ( encoding == 0 )
        {
            i += lstrlen ( (TCHAR*)(&(filelist[i])) );
            i++;
            continue;
        }

        g_fencoding = encoding;

        // occupy the first tab if "untitled" and not modified
        // but create tabs afterwards
        if ( (j > 0) && (newtab == FALSE) )
            newtab = TRUE;

        if ( newtab )
            if ( !Menu_New ( g_hTab ) )
            {
                SB_StatusSetSimple ( g_hStatus, FALSE );
                SB_StatusResizePanels ( g_hStatus );
                return FALSE;
            }

        // stream file in
        hRich  = tab_data[g_curtab].hchild;
        result = Rich_Streamin ( hRich, NULL, 
            (EDITSTREAMCALLBACK)STR_StreamInProc, encoding, fname );

        if ( result )
        {
            Rich_SetModified ( hRich, FALSE );
            TD_UpdateTabDataEntry ( tab_data, 
                TD_FNAME|TD_FPATH|TD_FENCOD, g_curtab, 
                    0, encoding, 0, 0, fname );
            Tab_SetText ( g_hTab, g_curtab, tab_data[g_curtab].fname );
            Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
        }
        else
        {
            StringCchPrintf ( temp, ARRAYSIZE(temp), CCH_SPEC2, fname );
            ShowMessage ( g_hMain, temp, MB_OK, IDI_APP );
        }

        // jump to the next filename
        i += lstrlen ( (TCHAR*)(&(filelist[i])) );
        i++;
        j++;

    }
    while ( (filelist[i] != 0) && (j < free_slots) );

    UpdateTitlebar ( g_hMain, tab_data[g_curtab].fpath, app_title );
    UpdateModifiedStatus ( FALSE );
    UpdateEncodingStatus ( FALSE );
    UpdateCaretPos ( hRich, TRUE );

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CycleTabs 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hWnd     : tab control
//    Param.    2: int direction : CT_UP/CT_DOWN
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Cycle tabs back and forth, on Ctrl+Tab or 
//                 Shift+Tab key combs.
/*--------------------------------------------------------------------@@-@@-*/
void CycleTabs ( HWND hWnd, int direction )
/*--------------------------------------------------------------------------*/
{
    int         tab_count, cur_tab, prev_tab;

    tab_count = Tab_GetCount ( g_hTab );

    if ( tab_count == 1 )
        return;

    cur_tab = Tab_GetCurSel ( g_hTab );

    if ( cur_tab == -1 )
        return;

    switch ( direction )
    {
        case CT_UP:
            cur_tab++;
            cur_tab %= tab_count;
            break;

        case CT_DOWN:
            cur_tab--;

            if ( cur_tab < 0 )
                cur_tab = tab_count-1;

            break;
    }

    prev_tab    = Tab_SetCurSel ( g_hTab, cur_tab );
    g_curtab    = cur_tab;
    ShowWindow ( tab_data[prev_tab].hchild, SW_HIDE );
    tab_data[prev_tab].nCmdShow     = SW_HIDE;
    ShowWindow ( tab_data[cur_tab].hchild, SW_SHOW );
    tab_data[cur_tab].nCmdShow      = SW_SHOW;
    SetFocus ( tab_data[cur_tab].hchild );
    UpdateTitlebar ( g_hMain, tab_data[cur_tab].fpath, app_title );
    UpdateCaretPos ( tab_data[cur_tab].hchild, FALSE );
    UpdateModifiedStatus ( FALSE );
    UpdateEncodingStatus ( TRUE );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: InitImgList 
/*--------------------------------------------------------------------------*/
//           Type: HIMAGELIST 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Create a imagelist, 16x16, color
/*--------------------------------------------------------------------@@-@@-*/
HIMAGELIST InitImgList ( void )
/*--------------------------------------------------------------------------*/
{
    HIMAGELIST      img;
    HINSTANCE       hinst;

    hinst = GetModuleHandle ( NULL );
    img = ImageList_Create ( 16, 16, ILC_COLOR32, 0, 8 );

    if ( img == NULL )
        return NULL;

    if ( ImageList_AddIcon ( img, LoadIcon 
            ( hinst, MAKEINTRESOURCE ( IDI_SAV ) ) ) == -1 )
                return NULL;

    if ( ImageList_AddIcon ( img, LoadIcon 
            ( hinst, MAKEINTRESOURCE ( IDI_MOD ) ) ) == -1 )
                return NULL;

    return img;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_Ontop 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Set main window to be "always on top", or not :-)
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_Ontop ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    LONG_PTR    ex_style;

    ex_style = GetWindowLongPtr ( hWnd, GWL_EXSTYLE );

    if ( ex_style )
    {
        g_bontop = !g_bontop;

        if ( g_bontop )
        {
            ex_style |= WS_EX_TOPMOST;
            SetWindowLongPtr ( hWnd, GWL_EXSTYLE, ex_style );
            SetWindowPos ( hWnd, HWND_TOPMOST, 0, 0, 0, 0, 
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
            WritePrivateProfileString ( 
                TEXT("window"), TEXT("ontop"), TEXT("1"), g_szinifile );
        }
        else
        {
            ex_style &= ~WS_EX_TOPMOST;
            SetWindowLongPtr ( hWnd, GWL_EXSTYLE, ex_style );
            SetWindowPos ( hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
            WritePrivateProfileString ( 
                TEXT("window"), TEXT("ontop"), TEXT("0"), g_szinifile );
        }

        return TRUE;
    }

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_Properties 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_Properties ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
#ifndef UNICODE // SHObjectProperties wants only unicode
    WCHAR   temp[MAX_PATH];
    int     wchars;

    wchars = MultiByteToWideChar ( CP_UTF8, 0, 
        tab_data[g_curtab].fpath, -1, NULL, 0 );

    MultiByteToWideChar ( CP_UTF8, 0, 
        tab_data[g_curtab].fpath, -1, (LPWSTR)temp, 
            (wchars > MAX_PATH) ? MAX_PATH : wchars );

    return SHObjectProperties ( hWnd, SHOP_FILEPATH, 
        (LPWSTR)temp, NULL );
#else
    return SHObjectProperties ( hWnd, SHOP_FILEPATH, 
        tab_data[g_curtab].fpath, NULL );
#endif
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_CFDefAll 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Find all C function definitions in a richedit
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_CFDefAll ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    if ( g_thread_working )
    {
        g_thread_working = FALSE;
        PostThreadMessage ( g_tid, WM_ABTCFDEFS, 0, 0 );
        return FALSE;
    }

    g_thread_working            = TRUE;

    ftd.hRich                   = tab_data[g_curtab].hchild;
    ftd.proc                    = FindCDefsProc;
    ftd.lParam                  = (LPARAM)g_hMain;

    EnableWindow ( g_hTab, FALSE );
    g_szoldmenu = GetMenuItemText ( g_hToolsmenu, IDM_CDEFS_ALL );
    SetMenuItemText ( g_hToolsmenu, IDM_CDEFS_ALL, 
        TEXT("Searching for C function definitions... click to abort") );

    CFDefsEnableMenus ( FALSE );
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
        TEXT("Searching for C function definitions...") );

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT("/*---------- Command started at "), TEXT(" ----------*/") );

    g_thandle = _beginthreadex ( NULL, 0, 
        Thread_CFFindDefs, &ftd, 0, &g_tid );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_CFDefSel 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Find all C functions definitions from selection
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_CFDefSel ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    if ( g_thread_working )
    {
        g_thread_working = FALSE;
        PostThreadMessage ( g_tid, WM_ABTCFDEFSSEL, 0, 0 );
        return FALSE;
    }

    g_thread_working            = TRUE;

    ftd.hRich                   = tab_data[g_curtab].hchild;
    ftd.proc                    = FindCDefsSelProc;
    ftd.lParam                  = (LPARAM)g_hMain;

    EnableWindow ( g_hTab, FALSE );
    g_szoldmenu = GetMenuItemText ( g_hToolsmenu, IDM_CDEFS_SEL );
    SetMenuItemText ( g_hToolsmenu, IDM_CDEFS_SEL, 
        TEXT("Searching for C function definitions... click to abort") );

    CFDefsSelEnableMenus ( FALSE );
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
        TEXT("Searching for C function definitions...") );

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT("/*---------- Command started at "), TEXT(" ----------*/") );

    g_thandle = _beginthreadex ( NULL, 0, 
        Thread_CFFindDefsSel, &ftd, 0, &g_tid );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_CFDeclAll 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Find all C functions declarations
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_CFDeclAll ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    if ( g_thread_working )
    {
        g_thread_working = FALSE;
        PostThreadMessage ( g_tid, WM_ABTCFDECL, 0, 0 );
        return FALSE;
    }

    g_thread_working            = TRUE;

    ftd.hRich                   = tab_data[g_curtab].hchild;
    ftd.proc                    = FindCDeclsProc;
    ftd.lParam                  = (LPARAM)g_hMain;

    EnableWindow ( g_hTab, FALSE );
    g_szoldmenu = GetMenuItemText ( g_hToolsmenu, IDM_CDECL_ALL );
    SetMenuItemText ( g_hToolsmenu, IDM_CDECL_ALL, 
        TEXT("Searching for C function declarations... click to abort") );

    CFDeclEnableMenus ( FALSE );
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
        TEXT("Searching for C function declarations...") );

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT("/*---------- Command started at "), TEXT(" ----------*/") );

    g_thandle = _beginthreadex ( NULL, 0, Thread_CFFindDecls, &ftd, 0, &g_tid );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_CFDeclSel 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Find all C functions declarations from selection
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_CFDeclSel ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    if ( g_thread_working )
    {
        g_thread_working = FALSE;
        PostThreadMessage ( g_tid, WM_ABTCFDECLSEL, 0, 0 );
        return FALSE;
    }

    g_thread_working            = TRUE;
 
    ftd.hRich                   = tab_data[g_curtab].hchild;
    ftd.proc                    = FindCDeclsSelProc;
    ftd.lParam                  = (LPARAM)g_hMain;

    g_szoldmenu = GetMenuItemText ( g_hToolsmenu, IDM_CDECL_SEL );
    CFDeclSelEnableMenus ( FALSE );
    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, 
        TEXT("Searching for C function declarations...") );

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT("/*---------- Command started at "), TEXT(" ----------*/") );

    g_thandle = _beginthreadex ( NULL, 0, 
        Thread_CFFindDeclsSel, &ftd, 0, &g_tid );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnCopyData 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Handle WM_COPYDATA message (sent by shell extension)
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnCopyData ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    COPYDATASTRUCT  * pcd;
    TCHAR           ** arglist;
    int             argc;
    BOOL            result = FALSE;

    // check if there's already an instance running and get out
    // this is so that only one instance handles WM_COPYDATA
    if ( g_bnot_alone )
        return result;

    pcd = (COPYDATASTRUCT *)(lParam);

    if ( pcd == NULL )
        return result;

    // check for our secret preshared key with 
    // the shell ctx menu extension :-))
    if ( pcd->dwData != COPYDATA_MAGIC )
        return result;

    __try
    {
        // use the same function from cmdline processing, 
        // since the received data is the same
        arglist = FILE_CommandLineToArgv ( (TCHAR *)(pcd->lpData), &argc );

        if ( argc >= 2 && arglist != NULL )
        {
            result = OpenFileListFromCMDL ( arglist, argc );
            RedrawWindow ( g_hTab, NULL, NULL, 
                RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
            // bring program window to foreground 
            // by momentarily making it topmost
            // ...but only if it isn't already 8-)
            if ( !g_bontop )
            {
                if ( IsIconic ( hWnd ) )
                    ShowWindow ( hWnd, SW_RESTORE );
                else
                {
                    ShowWindow ( hWnd, SW_MINIMIZE );
                    ShowWindow ( hWnd, SW_RESTORE );
                }

                SetWindowPos ( hWnd, HWND_TOPMOST, 0, 0, 0, 0, 
                    SWP_SHOWWINDOW| SWP_NOMOVE | SWP_NOSIZE );
                SetWindowPos ( hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
                    SWP_SHOWWINDOW| SWP_NOMOVE | SWP_NOSIZE );
            }
            else
            {
                if ( IsIconic ( hWnd ) )
                    ShowWindow ( hWnd, SW_RESTORE );
            }
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        result = FALSE;
    }

    if ( arglist != NULL )
        GlobalFree ( arglist );

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_OpenInBrowser 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Open crt file in default browser
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_OpenInBrowser ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    TCHAR       temp[TMAX_PATH];
    TCHAR       temp2[TMAX_PATH];
    TCHAR       * pbrowser;

    pbrowser    = GetDefaultBrowser();

    if ( pbrowser[0] != TEXT('\0') )
    {
        #ifdef UNICODE
            StringCchPrintf ( temp2, ARRAYSIZE(temp2), L"%ls ", pbrowser );
        #else
            StringCchPrintf ( temp2, ARRAYSIZE(temp2), "%s ", pbrowser );
        #endif
        StringCchCopy ( temp, ARRAYSIZE(temp), tab_data[g_curtab].fpath );
        PathQuoteSpaces ( temp );
        StringCchCat ( temp2, ARRAYSIZE(temp2), temp );

        return SimpleExec ( temp2, NULL );
    }

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_RunLastCmd 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Run last Execute External Tool cmd.
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_RunLastCmd ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    TCHAR   temp[TMAX_PATH];

    if ( g_thread_working )
    {
        g_thread_working = FALSE;
        PostThreadMessage ( g_tid, WM_ABTEXECTOOL, 0, 0 );
        return FALSE;
    }

    GetModuleFileName ( NULL, temp, ARRAYSIZE(temp) );
    RtlZeroMemory ( &etd, sizeof (EXEC_THREAD_DATA) );
    StringCchCopy ( etd.filename, ARRAYSIZE(etd.filename), 
        tab_data[g_curtab].fpath );
    GetPrivateProfileString ( TEXT("editor"), TEXT("last_cmd"), 
        TEXT("cmd.exe /c echo \"No Last Command To Run :-)\""), etd.cmd, 
            ARRAYSIZE(etd.cmd), g_szinifile );
    GetPrivateProfileString ( TEXT("editor"), TEXT("process working dir"), 
        FILE_Extract_path ( temp, FALSE ), etd.curdir, 
            ARRAYSIZE(etd.curdir), g_szinifile );

    etd.capture         = ( BOOL ) GetPrivateProfileInt ( 
        TEXT("editor"), TEXT("capture"), 0, g_szinifile );
    etd.lParam          = (LPARAM)g_hMain;
    etd.wParam          = (WPARAM)g_hOutwList;
    etd.proc            = (PBPROC)ExecToolProc;
    etd.exectype        = EXEC_LASTCMD;

    g_thread_working    = TRUE;

    g_szoldmenu         = GetMenuItemText ( g_hToolsmenu, IDM_RUNLAST );
    SetMenuItemText ( g_hToolsmenu, IDM_RUNLAST, 
        TEXT("Executing last command... click to abort") );
    ETLastEnableMenus ( FALSE );

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    ListBox_ResetContent ( g_hOutwList );

    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT(">>>> Process started at "), NULL );

    g_thandle = _beginthreadex ( NULL, 0, 
        Thread_ExecTool, &etd, 0, &g_tid );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnBUFEXECTOOL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_BUFEXECTOOL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnBUFEXECTOOL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int ic;
    
    ListBox_AddString ( g_hOutwList, (TCHAR *)wParam);
    ic = ListBox_GetCount ( g_hOutwList );
    
    if ( ic < 64 || ic % 64 == 0 )
    {
        OutwUpdateTitle ( g_hOutWnd, ic );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
    }
    
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnBUFCFDECL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_BUFEXECTOOL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnBUFCFDECL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int ic;
    
    ListBox_AddString ( g_hOutwList, (TCHAR *)wParam );
    ic = ListBox_GetCount ( g_hOutwList );
    
    if ( ic < 16 || ic % 8 == 0 )
    {
        OutwUpdateTitle ( g_hOutWnd, ic );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
    }
    
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnBUFCFDEFS 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_BUFCFDEFS
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnBUFCFDEFS ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int ic;
    
    ListBox_AddString ( g_hOutwList, (TCHAR *)wParam );
    ic = ListBox_GetCount ( g_hOutwList );
    
    if ( ic < 16 || ic % 8 == 0 )
    {
        OutwUpdateTitle ( g_hOutWnd, ic );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
    }
    
    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnBUFCFDECLSEL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_BUFCFDECLSEL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnBUFCFDECLSEL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int ic;
    
    ListBox_AddString ( g_hOutwList, (TCHAR *)wParam );
    ic = ListBox_GetCount ( g_hOutwList );
    
    if ( ic < 16 || ic % 8 == 0 )
    {
        OutwUpdateTitle ( g_hOutWnd, ic );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnBUFCFDEFSSEL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: WM_BUFCFDEFSSEL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnBUFCFDEFSSEL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int ic;
    
    ListBox_AddString ( g_hOutwList, (TCHAR *)wParam );
    ic = ListBox_GetCount ( g_hOutwList );

    if ( ic < 16 || ic % 8 == 0 )
    {
        OutwUpdateTitle ( g_hOutWnd, ic );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainWND_OnBUFFALL 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: hWnd   : 
//    Param.    2: wParam : 
//    Param.    3: lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 01.10.2020
//    DESCRIPTION: WM_BUFFALL
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainWND_OnBUFFALL ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    TCHAR   temp[1280];
    int     ic;

    #ifdef UNICODE
        StringCchPrintf ( temp, ARRAYSIZE(temp), 
            L"Line %lu: %ls", (DWORD)lParam, (WCHAR *)wParam );
    #else
        StringCchPrintf ( temp, ARRAYSIZE(temp), 
            "Line %lu: %s", (DWORD)lParam, (CHAR *)wParam );
    #endif

    ListBox_AddString ( g_hOutwList, temp );
    ic = ListBox_GetCount ( g_hOutwList );
    
    if ( ic < 64 || ic % 32 == 0 )
    {
        OutwUpdateTitle ( g_hOutWnd, ic );
        ListBox_SetTopIndex ( g_hOutwList, ic-1 );
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: OutwInsertTimeMarker 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hList   : listbox to write to
//    Param.    2: TCHAR * start: text to start line with
//    Param.    3: TCHAR * end  : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Inserts a line in a listbox; the line starts with s and 
//                 continues with dd.mm.yyyy, hh:mm:ss
/*--------------------------------------------------------------------@@-@@-*/
BOOL OutwInsertTimeMarker ( HWND hList, TCHAR * start, TCHAR * end )
/*--------------------------------------------------------------------------*/
{
    TCHAR   temp[256];

    if ( start == NULL )
        start = TEXT("");

    if ( end == NULL )
        end = TEXT("");

    #ifdef UNICODE
        StringCchPrintf ( temp, ARRAYSIZE(temp), 
        L"%ls%ls, %ls%ls", start, Today(), Now(), end );
    #else
        StringCchPrintf ( temp, ARRAYSIZE(temp), 
        "%s%s, %s%s", start, Today(), Now(), end );
    #endif

    return ListBox_AddString ( hList, temp );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Menu_RunUserCmd 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: INT_PTR menu_idx : crt. user menu index in user_menu array
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Execute the command associated with the 
//                 clicked user menu item
/*--------------------------------------------------------------------@@-@@-*/
BOOL Menu_RunUserCmd ( INT_PTR menu_idx )
/*--------------------------------------------------------------------------*/
{
    TCHAR   temp[TMAX_PATH];
    TCHAR   * filespec;

    if ( g_thread_working )
    {
        g_thread_working = FALSE;
        PostThreadMessage ( g_tid, WM_ABTEXECTOOL, 0, 0 );
        return FALSE;
    }

    RtlZeroMemory ( &etd, sizeof (EXEC_THREAD_DATA) );
    StringCchCopy ( etd.filename, ARRAYSIZE(etd.filename), 
        tab_data[g_curtab].fpath );
    StringCchCopy ( etd.cmd, ARRAYSIZE(etd.cmd), 
        user_menu[menu_idx].command );

    WritePrivateProfileString ( TEXT("editor"), 
        TEXT("last_cmd"), etd.cmd, g_szinifile );

    if ( PathFileExists ( user_menu[menu_idx].wcurdir ) )
        StringCchCopy ( etd.curdir, ARRAYSIZE(etd.curdir), 
            user_menu[menu_idx].wcurdir );
    else
    {
        filespec = (TCHAR *)FILE_Extract_path 
            ( tab_data[g_curtab].fpath, FALSE );

        if ( (_tcsstr ( user_menu[menu_idx].wcurdir, 
                TEXT("{p}")) != NULL) && (filespec != NULL) )
                    StringCchCopy ( etd.curdir, ARRAYSIZE(etd.curdir), 
                        filespec );
        else
        {
            GetModuleFileName ( NULL, temp, ARRAYSIZE(temp) );
            StringCchCopy ( etd.curdir, ARRAYSIZE(etd.curdir), 
                FILE_Extract_path ( temp, FALSE ) );
        }
    }

    WritePrivateProfileString ( TEXT("editor"), 
        TEXT("process working dir"), etd.curdir, g_szinifile );

    etd.capture         = user_menu[menu_idx].capture;
    etd.lParam          = (LPARAM)g_hMain;
    etd.wParam          = (WPARAM)g_hOutwList;
    etd.proc            = (PBPROC)ExecToolProc;
    etd.exectype        = EXEC_USERMNU;

    g_thread_working    = TRUE;
    g_szoldmenu         = GetMenuItemText ( g_hUsermenu, 
        (int)(IDM_USERSTART+menu_idx) );
    StringCchPrintf ( temp, ARRAYSIZE(temp), 
        TEXT("Executing user menu %zd command... click to abort"), menu_idx );
    SetMenuItemText ( g_hUsermenu, (int)(IDM_USERSTART+menu_idx), temp );
    ETUserEnableMenus ( FALSE );

    if ( g_thandle )
    {
        CloseHandle ( (HANDLE)g_thandle );
        g_thandle = 0;
    }

    ListBox_ResetContent ( g_hOutwList );
    OutwInsertTimeMarker ( g_hOutwList, 
        TEXT(">>>> Process started at "), NULL );

    g_thandle = _beginthreadex ( NULL, 0, 
        Thread_ExecTool, &etd, 0, &g_tid );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ETUserEnableMenus 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: BOOL state : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Enable / disable menu items when one userdef menus
//                 gets executed
/*--------------------------------------------------------------------@@-@@-*/
void ETUserEnableMenus ( BOOL state )
/*--------------------------------------------------------------------------*/
{
    HMENU       hsub;
    INT_PTR     i;
    DWORD       states[2] = { MF_GRAYED, MF_ENABLED };

    hsub = GetSubMenu ( g_hMainmenu, IDMM_FILE ); // file
    EnableMenuItem ( hsub, IDM_NEW, states[state] );
    EnableMenuItem ( hsub, IDM_OPEN, states[state] );
    EnableMenuItem ( hsub, IDM_SAVE, states[state] );
    EnableMenuItem ( hsub, IDM_SAVEAS, states[state] );
    EnableMenuItem ( hsub, IDM_NEWINST, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8, states[state] );
    EnableMenuItem ( hsub, IDM_UTF8_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF16_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_LE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_UTF32_BE_BOM, states[state] );
    EnableMenuItem ( hsub, IDM_ANSI, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_EDIT ); // edit
    EnableMenuItem ( hsub, IDM_UNDO, states[state] );
    EnableMenuItem ( hsub, IDM_REDO, states[state] );
    EnableMenuItem ( hsub, IDM_CUT, states[state] );
    EnableMenuItem ( hsub, IDM_COPY, states[state] );
    EnableMenuItem ( hsub, IDM_PASTE, states[state] );
    EnableMenuItem ( hsub, IDM_DEL, states[state] );
    EnableMenuItem ( hsub, IDM_SELALL, states[state] );
    EnableMenuItem ( hsub, IDM_FIND, states[state] );
    EnableMenuItem ( hsub, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL, states[state] );
    EnableMenuItem ( hsub, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( hsub, IDM_GOTO, states[state] );
    EnableMenuItem ( hsub, IDM_WRAP, states[state] );
    EnableMenuItem ( hsub, IDM_MARGIN, states[state] );
    EnableMenuItem ( hsub, IDM_VIEWHEX, states[state] );
    EnableMenuItem ( hsub, IDM_SAVESEL, states[state] );
    EnableMenuItem ( hsub, IDM_MERGE, states[state] );
    EnableMenuItem ( hsub, IDM_REFR, states[state] );
    EnableMenuItem ( hsub, IDM_FONT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_TOOLS ); // tools
    EnableMenuItem ( hsub, IDM_VEXPLORE, states[state] );
    EnableMenuItem ( hsub, IDM_RUN, states[state] );
    EnableMenuItem ( hsub, IDM_RUNLAST, states[state] );//IDM_RUNLAST
    EnableMenuItem ( hsub, IDM_WINFIND, states[state] );
    EnableMenuItem ( hsub, IDM_PRINT, states[state] );
    EnableMenuItem ( hsub, IDM_PROP, states[state] );
    EnableMenuItem ( hsub, IDM_CLEAR, states[state] );
    EnableMenuItem ( hsub, IDM_ECLIP, states[state] );
    EnableMenuItem ( hsub, IDM_COLOR, states[state] );
    EnableMenuItem ( hsub, IDM_FGCOLOR, states[state] );
    EnableMenuItem ( hsub, IDM_UPCASE, states[state] );
    EnableMenuItem ( hsub, IDM_LOWCASE, states[state] );
    EnableMenuItem ( hsub, IDM_RUNEXT, states[state] );
    
    EnableMenuItem ( hsub, IDM_CCOM, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_ALL, states[state] );
    EnableMenuItem ( hsub, IDM_CDEFS_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_CDECL_SEL, states[state] );
    EnableMenuItem ( hsub, IDM_TABS, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_VIEW ); // view
    EnableMenuItem ( hsub, IDM_ONTOP, states[state] );
    //EnableMenuItem ( hsub, IDM_VIEWOUT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_USER ); // view

    for ( i = 0; i < g_umenu_cnt; i++ )
        if ( i != g_umenu_idx )
            EnableMenuItem ( hsub, (UINT)(IDM_USERSTART+i), states[state] );
}
