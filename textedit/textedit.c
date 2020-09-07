
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

// textedit.c - main program module
// for UNICODE testing :-)
// http://www.columbia.edu/~fdc/utf8/index.html
#pragma warn(disable: 2008 2118 2228 2231 2030 2260) //enum not used in switch, = used in conditional

#include            "main.h"
#include            <strsafe.h>
#include            <windowsx.h>
#include            <shell32x.h>
#include            <shlwapi.h>
#include            <shlobj.h>
#include            <process.h>

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

INT_PTR CALLBACK    STR_StreamInProc        ( DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb );
INT_PTR CALLBACK    STR_StreamOutProc       ( DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb );
INT_PTR CALLBACK    DLG_GotoDlgProc         ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK    DLG_FindDlgProc         ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK    DLG_AboutDlgProc        ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK    DLG_TabsDlgProc         ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK    DLG_EXTToolDlgProc      ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK    DLG_OUTviewDlgProc      ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK    DLG_HEXViewDlgProc      ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL                DLG_OpenDlg             ( HWND hParent, TCHAR * filename, int size, BOOL whole, BOOL newtab );
BOOL                DLG_SaveDlg             ( HWND hParent, TCHAR * filename, int size, BOOL whole );
INT_PTR             DLG_OutputWnd           ( HWND hwnd );

LRESULT CALLBACK    WndProc                 ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK    OutwSubclassProc        ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK    RichSubclassProc        ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK    TabSubclassProc         ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK       CvtTabsProc             ( DWORD tabstops, DWORD donetabs, DWORD_PTR lParam );

void                UpdateMenuStatus        ( void );
BOOL                QuitSaveCheck           ( void );
void                UpdateTitlebar          ( HWND hwnd, const TCHAR * fname, const TCHAR * wtitle );
BOOL                MainWND_OnDropFiles     ( HWND hwnd, WPARAM wParam, LPARAM lParam );
void                UpdateCaretPos          ( HWND hrich, BOOL resize_panels );
BOOL                UpdateEncodingMenus     ( HMENU menu, int index );
BOOL                TD_UpdateTabDataEntry   ( TAB_DATA * pdata, DWORD flags, int index, int childid, int encoding, int cmdshow, HWND hchild, const TCHAR * path );
BOOL                TD_RebuildTabData       ( TAB_DATA * pdata, int ex_element, int elements );

BOOL                Prompt_to_save          ( void );
void                UpdateModifiedStatus    ( BOOL resize_panels );
void                UpdateEncodingStatus    ( BOOL resize_panels );
BOOL                RemoveTab               ( HWND htab );
const TCHAR         * ExpandMacro           ( const TCHAR * src, const TCHAR * filespec );

void __cdecl        ConvertTabsToSpaces     ( void * unused );

void                EnableMenus             ( BOOL state );
void                Menu_NewInstance        ( void );
void                Menu_SaveSelection      ( HWND hparent );
void                Menu_WWrap              ( HWND hwnd );
void                Menu_Open               ( HWND hwnd );
BOOL                Menu_WipeClipboard      ( HWND hwnd );
BOOL                Menu_MergeFile          ( HWND hwnd );
BOOL                Menu_RefreshFromDisk    ( HWND hwnd );
BOOL                Menu_Save               ( HWND hwnd );
BOOL                Menu_Saveas             ( HWND hwnd );
BOOL                Menu_New                ( HWND hwnd );
BOOL                Menu_ToggleMargin       ( HWND hwnd );
BOOL                Menu_Find               ( HWND hwnd );
BOOL                Menu_ConvertTabs        ( HWND hwnd );
BOOL                Menu_SetFont            ( HWND hwnd );
BOOL                Menu_Print              ( HWND hwnd );
BOOL                Menu_SetFGColor         ( HWND hwnd );
BOOL                Menu_SetBGColor         ( HWND hwnd );
BOOL                Menu_HexView            ( HWND hwnd );
BOOL                Menu_Quit               ( HWND hwnd );
BOOL                Menu_Ontop              ( HWND hwnd );
BOOL                Menu_Properties         ( HWND hwnd );

BOOL                MainWND_OnNotify        ( HWND hwnd, WPARAM wParam, LPARAM lParam );
BOOL                MainWND_OnCreate        ( HWND hwnd, WPARAM wParam, LPARAM lParam );
BOOL                MainWND_OnSize          ( HWND hwnd, WPARAM wParam, LPARAM lParam );
BOOL                MainWND_OnInitMenuPopup ( HWND hwnd, WPARAM wParam, LPARAM lParam );
BOOL                MainWND_OnCommand       ( HWND hwnd, WPARAM wParam, LPARAM lParam );
BOOL                MainWND_OnCopyData      ( HWND hwnd, WPARAM wParam, LPARAM lParam );
BOOL                OpenFileListFromCMDL    ( TCHAR ** filelist, int files );
BOOL                OpenFileListFromOFD     ( TCHAR * filelist, int fofset, BOOL newtab );
void                CycleTabs               ( HWND hwnd, int direction );
HIMAGELIST          InitImgList             ( void );

// you can judge a programmer by the # of globals :-))
HWND                g_hStatus;
HWND                g_hRich, g_hTab;
HWND                g_hMain;

HMENU               g_hMainmenu, g_hFilemenu, g_hEditmenu, g_hToolsmenu, g_hViewmenu;
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
BOOL                g_ball       = FALSE;
BOOL                g_bontop     = FALSE;
BOOL                g_bmenu_needs_update = TRUE;
BOOL                g_bnot_alone = FALSE;
volatile BOOL       g_converting = FALSE;

TCHAR               g_szbuf[TMAX_PATH];
TCHAR               g_sztextbuf[512];
TCHAR               g_szrepbuf[512];
TCHAR               g_szinifile[TMAX_PATH];

WNDPROC             g_rich_old_proc;
WNDPROC             g_outw_old_proc;
WNDPROC             g_tab_old_proc;

INT_PTR             g_dwtabsize  = 4;
void                * g_outw_buf;
int                 g_fontheight, g_ctr;
int                 g_curtab      = 0;
int                 g_fencoding   = E_UTF_8;
BOOL                g_bom_written = FALSE;

CRITICAL_SECTION    g_lock;

TAB_DATA            tab_data[MAX_TABS];



int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow )
/*****************************************************************************************************************/
/* Main program loop                                                                                             */
{
    HWND            hwnd;
    MSG             msg;
    WNDCLASSEX      wndclass;
    TCHAR           ** arglist;
    DWORD           x, y, w, h, sw, sh;
    int             argc;
    RECT            r;
    TCHAR           temp[128];
    HMODULE         hDetour = NULL;

    InitializeCriticalSection ( &g_lock );
    g_outw_buf             = alloc_and_zero_mem ( CAPTURE_BUF_SIZE );

    if ( !g_outw_buf )
    {
        StringCchPrintf ( temp, ARRAYSIZE(temp), TEXT("Could not allocate %lu of memory. Amazing!"), CAPTURE_BUF_SIZE );
        ShowMessage ( NULL, temp, MB_OK, IDI_LARGE );
        return 0;
    }

    *((TCHAR *)g_outw_buf) = TEXT('\0');
    g_hInst                = hInstance;
    hDetour                = LoadLibrary ( DETOUR_DLL );
    g_hDll                 = LoadLibrary ( RICH_DLL );

    if ( !g_hDll )
    {
        StringCchPrintf ( temp, ARRAYSIZE(temp), CCH_SPEC5, RICH_DLL ); // unable to find richedit/msft dll
        ShowMessage ( NULL, temp, MB_OK, IDI_LARGE );
        free_mem ( g_outw_buf );
        FreeLibrary ( hDetour );
        return 0;
    }

    InitCommonControls();
    g_bnot_alone = IsThereAnotherInstance ( app_classname );

    g_hIcon                = LoadIcon ( g_hInst, MAKEINTRESOURCE ( IDI_LARGE ) );
    g_hSmallIcon           = LoadIcon ( g_hInst, MAKEINTRESOURCE ( IDI_SMALL ) );
    g_hIml                 = InitImgList();

    wndclass.cbSize        = sizeof ( WNDCLASSEX );
    wndclass.hInstance     = hInstance;
    wndclass.style         = WIN_CLASS;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hIcon         = g_hIcon;
    wndclass.hIconSm       = g_hSmallIcon;
    wndclass.hCursor       = LoadCursor ( NULL, IDC_ARROW );
    wndclass.hbrBackground = ( HBRUSH ) GetStockObject ( NULL_BRUSH ) ;
    wndclass.lpszMenuName  = MAKEINTRESOURCE ( IDR_MAINMENU );
    // make only the first instance "magic" :-)
    wndclass.lpszClassName = g_bnot_alone ? app_classname2 : app_classname;

    if ( !RegisterClassEx ( &wndclass ) )
    {
        ShowMessage ( NULL, TEXT("Unable to register window class"), MB_OK, IDI_LARGE ) ;
        free_mem ( g_outw_buf );
        FreeLibrary ( hDetour );
        return 0 ;
    }

    FILE_MakeFullIniFname ( g_szinifile, ARRAYSIZE(g_szinifile) );
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

    g_bmargin              = ( BOOL ) GetPrivateProfileInt ( TEXT("editor"), TEXT("margin"), 1, g_szinifile );
    g_bwrap                = ( BOOL ) GetPrivateProfileInt ( TEXT("editor"), TEXT("wrap"), 0, g_szinifile );
    g_bontop               = ( BOOL ) GetPrivateProfileInt ( TEXT("window"), TEXT("ontop"), 0, g_szinifile );

    hwnd = CreateWindowEx ( WS_EX_LEFT|WS_EX_ACCEPTFILES,
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

    if ( !hwnd )
    {
        ShowMessage ( NULL, TEXT("Could not create main window"), MB_OK, IDI_LARGE );
        DeleteObject ( g_hFont );
        free_mem ( g_outw_buf );
        FreeLibrary ( hDetour );
        return 0;
    }

    if ( g_bontop )
        SetWindowPos ( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
    else
        SetWindowPos ( hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    if ( GetPrivateProfileInt ( TEXT("window"), TEXT("maximized"), 0, g_szinifile ) )
        ShowWindow ( hwnd, SW_MAXIMIZE );
    else
        ShowWindow ( hwnd, iCmdShow );

    UpdateWindow ( hwnd );
    g_hAccel                = LoadAccelerators ( g_hInst, MAKEINTRESOURCE ( IDR_ACCEL ) );
    g_hMain                 = hwnd;
    g_hMainmenu             = GetMenu ( hwnd );

    // save these for later, needed for menu status update
    g_hFilemenu             = GetSubMenu ( g_hMainmenu, IDMM_FILE );
    g_hEditmenu             = GetSubMenu ( g_hMainmenu, IDMM_EDIT );
    g_hToolsmenu            = GetSubMenu ( g_hMainmenu, IDMM_TOOLS );
    g_hViewmenu             = GetSubMenu ( g_hMainmenu, IDMM_VIEW );

    RtlZeroMemory ( g_sztextbuf, sizeof ( g_sztextbuf ) );
    RtlZeroMemory ( g_szrepbuf, sizeof ( g_szrepbuf ) );
    arglist = FILE_CommandLineToArgv ( GetCommandLine(), &argc );

    if ( argc >= 2 && arglist != NULL )
    {
        OpenFileListFromCMDL ( arglist, argc );
        RedrawWindow ( g_hTab, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
    }
    else
    {
        StringCchCopy ( g_szbuf, ARRAYSIZE(g_szbuf), TEXT("Untitled") );
        UpdateTitlebar ( g_hMain, g_szbuf, app_title );
    }

    while ( GetMessage ( &msg, NULL, 0, 0 ) )
    {
        if ( !TranslateAccelerator ( hwnd, g_hAccel, &msg ) )
        {
            TranslateMessage ( &msg ) ;
            DispatchMessage ( &msg ) ;
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
    free_mem ( g_outw_buf );

    if ( arglist != NULL )
        GlobalFree ( arglist );

    FreeLibrary ( g_hDll );
    FreeLibrary ( hDetour );

    DeleteCriticalSection ( &g_lock );
    return msg.wParam;
}

LRESULT CALLBACK TabSubclassProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
{
    switch ( message )
    {
        case WM_COMMAND:
            if ( LOWORD ( wParam ) == tab_data[g_curtab].child_id )
            {
                if ( HIWORD ( wParam ) == EN_CHANGE && !g_converting ) // if we're in some lengthy tab cvt op, pass on
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
            PostMessage ( hwnd, WM_LBUTTONDOWN, (WPARAM)MK_LBUTTON, lParam );
            PostMessage ( hwnd, WM_LBUTTONUP, (WPARAM)MK_LBUTTON | MK_SHIFT, lParam );
            break;

        case WM_LBUTTONUP:
            if ( wParam & MK_SHIFT )
                RemoveTab ( hwnd );
            break;
    }
    return CallWindowProc ( g_tab_old_proc, hwnd, message, wParam, lParam );
}

#define MKCARET_HEIGHT(x) ((x+(x>>2))-(!(x&1)))

LRESULT CALLBACK RichSubclassProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* Richedit custom window procedure (subclass). Mainly for custom caret and menu                                 */
{
    POINT       pt;
    HMENU       hSub;
    LRESULT     res = 0;

    switch ( message )
    {
        // a bunch of msg's on which we set our caret
        case WM_SETFOCUS:
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
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
            res = CallWindowProc ( g_rich_old_proc, hwnd, message, wParam, lParam );
            Rich_CreateShowCaret ( hwnd, CARET_WIDTH, MKCARET_HEIGHT(g_fontheight), g_binsmode );                // setting the custom caret (wider than default)
            break;

        case WM_KEYDOWN:
            res = CallWindowProc ( g_rich_old_proc, hwnd, message, wParam, lParam );

            if ( wParam == VK_INSERT )
                g_binsmode = !g_binsmode;

            Rich_CreateShowCaret ( hwnd, CARET_WIDTH, MKCARET_HEIGHT(g_fontheight), g_binsmode );
            break;
        // you can comment this case to get back the original richedit "smooth" scroll
        // (smooth my ass, lol), although I don't see why you'd want to do that...
        case WM_MOUSEWHEEL:
            if ( GET_WHEEL_DELTA_WPARAM(wParam) > 0 )
                SendMessage ( hwnd, EM_LINESCROLL, 0, -RICH_VERT_SCROLL );
            else
                SendMessage ( hwnd, EM_LINESCROLL, 0, RICH_VERT_SCROLL );
            res = TRUE;
            break;

        case WM_KILLFOCUS:
            res = CallWindowProc ( g_rich_old_proc, hwnd, message, wParam, lParam );
            DestroyCaret();
            break;

        case WM_RBUTTONUP:
            GetCursorPos ( &pt );
            hSub = GetSubMenu ( g_hMainmenu, 1 );
            UpdateMenuStatus();
            TrackPopupMenuEx ( hSub, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, pt.x, pt.y, g_hMain, NULL );
            res = CallWindowProc ( g_rich_old_proc, hwnd, message, wParam, lParam );
            Rich_CreateShowCaret ( hwnd, CARET_WIDTH, g_fontheight+4, g_binsmode );
            break;

        default:
            res = CallWindowProc ( g_rich_old_proc, hwnd, message, wParam, lParam );
            break;
    }

    return res;
}

LRESULT CALLBACK OutwSubclassProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* output wnd for the execute tool. Subclass o process dblclick and to make a popup menu                         */
{
    static HMENU    hcpmenu, hsub;
    POINT           pt;
    DWORD           states[2] = { MF_GRAYED, MF_ENABLED };
    BOOL            state;

    switch ( message )
    {
        case WM_CONTEXTMENU:
            hcpmenu = LoadMenu ( g_hInst, MAKEINTRESOURCE ( IDR_OUTWMENU ) );
            if ( !hcpmenu ) { break; }
            hsub = GetSubMenu ( hcpmenu, 0 );
            state = ( ListBox_GetCount ( hwnd ) != 0 );
            EnableMenuItem ( hsub, IDM_CPLINES, states[state] );
            GetCursorPos ( &pt );
            TrackPopupMenuEx ( hsub, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, pt.x, pt.y, hwnd, NULL );
            break;

        case WM_COMMAND:
            if ( LOWORD ( wParam ) == IDM_CPLINES )
                CopyListToClipboard ( g_hMain, hwnd );
            break;

        case WM_LBUTTONDBLCLK:
            CopyListToClipboard ( g_hMain, hwnd );
            break;

        case WM_DESTROY:
            if ( hcpmenu ) { DestroyMenu ( hcpmenu ); }
            break;

        default:
            break;
    }

    return CallWindowProc ( g_outw_old_proc, hwnd, message, wParam, lParam );
}

INT_PTR CALLBACK DLG_AboutDlgProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* about dlg                                                                                                     */
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDlg ( hDlg, g_hMain );
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
            break; // why do you put a break after ret? for the future when you'll make it a case =)
    }
    return 1;
}

INT_PTR CALLBACK DLG_TabsDlgProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* convert tabs dlg box proc                                                                                     */
{
    BOOL    converted;
    UINT    result;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDlg ( hDlg, g_hMain );
            SendMessage ( GetDlgItem ( hDlg, IDC_TABSTOPS ), EM_LIMITTEXT, 1, 0 );
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
                    result = GetDlgItemInt ( hDlg, IDC_TABSTOPS, &converted, FALSE );
                    if ( ( result > 0 ) && ( result <= 8 ) && ( converted == TRUE ) )
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

INT_PTR CALLBACK DLG_FindDlgProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* find dlg box proc                                                                                             */
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
            CheckDlgButton ( hDlg, IDC_SFWD, states[g_bfwd] );
            LoadCBBHistoryFromINI ( hDlg, IDC_FINDSTR, TEXT("find history"), g_szinifile, FALSE );
            LoadCBBHistoryFromINI ( hDlg, IDC_REPSTR, TEXT("replace history"), g_szinifile, FALSE );

            if ( (TCHAR *)lParam != NULL )
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
                            if ( SaveCBBHistoryToINI ( hDlg, IDC_FINDSTR, TEXT("find history"), g_szinifile ) )
                            {
                                GetDlgItemText ( hDlg, IDC_FINDSTR, g_sztextbuf, ARRAYSIZE ( g_sztextbuf ) );

                                g_bcase     = ( IsDlgButtonChecked ( hDlg, IDC_CASE )       == BST_CHECKED );
                                g_bwhole    = ( IsDlgButtonChecked ( hDlg, IDC_WHOLE )      == BST_CHECKED );
                                g_ball      = ( IsDlgButtonChecked ( hDlg, IDC_REPALL )     == BST_CHECKED );
                                g_bprompt   = ( IsDlgButtonChecked ( hDlg, IDC_REPPROMPT )  == BST_CHECKED );
                                g_bfwd      = ( IsDlgButtonChecked ( hDlg, IDC_SFWD )       == BST_CHECKED );

                                if ( SaveCBBHistoryToINI ( hDlg, IDC_REPSTR, TEXT("replace history"), g_szinifile ) )
                                {
                                    GetDlgItemText ( hDlg, IDC_REPSTR, g_szrepbuf, ARRAYSIZE ( g_szrepbuf ) );
                                    EndDialog ( hDlg, IDREPLACE );
                                    break;
                                }

                                EndDialog ( hDlg, IDFIND );
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

INT_PTR CALLBACK DLG_GotoDlgProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* goto line dlg box procedure                                                                                   */
{
    DWORD   line;
    BOOL    success;
    HWND    hRich;

    hRich = tab_data[g_curtab].hchild;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDlg ( hDlg, g_hMain );
            SetDlgItemInt ( hDlg, IDC_LINES, Rich_GetLineCount( hRich ), FALSE );
            SetDlgItemInt ( hDlg, IDC_CRTLINE, Rich_GetCrtLine( hRich )+1, FALSE );
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
                            line = GetDlgItemInt ( hDlg, IDC_DEST, &success, FALSE );
                            if ( success )
                            {
                                if ( line < Rich_GetLineCount( hRich )+1 )
                                {
                                    if ( line > 0 ) line--;
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

INT_PTR CALLBACK DLG_EXTToolDlgProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* exec external tool dlg proc                                                                                   */
{
    HWND    hCb, hEd;
    int     result;
    TCHAR   temp[TMAX_PATH];
    TCHAR   item_txt[TMAX_PATH];
    TCHAR   fcmd[TMAX_PATH];
    TCHAR   * cmd, * dir;
    BOOL    capture;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            CenterDlg ( hDlg, g_hMain );
            CheckDlgButton ( hDlg, IDC_CKREDIR, ( BOOL ) GetPrivateProfileInt ( TEXT("editor"), TEXT("capture"), 0, g_szinifile ) );
            GetModuleFileName ( NULL, temp, sizeof ( temp ) );
            GetPrivateProfileString ( TEXT("editor"), TEXT("process working dir"), FILE_Extract_path ( temp, FALSE ), item_txt, sizeof(item_txt), g_szinifile );
            SetDlgItemText ( hDlg, IDC_WKDIR, item_txt );
            LoadCBBHistoryFromINI ( hDlg, IDC_CB, TEXT("run history"), g_szinifile, TRUE );
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
                    hCb = GetDlgItem ( hDlg, IDC_CB );
                    if ( SaveCBBHistoryToINI ( hDlg, IDC_CB, TEXT("run history"), g_szinifile ) )
                    {
                        capture = ( IsDlgButtonChecked ( hDlg, IDC_CKREDIR ) == BST_CHECKED );
                        StringCchPrintf ( temp, ARRAYSIZE(temp), TEXT("%d"), (int)capture );
                        WritePrivateProfileString ( TEXT("editor"), TEXT("capture"), temp, g_szinifile );
                        hEd = GetDlgItem ( hDlg, IDC_WKDIR );
                        dir = NULL;
                        if ( GetWindowTextLength ( hEd ) != 0 )
                        {
                            GetDlgItemText ( hDlg, IDC_WKDIR, temp, ARRAYSIZE ( temp ) );
                            WritePrivateProfileString ( TEXT("editor"), TEXT("process working dir"), temp, g_szinifile );
                            dir = temp;
                        }
                        GetDlgItemText ( hDlg, IDC_CB, item_txt, ARRAYSIZE ( item_txt ) );
                        cmd = (TCHAR *)ExpandMacro ( item_txt, tab_data[g_curtab].fpath );
                        result = IDCANCEL;
                        if ( cmd )
                        {
                            __try
                            {
                                if ( capture )
                                {
                                    StringCchCopy ( fcmd, ARRAYSIZE(fcmd), TEXT("spawn.exe ") );
                                    StringCchCat ( fcmd, ARRAYSIZE(fcmd), cmd );
                                    if ( ExecAndCapture ( fcmd, dir, (TCHAR *)g_outw_buf, CAPTURE_BUF_SIZE/sizeof(TCHAR) ) )
                                        result = IDOK;
                                    else
                                        ShowMessage ( hDlg, TEXT("Error creating process."), MB_OK, IDI_LARGE );
                                }
                                else
                                {
                                    if ( !SimpleExec ( cmd, dir ) )
                                        ShowMessage ( hDlg, TEXT("Error creating process."), MB_OK, IDI_LARGE );
                                }
                            }
                            __except ( EXCEPTION_EXECUTE_HANDLER )
                            {
                                ShowMessage ( hDlg, TEXT("Error creating process."), MB_OK, IDI_LARGE );
                            }
                        }
                        else
                            ShowMessage ( hDlg, TEXT("Error parsing command line."), MB_OK, IDI_LARGE );
                        EndDialog ( hDlg, result );
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

INT_PTR CALLBACK DLG_OUTviewDlgProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* output window dlg proc                                                                                        */
{
    RECT        r;
    HWND        hlist;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            hlist = GetDlgItem ( hDlg, IDC_LIST );
            g_outw_old_proc = ( WNDPROC ) SetWindowLongPtr ( hlist, GWLP_WNDPROC, ( LONG_PTR ) OutwSubclassProc ); //subclass

            if ( LoadDlgPosition ( &r, g_szinifile, TEXT("outwindow") ) )
                MoveWindow ( hDlg, r.left, r.top, r.right-r.left, r.bottom-r.top, FALSE );

            CenterDlg ( hDlg, g_hMain );
            Parse_OUT_Buf ( (TCHAR *)g_outw_buf, (PBPROC)Parse_OUT_Buf_Proc, (LPARAM)hlist );

            if ( SendMessage ( hlist, LB_GETCOUNT, 0, 0 ) <= 0 )
                SendMessage ( hlist, LB_ADDSTRING, 0, (LPARAM)TEXT("<-- no output (yet :-)) -->") );

            SetFocus ( hlist );
            ListBox_SetCurSel ( hlist, 0 );
            break;

        case WM_SIZE:
            GetClientRect ( hDlg, &r );
            MoveWindow ( GetDlgItem ( hDlg, IDC_LIST ), r.left, r.top, r.right, r.bottom, TRUE );
            break;

        case WM_CLOSE:
            SaveDlgPosition ( hDlg, g_szinifile, TEXT("outwindow") );
            EndDialog ( hDlg, 0 );
            break;

        case WM_COMMAND:
            switch ( LOWORD ( wParam ) )
            {
                case IDCANCEL:
                    SendMessage ( hDlg, WM_CLOSE, 0, 0 );
                    break;
            }
            break;

        default:
            return 0;
            break;
    }
    return 1;
}

LRESULT CALLBACK WndProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* main window procedure                                                                                         */
{
    switch ( message )
    {
        case WM_COPYDATA:
            return MainWND_OnCopyData ( hwnd, wParam, lParam );
            break;

        case WM_DROPFILES:
            MainWND_OnDropFiles ( hwnd, wParam, lParam );
            break;

        case WM_MENUSELECT: // menu description on statusbar
            SB_MenuHelp ( g_hInst, g_hStatus, (DWORD)wParam );
            SendMessage ( g_hStatus, WM_PAINT, 0, 0 );
            break;

        case WM_CREATE:
            MainWND_OnCreate ( hwnd, wParam, lParam );
            break;

        case WM_SIZE:
            MainWND_OnSize ( hwnd, wParam, lParam );
            break;

        case WM_SETFOCUS:
            SetFocus ( tab_data[g_curtab].hchild );
            break;

        case WM_INITMENUPOPUP:
            MainWND_OnInitMenuPopup ( hwnd, wParam, lParam );
            break;

        case WM_NOTIFY:
            MainWND_OnNotify ( hwnd, wParam, lParam );
            break;

        case WM_COMMAND:
            MainWND_OnCommand ( hwnd, wParam, lParam );
            break;

        case WM_CLOSE:
            if ( QuitSaveCheck() )
            {
                SaveWindowPosition ( hwnd, g_szinifile );
                DragAcceptFiles ( hwnd, FALSE ); // no more Satan
                DestroyWindow ( hwnd );
            }
            break;

        case WM_DESTROY:
            PostQuitMessage ( 0 ) ;
            break;

        default:
            return DefWindowProc ( hwnd, message, wParam, lParam ) ;
            break;
    }

    return 0;
}

void UpdateMenuStatus ( void )
/*****************************************************************************************************************/
{
    BOOL        state;
    DWORD       states[4] = { MF_GRAYED, MF_ENABLED, MF_UNCHECKED, MF_CHECKED };
    HWND        hRich;

    // get out if we're converting tabs to spaces
    if ( g_converting ) { return; }

    hRich = tab_data[g_curtab].hchild;

    state = ( ( tab_data[g_curtab].fpath[0] != 0 ) && ( lstrcmp ( tab_data[g_curtab].fpath, TEXT("Untitled") ) != 0 ) );
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

    // if no more updating, exit early, since this may be executing on idle state
    if ( !g_bmenu_needs_update ) { return; }

    state = ( GetWindowTextLength ( hRich ) != 0 );
    EnableMenuItem ( g_hEditmenu, IDM_GOTO, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_FIND, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_FINDNEXT, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_SELALL, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_MERGE, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_FINDSEL, states[state] );
    EnableMenuItem ( g_hEditmenu, IDM_FINDSEL_BK, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_TABS, states[state] );
    EnableMenuItem ( g_hToolsmenu, IDM_PRINT, states[state] );

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

    g_bmenu_needs_update = FALSE;
}

BOOL DLG_OpenDlg ( HWND hParent, TCHAR * filename, int size, BOOL whole, BOOL newtab )
/*****************************************************************************************************************/
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
            result = Rich_SelStreamin ( hRich, NULL/*g_hStatus*/, (EDITSTREAMCALLBACK)STR_StreamInProc, encoding, filename );
            if ( result )
                TD_UpdateTabDataEntry ( tab_data, TD_FENCOD, g_curtab, 0, encoding, 0, 0, NULL );
        }

        SB_StatusSetSimple ( g_hStatus, FALSE );
        SB_StatusResizePanels ( g_hStatus );
    }

    return result;
}

BOOL DLG_SaveDlg ( HWND hParent, TCHAR * filename, int size, BOOL whole )
/*****************************************************************************************************************/
{
    OPENFILENAME    ofn;
    DWORD           dlgstyle;
    float           file_size;
    BOOL            result = TRUE;
    TCHAR           temp[128];
    HWND            hRich;

    RtlZeroMemory ( &ofn, sizeof ( ofn ) );

    dlgstyle = OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

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
            result = Rich_Streamout ( hRich, NULL/*g_hStatus*/, (EDITSTREAMCALLBACK)STR_StreamOutProc, tab_data[g_curtab].fencoding, filename );
            if ( result )
            {
                TD_UpdateTabDataEntry ( tab_data, TD_FNAME|TD_FPATH, g_curtab, 0, 0, 0, 0, filename );
                Tab_SetText ( g_hTab, g_curtab, tab_data[g_curtab].fname );
                Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
                UpdateTitlebar ( g_hMain, tab_data[g_curtab].fpath, app_title );
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
            result = Rich_SelStreamout ( hRich, NULL/*g_hStatus*/, (EDITSTREAMCALLBACK)STR_StreamOutProc, tab_data[g_curtab].fencoding, filename );

	    SB_StatusSetSimple ( g_hStatus, FALSE );
        SB_StatusResizePanels ( g_hStatus );
    }

    return result;
}

INT_PTR CALLBACK STR_StreamInProc ( DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb )
/*****************************************************************************************************************/
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

    data = alloc_and_zero_mem ( (cb * 4) + 32 ); // reserve for worst case :-)
    if ( data == NULL ) { return TRUE; }

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
                // tell MultiByteToWideChar to return the actual # of widechars in our buffer
                chars = MultiByteToWideChar ( CP_ACP, 0, data, csize, NULL, 0 );
                // got it, do the actual conversion
                MultiByteToWideChar ( CP_ACP, 0, data, csize, (WCHAR *)pbBuff, chars );
                chars <<= 1;    // bytes = chars * 2
                isize = *pcb;
                *pcb = chars;   // tell richedit streamin function the actual # of bytes processed
            }
            break;

        case E_UTF_8:
            // read only cb/2 bytes, since outbuf will be max. cb bytes
            isize = cb >> 1;
            ReadFile ( ( HANDLE )hfile, data, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 ) // have we actually read anything?
            {
                csize = *pcb;
                // tell MultiByteToWideChar to return the actual # of widechars in our buffer
                chars = MultiByteToWideChar ( CP_UTF8, 0, data, csize, NULL, 0 );
                // got it, do the actual conversion
                MultiByteToWideChar ( CP_UTF8, 0, data, csize, (WCHAR *)pbBuff, chars );
                chars <<= 1;    // bytes = chars * 2
                isize = *pcb;//
                *pcb = chars;   // tell richedit streamin function the actual # of bytes processed
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
                    chars = MultiByteToWideChar ( CP_UTF8, 0, data, csize, NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, data, csize, (WCHAR *)pbBuff, chars );
                }
                else
                {
                    csize -= 3; // skip 3 bytes of the UTF8 BOM and adjust count
                    chars = MultiByteToWideChar ( CP_UTF8, 0, ((char *)(data)+3), csize, NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, ((char *)(data)+3), csize, (WCHAR *)pbBuff, chars );
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
            ReadFile ( ( HANDLE )hfile, pbBuff, isize, (LPDWORD)pcb, NULL ); // just read in, since richedit is in unimode
            break;

        case E_UTF_16_BE:
            isize = cb;
            ReadFile ( ( HANDLE )hfile, data, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 )
                ENC_ChangeEndian ( (WCHAR *)pbBuff, (const WCHAR *)data, isize >> 1 ); // byte swap isize *WCHARS*
            break;

        case E_UTF_16_LE_BOM:
            isize = cb;
            ReadFile ( ( HANDLE )hfile, data, isize, (LPDWORD)pcb, NULL );

            if ( *pcb > 0 )
            {
                csize = *pcb;

                if ( bom_skipped )
                {
                    RtlCopyMemory ( pbBuff, data, csize ); // copy byte by byte, since it's LE
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
                    ENC_ChangeEndian ( (WCHAR *)pbBuff, (const WCHAR *)data, csize >> 1 );
                }
                else
                {
                    csize -= 1; // skip BOM (1 WCHAR) and adjust count
                    ENC_ChangeEndian ( (WCHAR *)pbBuff, ((const WCHAR *)(data) + 1 ), csize >> 1 );
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
                ilen = ENC_Utf32Utf8_LE ( (const char *)pbBuff, (int)csize, data, (int)csize );

                chars = MultiByteToWideChar ( CP_UTF8, 0, data, ilen, NULL, 0 );
                MultiByteToWideChar ( CP_UTF8, 0, data, ilen, (WCHAR *)pbBuff, chars );

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
                ilen = ENC_Utf32Utf8_BE ( (const char *)pbBuff, (int)csize, data, (int)csize );

                chars = MultiByteToWideChar ( CP_UTF8, 0, data, ilen, NULL, 0 );
                MultiByteToWideChar ( CP_UTF8, 0, data, ilen, (WCHAR *)pbBuff, chars );

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
                ilen = ENC_Utf32Utf8_BE ( (const char *)pbBuff, (int)csize, data, (int)csize );

                if ( bom_skipped )
                {
                    chars = MultiByteToWideChar ( CP_UTF8, 0, data, ilen, NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, data, ilen, (WCHAR *)pbBuff, chars );
                }
                else
                {
                    ilen -= 3;
                    chars = MultiByteToWideChar ( CP_UTF8, 0, ((char *)(data)+3), ilen, NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, ((char *)(data)+3), ilen, (WCHAR *)pbBuff, chars );
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
                ilen = ENC_Utf32Utf8_LE ( (const char *)pbBuff, (int)csize, data, (int)csize );

                if ( bom_skipped )
                {
                    chars = MultiByteToWideChar ( CP_UTF8, 0, data, ilen, NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, data, ilen, (WCHAR *)pbBuff, chars );
                }
                else
                {
                    ilen -= 3;
                    chars = MultiByteToWideChar ( CP_UTF8, 0, ((char *)(data)+3), ilen, NULL, 0 );
                    MultiByteToWideChar ( CP_UTF8, 0, ((char *)(data)+3), ilen, (WCHAR *)pbBuff, chars );
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

INT_PTR CALLBACK STR_StreamOutProc ( DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb )
/*****************************************************************************************************************/
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

    data = alloc_and_zero_mem ( (cb * 4) + 32 ); // reserve for worst case :-)
    if ( data == NULL ) { return TRUE; }

    switch ( psdata->fencoding )
    {
        case E_DONT_BOTHER:
        case E_ANSI:
            isize = cb >> 1;
            bytes = WideCharToMultiByte ( GetACP(), 0, (const WCHAR *)pbBuff, isize, NULL, 0, NULL, NULL );
            WideCharToMultiByte ( GetACP(), 0, (const WCHAR *)pbBuff, isize, data, bytes, NULL, NULL );
            WriteFile ( hfile, data, bytes, (LPDWORD)pcb, NULL );
            *pcb = cb;
            break;

        case E_UTF_8:
            isize = cb >> 1;
            bytes = WideCharToMultiByte ( CP_UTF8, 0, (const WCHAR *)pbBuff, isize, NULL, 0, NULL, NULL );
            WideCharToMultiByte ( CP_UTF8, 0, (const WCHAR *)pbBuff, isize, data, bytes, NULL, NULL );
            WriteFile ( hfile, data, bytes, (LPDWORD)pcb, NULL );
            *pcb = cb;
            break;

        case E_UTF_8_BOM:
            isize = cb >> 1;
            bytes = WideCharToMultiByte ( CP_UTF8, 0, (const WCHAR *)pbBuff, isize, NULL, 0, NULL, NULL );
            WideCharToMultiByte ( CP_UTF8, 0, (const WCHAR *)pbBuff, isize, data, bytes, NULL, NULL );

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
            WriteFile ( hfile, pbBuff, cb, (LPDWORD)pcb, NULL ); // just write out, since richedit is in unimode
            break;

        case E_UTF_16_BE:
            isize = cb >> 1;
            ENC_ChangeEndian ( (WCHAR *)data, (const WCHAR *)pbBuff, isize ); // byte swap isize *WCHARS*
            WriteFile ( hfile, data, cb, (LPDWORD)pcb, NULL );
            break;

        case E_UTF_16_LE_BOM:
            isize = cb >> 1;

            if ( !g_bom_written )
            {
                WriteFile ( hfile, UTF_16_LE_BOM, 2, &b, NULL ); // UTF_16_LE_BOM
                g_bom_written = TRUE;
            }
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

BOOL Prompt_to_save ( void )
/*****************************************************************************************************************/
{
    int     result;
    TCHAR   temp[512];

    if ( Rich_GetModified( tab_data[g_curtab].hchild ) )
    {
#ifdef UNICODE
        StringCchPrintf ( temp, ARRAYSIZE(temp), L"Do you wish to save \"%ls\" ?", tab_data[g_curtab].fpath );
#else
        StringCchPrintf ( temp, ARRAYSIZE(temp), "Do you wish to save \"%s\" ?", tab_data[g_curtab].fpath );
#endif
        result = ShowMessage ( g_hMain, temp, MB_YESNOCANCEL, IDI_LARGE );

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

void UpdateTitlebar ( HWND hwnd, const TCHAR * fname, const TCHAR * wtitle )
/*****************************************************************************************************************/
{
    TCHAR       temp[TMAX_PATH];
#ifdef UNICODE
    StringCchPrintf ( temp, ARRAYSIZE(temp), L"%ls - %ls", fname, wtitle );
#else
    StringCchPrintf ( temp, ARRAYSIZE(temp), "%s - %s", fname, wtitle );
#endif
    SetWindowText ( hwnd, temp );
}

BOOL Menu_WipeClipboard ( HWND hwnd )
/*****************************************************************************************************************/
{
    if ( OpenClipboard ( hwnd ) )
    {
        EmptyClipboard();
        CloseClipboard();
        return TRUE;
    }

    return FALSE;
}

void Menu_NewInstance ( void )
/*****************************************************************************************************************/
{
    TCHAR    temp[TMAX_PATH];

    if ( GetModuleFileName ( NULL, temp, sizeof ( temp ) ) )
        SimpleExec ( temp, FILE_Extract_path ( temp, FALSE ) );
}

BOOL Menu_MergeFile ( HWND hwnd )
/*****************************************************************************************************************/
{
    TCHAR        temp[TMAX_PATH], temp2[TMAX_PATH];
    int          old_encoding;
    BOOL         result;

    temp[0]      = TEXT('\0'); // for GetOpenFileName
    old_encoding = 0;
    result       = TRUE;

    // save old encoding, if any
    if ( tab_data[g_curtab].fencoding <= E_ANSI && tab_data[g_curtab].fencoding >= E_UTF_8 )
        old_encoding = tab_data[g_curtab].fencoding;

    if ( !DLG_OpenDlg ( hwnd, temp, ARRAYSIZE ( temp ), FALSE, FALSE ) )
    {
        StringCchPrintf ( temp2, ARRAYSIZE(temp2), CCH_SPEC2, temp );
        ShowMessage ( hwnd, temp2, MB_OK, IDI_LARGE );
        result = FALSE;
    }

    if ( old_encoding != tab_data[g_curtab].fencoding )
    {
        StringCchPrintf ( temp2, ARRAYSIZE(temp2),
                            CCH_SPEC4,
                            ENC_EncTypeToStr ( old_encoding ),
                            ENC_EncTypeToStr ( tab_data[g_curtab].fencoding ) );
        ShowMessage ( hwnd, temp2, MB_OK, IDI_LARGE );
    }
    // restore encoding
    if ( old_encoding > 0 )
        tab_data[g_curtab].fencoding = old_encoding;

    return result;
}

void Menu_SaveSelection ( HWND hparent )
/*****************************************************************************************************************/
{
    TCHAR    temp[TMAX_PATH], temp2[TMAX_PATH];

    temp[0] = 0;
    if ( !DLG_SaveDlg ( hparent, temp, ARRAYSIZE ( temp ), FALSE ) )
    {
#ifdef UNICODE
        StringCchPrintf ( temp2, ARRAYSIZE(temp2), L"Error saving selection to %ls", temp );
#else
        StringCchPrintf ( temp2, ARRAYSIZE(temp2), "Error saving selection to %s", temp );
#endif
        ShowMessage ( hparent, temp2, MB_OK, IDI_LARGE );
    }
}

BOOL CALLBACK CvtTabsProc ( DWORD tabstops, DWORD donetabs, DWORD_PTR lParam )
/*****************************************************************************************************************/
{
    TCHAR   temp[128];

    StringCchPrintf ( temp, ARRAYSIZE(temp), TEXT("Converting tabs to %lu spaces, hold on to your butt... %lu tabs converted"), tabstops, donetabs );
    SB_StatusSetText ( (HWND)lParam, 3|SBT_NOBORDERS, temp );

    EnterCriticalSection ( &g_lock );
    if ( g_converting == FALSE )
    {
        LeaveCriticalSection ( &g_lock );
        return FALSE;
    }
    LeaveCriticalSection ( &g_lock );

    return TRUE;
}

const TCHAR * ExpandMacro ( const TCHAR * src, const TCHAR * filespec )
/*****************************************************************************************************************/
{
    static  TCHAR final_buf[1024];
    TCHAR   macro;
    const TCHAR * fbits;
    DWORD   src_idx, len, dst_idx;

    if ( ( src == NULL ) || ( filespec == NULL ) ) return NULL;

    len = lstrlen ( src );
    if ( len < 3 ) return src;

    src_idx = 0;
    dst_idx = 0;
    RtlZeroMemory ( final_buf, sizeof(final_buf) );
    while ( src[src_idx] )
    {
        if ( src[src_idx] != TEXT('{') )
            final_buf[dst_idx] = src[src_idx];
        else
        {
            if ( ( len-src_idx > 2 ) && ( src[src_idx+2] == TEXT('}') ) )
            {
                macro = src[src_idx+1];
                switch ( macro )
                {
                    case TEXT('f'):
                        StringCchCat ( final_buf, ARRAYSIZE(final_buf), filespec );
                        break;
                    case TEXT('F'):
                        fbits = FILE_Extract_full_fname ( filespec );
                        if ( fbits != NULL ) { StringCchCat ( final_buf, ARRAYSIZE(final_buf), fbits ); }
                        break;
                    case TEXT('n'):
                        fbits = FILE_Extract_filename ( filespec );
                        if ( fbits != NULL ) { StringCchCat ( final_buf, ARRAYSIZE(final_buf), fbits ); }
                        break;
                    case TEXT('e'):
                        fbits = FILE_Extract_ext ( filespec );
                        if ( fbits != NULL ) { StringCchCat ( final_buf, ARRAYSIZE(final_buf), fbits ); }
                        break;
                    case TEXT('p'):
                        fbits = FILE_Extract_path ( filespec, TRUE );
                        if ( fbits != NULL ) { StringCchCat ( final_buf, ARRAYSIZE(final_buf), fbits ); }
                        break;
                    default:
                        final_buf[dst_idx] = TEXT('{');
                        src_idx++;
                        dst_idx++;
                        continue;
                }
                dst_idx = lstrlen ( final_buf )-1;
                src_idx += 2;
            }
            else
            {
                final_buf[dst_idx] = TEXT('{');
                src_idx++;
                dst_idx++;
                continue;
            }
        }
        src_idx++;
        dst_idx++;
    }
    return final_buf;
}

INT_PTR DLG_OutputWnd ( HWND hwnd )
/*****************************************************************************************************************/
{
    return DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_OUT ), hwnd, DLG_OUTviewDlgProc, ( LPARAM ) hwnd );
}

BOOL Menu_HexView ( HWND hwnd )
/*****************************************************************************************************************/
{
    void        * buf;
    CHARRANGE   chr;
    DWORD       size;
    HEX_DATA    hd;
    HWND        hRich;

    hRich = tab_data[g_curtab].hchild;

    SendMessage ( hRich, EM_EXGETSEL, 0, ( LPARAM )&chr );
    if ( chr.cpMax == -1 ) //select all
        size = GetWindowTextLength ( hRich );
    else
        size = chr.cpMax - chr.cpMin;

    buf = alloc_and_zero_mem ( (size * sizeof(TCHAR))+32 );
    if ( buf == NULL ) { return FALSE; }

    SendMessage ( hRich, EM_GETSELTEXT, 0, ( LPARAM )( TCHAR * )buf );
    hd.hg = buf;
    hd.dwlength = size;
    DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_HEX ), hwnd, DLG_HEXViewDlgProc, ( LPARAM )&hd );
    return free_mem ( buf );
}

INT_PTR CALLBACK DLG_HEXViewDlgProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* hex view dlg proc                                                                                             */
{
    RECT        r;
    HWND        hlist;
    TCHAR       * data;
    DWORD       data_size;

    switch ( uMsg )
    {
        case WM_INITDIALOG:

            if ( LoadDlgPosition ( &r, g_szinifile, TEXT("hexviewer") ) )
                MoveWindow ( hDlg, r.left, r.top, r.right-r.left, r.bottom-r.top, FALSE );

            CenterDlg ( hDlg, g_hMain );
            hlist = GetDlgItem ( hDlg, IDC_HEXLIST );
            BeginDraw ( hlist );
            __try
            {
                data = (TCHAR *)(((HEX_DATA *)lParam)->hg);
                data_size = ((HEX_DATA *)lParam)->dwlength;
                HexDump ( data, data_size, (HEXDUMPPROC)HexDumpProc, (LPARAM)hlist );
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                ShowMessage ( hDlg, TEXT("Invalid data received :("), MB_OK, IDI_LARGE );
            }
            EndDraw ( hlist );
            SetFocus ( hlist );
            ListBox_SetCurSel ( hlist, 0 );
            break;

        case WM_SIZE:
            GetClientRect ( hDlg, &r );
            MoveWindow ( GetDlgItem ( hDlg, IDC_HEXLIST ), r.left, r.top, r.right, r.bottom, TRUE );
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

BOOL Menu_RefreshFromDisk ( HWND hwnd )
/*****************************************************************************************************************/
/* reload the file from disk                                                                                     */
{
    DWORD   line;
    float   file_size;
    TCHAR   temp[TMAX_PATH];
    HWND    hRich;

    hRich = tab_data[g_curtab].hchild;

    line = Rich_GetCrtLine( hRich );
    SB_StatusSetSimple ( g_hStatus, TRUE );
    SB_StatusSetText ( g_hStatus, 255|SBT_NOBORDERS, TEXT("opening...") );

    if ( Rich_Streamin ( hRich, NULL /* g_hStatus*/, (EDITSTREAMCALLBACK)STR_StreamInProc, tab_data[g_curtab].fencoding, tab_data[g_curtab].fpath ) )
        Rich_GotoLine ( hRich, line );
    else
    {
        StringCchPrintf ( temp, ARRAYSIZE(temp), CCH_SPEC2, tab_data[g_curtab].fpath );
        ShowMessage ( hwnd, temp, MB_OK, IDI_LARGE );
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

void __cdecl ConvertTabsToSpaces ( void * unused )
/*****************************************************************************************************************/
/* function for tab to space cvt. in a separate thread                                                           */
{
    HMENU               hTools;
    TCHAR               * szoldmenu;
    HWND                hRich;

    hRich = tab_data[g_curtab].hchild;

    EnableWindow ( g_hTab, FALSE );
    EnableMenus ( FALSE );
    BeginDraw ( hRich );
    hTools = GetSubMenu ( g_hMainmenu, 2 );
    szoldmenu = GetMenuItemText ( hTools, IDM_TABS );
    SetMenuItemText ( hTools, IDM_TABS, TEXT("C&onverting... click to abort") );
    Rich_ConvertTabsToSpaces ( hRich, (DWORD)g_dwtabsize, CvtTabsProc, (DWORD_PTR)g_hStatus );

    EnterCriticalSection ( &g_lock );
        g_converting = FALSE;
    LeaveCriticalSection ( &g_lock );

    SB_StatusSetText ( g_hStatus, 3|SBT_NOBORDERS, TEXT("Done!") );
    EnableWindow ( g_hTab, TRUE );

    UpdateModifiedStatus ( TRUE );

    SetMenuItemText ( hTools, IDM_TABS, szoldmenu );
    EndDraw ( hRich );
    EnableMenus ( TRUE );
}

void EnableMenus ( BOOL state )
/*****************************************************************************************************************/
/* enable/disable menus (for lengthy ops)                                                                        */
{
    HMENU       hsub;
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
    EnableMenuItem ( hsub, IDM_VIEWOUT, states[state] );

    hsub = GetSubMenu ( g_hMainmenu, IDMM_VIEW ); // file
    EnableMenuItem ( hsub, IDM_ONTOP, states[state] );
}

BOOL Menu_Save ( HWND hwnd )
/*****************************************************************************************************************/
/* save file in the selected tab                                                                                 */
{
    TCHAR           temp[TMAX_PATH], temp2[TMAX_PATH];
    HWND            hRich;
    float           file_size;

    hRich = tab_data[g_curtab].hchild;
    StringCchCopy ( temp, ARRAYSIZE(temp), TEXT("Untitled") );

    if ( lstrcmp ( tab_data[g_curtab].fpath, TEXT("Untitled") ) == 0 )
    {
        if ( !DLG_SaveDlg ( hwnd, temp, ARRAYSIZE ( temp ), TRUE  ) )
        {
            StringCchPrintf ( temp2, ARRAYSIZE(temp2), CCH_SPEC3, temp );
            ShowMessage ( hwnd, temp2, MB_OK, IDI_LARGE );
            return FALSE;
        }
    }
    else
    {
        SB_StatusSetSimple ( g_hStatus, TRUE );
        SB_StatusSetText ( g_hStatus, 255|SBT_NOBORDERS, TEXT("saving...") );
        g_bom_written = FALSE;

        if ( !Rich_Streamout ( hRich, NULL/*g_hStatus*/, (EDITSTREAMCALLBACK)STR_StreamOutProc, tab_data[g_curtab].fencoding, tab_data[g_curtab].fpath ) )
        {
            StringCchPrintf ( temp2, ARRAYSIZE(temp2), CCH_SPEC3, tab_data[g_curtab].fpath );
            ShowMessage ( hwnd, temp2, MB_OK, IDI_LARGE );
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

BOOL Menu_Saveas ( HWND hwnd )
/*****************************************************************************************************************/
/* save file in the selected tab as                                                                              */
{
    TCHAR   temp [TMAX_PATH];
    TCHAR   temp2[TMAX_PATH];

    StringCchCopy ( temp, ARRAYSIZE(temp), tab_data[g_curtab].fpath );

    if ( !DLG_SaveDlg ( hwnd, temp, ARRAYSIZE ( temp ), TRUE ) )
    {
        StringCchPrintf ( temp2, ARRAYSIZE(temp2), CCH_SPEC3, temp );
        ShowMessage ( hwnd, temp2, MB_OK, IDI_LARGE );
        return FALSE;
    }
    return TRUE;
}

BOOL Menu_New ( HWND hwnd )
/*****************************************************************************************************************/
/* make a new tab                                                                                                */
{
    int          tidx, tcount, lastsel;
    HWND         hRich;
    RECT         wr;

    tcount = Tab_GetCount ( g_hTab );
    if ( tcount >= MAX_TABS )
    {
        ShowMessage ( hwnd, TEXT("Maximum tab page limit reached"), MB_OK, IDI_LARGE );
        return FALSE;
    }

    BeginDraw ( g_hMain );
    tidx = Tab_NewTabPage ( g_hTab, tcount, TEXT("Untitled") );
    if ( tidx == -1 )
    {
        ShowMessage ( hwnd, TEXT("Unable to create tab page :("), MB_OK, IDI_LARGE );
        return FALSE;
    }

    hRich = Rich_NewRichedit ( g_hInst, &g_rich_old_proc, RichSubclassProc, hwnd, IDC_RICH + tidx, &g_hFont, g_bwrap, g_bmargin, g_szinifile );

    if ( hRich == NULL )
    {
        ShowMessage ( hwnd, TEXT("Unable to create tab page :("), MB_OK, IDI_LARGE );
        return FALSE;
    }
    g_curtab = tidx;
    TD_UpdateTabDataEntry ( tab_data, TD_ALL, g_curtab, IDC_RICH + tidx, E_UTF_8, SW_SHOW, hRich, NULL );
    lastsel = Tab_SetCurSel ( g_hTab, tidx );
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
    RedrawWindow ( g_hMain, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
    return TRUE;
}

BOOL Menu_ToggleMargin ( HWND hwnd )
/*****************************************************************************************************************/
/* toggle selection margin; such hassle for most useless feature :-))                                            */
{
    BOOL            mod;
    RECT            wr, tr, rstat;
    int             i, tcount;
    int             img_state[2] = {TIDX_SAV, TIDX_MOD};

    BeginDraw ( g_hMain );
    g_bmargin = !g_bmargin;
    tcount = Tab_GetCount ( g_hTab );
    SendMessage ( g_hStatus, WM_SIZE, 0, 0 );
    GetClientRect ( g_hStatus, &rstat );
    GetClientRect ( g_hMain, &wr );
    MoveWindow ( g_hTab, wr.left, wr.top, wr.right-wr.left, wr.bottom - rstat.bottom, FALSE );
    GetClientRect ( g_hTab, &tr );
    SendMessage ( g_hTab, TCM_ADJUSTRECT, 0, (LPARAM)&tr );

    if ( tcount > 1 )
    {
        for ( i = 0; i < tcount; i++ )
        {
            mod = Rich_GetModified( tab_data[i].hchild );
            Rich_ToggleSelBar ( g_hInst, &g_rich_old_proc, RichSubclassProc, &(tab_data[i].hchild), hwnd, tab_data[i].child_id, &g_hFont, g_bwrap, g_bmargin, g_szinifile );
            ShowWindow ( tab_data[i].hchild, tab_data[i].nCmdShow );
            MoveWindow ( tab_data[i].hchild, tr.left, tr.top, tr.right-tr.left, tr.bottom-tr.top, TRUE );
            Rich_SetModified ( tab_data[i].hchild, mod );
            Tab_SetImg ( g_hTab, i, img_state[mod] );
        }
    }
     else
    {
        mod = Rich_GetModified( tab_data[g_curtab].hchild );
        Rich_ToggleSelBar ( g_hInst, &g_rich_old_proc, RichSubclassProc, &(tab_data[g_curtab].hchild), hwnd, tab_data[g_curtab].child_id, &g_hFont, g_bwrap, g_bmargin, g_szinifile );
        ShowWindow ( tab_data[g_curtab].hchild, tab_data[g_curtab].nCmdShow );
        MoveWindow ( tab_data[g_curtab].hchild, tr.left, tr.top, tr.right-tr.left, tr.bottom-tr.top, TRUE );
        Rich_SetModified ( tab_data[g_curtab].hchild, mod );
        Tab_SetImg ( g_hTab, g_curtab, img_state[mod] );
    }

    if ( g_bmargin )
        WritePrivateProfileString ( TEXT("editor"), TEXT("margin"), TEXT("1"), g_szinifile );
    else
        WritePrivateProfileString ( TEXT("editor"), TEXT("margin"), TEXT("0"), g_szinifile );

    mod = Rich_GetModified( tab_data[g_curtab].hchild );
    ShowWindow ( g_hTab, SW_SHOWNORMAL );
    SetFocus ( tab_data[g_curtab].hchild );
    UpdateCaretPos ( tab_data[g_curtab].hchild, TRUE );

    if ( mod )
        UpdateModifiedStatus(TRUE);

    EndDraw ( g_hMain );
    RedrawWindow ( g_hMain, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
    return TRUE;
}

BOOL Menu_Find ( HWND hwnd )
/*****************************************************************************************************************/
/* find in the active tab                                                                                        */
{
    INT_PTR     result;
    HWND        hRich;
    TCHAR       * seltext = NULL;

    hRich = tab_data[g_curtab].hchild;
    seltext = Rich_GetSelText ( hRich );

    // fall silently if overzealous with the selection :-)
    if ( seltext != NULL )
        if ( lstrlen ( seltext ) > 250 )
            seltext = NULL;

    result = DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_FIND ), hwnd, DLG_FindDlgProc, ( LPARAM ) seltext );

    switch ( result )
    {
        case IDFIND:
            Rich_FindReplaceText ( hRich, hwnd, g_sztextbuf, NULL, g_bcase, g_bwhole, FALSE, g_bfwd );
            break;

        case IDREPLACE:
            if ( g_ball )
            {
                if ( !g_bprompt )
                {
                    BeginDraw ( hRich );
                    ShowCursor ( FALSE );
                }
                while ( Rich_FindReplaceText ( hRich, hwnd, g_sztextbuf, g_szrepbuf, g_bcase, g_bwhole, g_bprompt, g_bfwd ) );
                if ( !g_bprompt )
                {
                    EndDraw ( hRich );
                    ShowCursor ( TRUE );
                }
            }
            else
                Rich_FindReplaceText ( hRich, hwnd, g_sztextbuf, g_szrepbuf, g_bcase, g_bwhole, g_bprompt, g_bfwd );
            break;
    }
    return TRUE;
}

BOOL Menu_ConvertTabs ( HWND hwnd )
/*****************************************************************************************************************/
/* convert tabs to spaces; starts a new thread since it can take a while                                         */
{
    INT_PTR      result;

    EnterCriticalSection ( &g_lock );
    if ( g_converting )
    {
        g_converting = FALSE;
        LeaveCriticalSection ( &g_lock );
        return FALSE;
    }
    LeaveCriticalSection ( &g_lock );
    result = DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_TABS ), hwnd, DLG_TabsDlgProc, ( LPARAM ) g_dwtabsize );
    if ( result > 0 )
    {
        g_dwtabsize = result;
        EnterCriticalSection ( &g_lock );
        g_converting = TRUE;
        LeaveCriticalSection ( &g_lock );
        _beginthread ( ConvertTabsToSpaces, 0, NULL );
    }
    return TRUE;
}

BOOL Menu_SetFont ( HWND hwnd )
/*****************************************************************************************************************/
/* Set font to all tabs                                                                                          */
{
    BOOL       mod;
    HWND       hRich;
    int        tcount, i;
    int        img_state[2] = {TIDX_SAV, TIDX_MOD};

    tcount = Tab_GetCount ( g_hTab );

    if ( tcount > 1 ) // do we have more than one tab open?
    {
        // aye, set font for all richedits, but only save settings once
        mod = Rich_GetModified( tab_data[0].hchild );
        Rich_SetFont ( tab_data[0].hchild, hwnd, &g_hFont, g_szinifile );
        Rich_SetModified ( tab_data[0].hchild, mod );
        Tab_SetImg ( g_hTab, 0, img_state[mod] );
        for ( i = 1; i < tcount; i++ )
        {
            mod = Rich_GetModified( tab_data[i].hchild );
            Rich_SetFontNoCF ( tab_data[i].hchild, g_hFont );
            Rich_SetFGColor ( tab_data[i].hchild, hwnd, CC_INIRESTORE, g_szinifile );
            Rich_SetModified ( tab_data[i].hchild, mod );
            Tab_SetImg ( g_hTab, i, img_state[mod] );
        }
    }
    else // no, get the only one rich
    {
        hRich = tab_data[g_curtab].hchild;
        mod = Rich_GetModified ( hRich );
        Rich_SetFont ( hRich, hwnd, &g_hFont, g_szinifile );
        Rich_SetModified ( hRich, mod );
        Tab_SetImg ( g_hTab, g_curtab, img_state[mod] );
    }
    SB_StatusSetText ( g_hStatus, 1, TEXT("") );
    SB_StatusSetText ( g_hStatus, 2, TEXT("") );
    SB_StatusResizePanels ( g_hStatus );
    g_fontheight = Rich_GetFontHeight ( tab_data[g_curtab].hchild );
    return TRUE;
}

BOOL Menu_Print ( HWND hwnd )
/*****************************************************************************************************************/
/* try to print :-)                                                                                              */
{
    BOOL       mod;
    HWND       hRich;
    int        img_state[2] = {TIDX_SAV, TIDX_MOD};

    hRich = tab_data[g_curtab].hchild;

    // temporarily set richedit with black text on white bg :-)
    mod = Rich_GetModified( hRich );
    BeginDraw ( hRich );
    Rich_SetBGColor ( hRich, hwnd, RGB ( 255, 255, 255 ), NULL );
    Rich_SetFGColor ( hRich, hwnd, RGB ( 0, 0, 0 ), NULL );
    __try
    {
        Rich_Print ( hRich, hwnd, FILE_Extract_filename ( tab_data[g_curtab].fpath ) );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ShowMessage ( hwnd, TEXT("Error printing"), MB_OK, IDI_LARGE );
    }
    Rich_SetBGColor ( hRich, hwnd, CC_INIRESTORE, g_szinifile );
    Rich_SetFGColor ( hRich, hwnd, CC_INIRESTORE, g_szinifile );
    Rich_SetModified ( hRich, mod );
    Tab_SetImg ( g_hTab, g_curtab, img_state[mod] );
    EndDraw ( hRich );
    SB_StatusSetText ( g_hStatus, 1, TEXT("") );
    SB_StatusSetText ( g_hStatus, 2, TEXT("") );
    SB_StatusResizePanels ( g_hStatus );

    return TRUE;
}

BOOL Menu_SetFGColor ( HWND hwnd )
/*****************************************************************************************************************/
/* set fgcolor in all tabs                                                                                       */
{
    BOOL       mod;
    int        tcount, i;
    HWND       hRich;
    int        img_state[2] = {TIDX_SAV, TIDX_MOD};

    tcount = Tab_GetCount ( g_hTab );

    if ( tcount > 1 ) // do we have more than one tab open?
    {
        mod = Rich_GetModified( tab_data[0].hchild );
        // pass invalid color to force pickcolordlg
        Rich_SetFGColor ( tab_data[0].hchild, hwnd, CC_USERDEFINED, g_szinifile );
        Rich_SetModified ( tab_data[0].hchild, mod );
        Tab_SetImg ( g_hTab, 0, img_state[mod] );
        for ( i = 1; i < tcount; i++ )
        {
            mod = Rich_GetModified( tab_data[i].hchild );
            // set the already saved values for all other tabs
            Rich_SetFGColor ( tab_data[i].hchild, hwnd, CC_INIRESTORE, g_szinifile );
            Rich_SetModified ( tab_data[i].hchild, mod );
            Tab_SetImg ( g_hTab, i, img_state[mod] );
        }
    }
    else
    {
        hRich = tab_data[g_curtab].hchild;
        mod = Rich_GetModified( hRich );
        BeginDraw ( hRich );
        Rich_SetFGColor ( hRich, hwnd, CC_USERDEFINED, g_szinifile ); // pass invalid color to force pickcolordlg
        Rich_SetModified ( hRich, mod );
        Tab_SetImg ( g_hTab, g_curtab, img_state[mod] );
        EndDraw ( hRich );
    }
    SB_StatusSetText ( g_hStatus, 1, TEXT("") );
    SB_StatusSetText ( g_hStatus, 2, TEXT("") );
    SB_StatusResizePanels ( g_hStatus );
    return TRUE;
}

BOOL Menu_SetBGColor ( HWND hwnd )
/*****************************************************************************************************************/
/* set bgcolor in all tabs                                                                                       */
{
    int        tcount, i;
    BOOL       result;

    tcount = Tab_GetCount ( g_hTab );

    if ( tcount > 1 ) // do we have more than one tab open?
    {
        // pass invalid color to force pickcolordlg
        result = Rich_SetBGColor ( tab_data[0].hchild, hwnd, CC_USERDEFINED, g_szinifile );
        // set the already saved values for all other tabs
        for ( i = 1; i < tcount; i++ )
            Rich_SetBGColor ( tab_data[i].hchild, hwnd, CC_INIRESTORE, g_szinifile );
    }
    else
    {
        // pass invalid color to force pickcolordlg
        result = Rich_SetBGColor ( tab_data[g_curtab].hchild, hwnd, CC_USERDEFINED, g_szinifile );
    }
    return result;
}

BOOL UpdateEncodingMenus ( HMENU menu, int encoding )
/*****************************************************************************************************************/
/* check one member from the menu group coresponding to file encoding                                            */
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
    if ( menu == NULL  ) { return FALSE; }
    return CheckMenuRadioItem ( menu, 6, 19, menu_idx[enc-E_UTF_8], MF_BYPOSITION ); // 6 = UTF8, 19 = ANSI
}

BOOL TD_UpdateTabDataEntry ( TAB_DATA * pdata, DWORD flags, int index, int childid, int encoding,
                             int cmdshow, HWND hchild, const TCHAR * path )
/*****************************************************************************************************************/
/* update a TAB_DATA entry in an TAB_DATA array according with the requested flags                               */
{
    const TCHAR   * temp = NULL;

    if ( index > MAX_TABS-1 ) { return FALSE; }
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
            StringCchCopy ( pdata[index].fpath, ARRAYSIZE(pdata[index].fpath ), path );
            StringCchCopy ( pdata[index].fname, ARRAYSIZE(pdata[index].fname ), ( temp == NULL ) ? path : temp );
        }
        else
        {
            StringCchCopy ( pdata[index].fpath, ARRAYSIZE(pdata[index].fpath), TEXT("Untitled") );
            StringCchCopy ( pdata[index].fname, ARRAYSIZE(pdata[index].fname), TEXT("Untitled") );
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
                StringCchCopy ( pdata[index].fpath, ARRAYSIZE(pdata[index].fpath), path );
            else
                StringCchCopy ( pdata[index].fpath, ARRAYSIZE(pdata[index].fpath), TEXT("Untitled") );
        }
        if ( flags & TD_FNAME )
        {
            if ( path != NULL )
                StringCchCopy ( pdata[index].fname, ARRAYSIZE(pdata[index].fname), ( temp == NULL ) ? path : temp );
            else
                StringCchCopy ( pdata[index].fname, ARRAYSIZE(pdata[index].fname), TEXT("Untitled") );
        }
        return TRUE;
    }
}

BOOL TD_RebuildTabData ( TAB_DATA * pdata, int ex_element, int elements )
/*****************************************************************************************************************/
/* rebuild a TAB_DATA array after the ex_element is removed (closed tab with ex_element index)                   */
{
    TAB_DATA    * ptemp;
    int         i_src, i_dest;
    size_t      data_size;
    BOOL        result;

    if ( (pdata == NULL) || (ex_element >= elements) ) { return FALSE; }

    result      = TRUE;
    data_size   = sizeof ( TAB_DATA ) * elements;
    ptemp       = alloc_and_zero_mem ( data_size );                 // alloc enough mem to hold the actual data

    if ( ptemp == NULL ) { return FALSE; }
    __try
    {
        RtlCopyMemory ( ptemp, pdata, data_size );                  // copy all entries
        RtlZeroMemory ( pdata, data_size );                         // wipe destination

        for ( i_src = 0, i_dest = 0; i_src < elements; i_src++ )    // write back, minus the ex_element
        {
            if ( i_src == ex_element ) continue;
            RtlCopyMemory ( &pdata[i_dest], &ptemp[i_src], sizeof ( TAB_DATA ) );
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

void UpdateCaretPos ( HWND hrich, BOOL resize_panels )
/*****************************************************************************************************************/
/* show caret position in the statusbar                                                                          */
{
    CARET_POS      cp;
    TCHAR          temp[64];

    Rich_GetCaretPos( hrich, &cp );
    StringCchPrintf ( temp, ARRAYSIZE(temp), TEXT(" ln %8lu, col %4lu"), cp.c_line+1, cp.c_col+1 );
    SB_StatusSetText ( g_hStatus, 0, temp );
    if ( resize_panels )
        SB_StatusResizePanels ( g_hStatus );
}

void Menu_WWrap ( HWND hwnd )
/*****************************************************************************************************************/
/* change wordwrap on the active tab                                                                             */
{
    int        tcount, i;
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
        WritePrivateProfileString ( TEXT("editor"), TEXT("wrap"), TEXT("1"), g_szinifile );
    else
        WritePrivateProfileString ( TEXT("editor"), TEXT("wrap"), TEXT("0"), g_szinifile );

    EndDraw ( g_hMain );
    RedrawWindow ( g_hMain, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
}

void UpdateModifiedStatus ( BOOL resize_panels )
/*****************************************************************************************************************/
/* show file modified status in the statusbar                                                                    */
{
    TCHAR   temp[TMAX_PATH];

    if ( Rich_GetModified ( tab_data[g_curtab].hchild ) )
    {
        SB_StatusSetText ( g_hStatus, 1, TEXT("Modified") );
        StringCchPrintf ( temp, ARRAYSIZE(temp), TEXT("Current size %lu chars"), GetWindowTextLength ( tab_data[g_curtab].hchild ) );
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

void UpdateEncodingStatus ( BOOL resize_panels )
/*****************************************************************************************************************/
/* show file encoding info in the statusbar                                                                      */
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

void Menu_Open ( HWND hwnd )
/*****************************************************************************************************************/
/* open a file in new/same tab                                                                                   */
{
    TCHAR       * temp;
    TCHAR       * temp2;
    int         tcount;
    BOOL        newtab = FALSE;

    temp        = (TCHAR *)alloc_and_zero_mem ( OFN_BUF_SIZE );
    temp2       = (TCHAR *)alloc_and_zero_mem ( OFN_BUF_SIZE );

    if ( temp == NULL || temp2 == NULL )
    {
        ShowMessage ( hwnd, TEXT("Error allocating buffers for Open File dialog :-/"), MB_OK, IDI_LARGE );
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
        if ( Rich_GetModified ( tab_data[g_curtab].hchild ) || ( lstrcmp ( tab_data[g_curtab].fpath, TEXT("Untitled") ) != 0 ) )
            newtab = TRUE;
    }

    DLG_OpenDlg ( hwnd, temp, OFN_BUF_SIZE / sizeof (TCHAR), TRUE, newtab );

    free_mem ( temp );
    free_mem ( temp2 );
}

BOOL Menu_Quit ( HWND hwnd )
/*****************************************************************************************************************/
{
    SendMessage ( hwnd, WM_CLOSE, 0, 0 );
    return TRUE;
}

BOOL QuitSaveCheck ( void )
/*****************************************************************************************************************/
/* check if we need saving on WM_CLOSE                                                                           */
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
    g_converting = FALSE; // stop any tab cvt in progress
    return result;
}

BOOL RemoveTab ( HWND htab )
/*****************************************************************************************************************/
/* remove a tab                                                                                                  */
{
    int    tab_idx;

    if ( ( Tab_GetCount ( htab ) == 1 ) || !Prompt_to_save() ) { return FALSE; }

    tab_idx = g_curtab;
    BeginDraw ( g_hMain ); // here we go again

    if ( !Tab_DeletePage ( htab, tab_idx ) )
    {
        EndDraw ( g_hMain );
        RedrawWindow ( g_hMain, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
        return FALSE;
    }

    DestroyWindow ( tab_data[tab_idx].hchild );
    TD_RebuildTabData ( tab_data, tab_idx, ARRAYSIZE(tab_data) );
    g_curtab = Tab_GetCount ( htab ) - 1;
    Tab_SetCurSel ( htab, g_curtab );
    ShowWindow ( tab_data[g_curtab].hchild, SW_SHOW );
    tab_data[g_curtab].nCmdShow = SW_SHOW;
    SetFocus ( tab_data[g_curtab].hchild );
    UpdateCaretPos ( tab_data[g_curtab].hchild, FALSE );
    UpdateModifiedStatus ( FALSE );
    UpdateEncodingStatus ( TRUE );
    UpdateTitlebar ( g_hMain, tab_data[g_curtab].fpath, app_title );
    EndDraw ( g_hMain );
    RedrawWindow ( g_hMain, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
    // hello flicker my ol' frieeennn'
    // you've come to kick my ass againnnn
    return TRUE;
}

BOOL MainWND_OnDropFiles ( HWND hwnd, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* process WM_DROPFILES events for the main window                                                               */
{
    int         file_count, i, tab_count,
                free_slots, encoding;
    TCHAR       temp[128];
    HWND        hRich;
    BOOL        result;

    tab_count = Tab_GetCount ( g_hTab );
    if ( tab_count == MAX_TABS ) { return FALSE; } // silently go away if we're full on tabs

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

        if ( encoding == 0 ) continue; // give the remaining others a chance :-)

        g_fencoding = encoding;

        if ( !Menu_New ( g_hTab ) ) { return FALSE; }

        hRich = tab_data[g_curtab].hchild;
        result = Rich_Streamin ( hRich, NULL/*g_hStatus*/, (EDITSTREAMCALLBACK)STR_StreamInProc, encoding, temp );

        if ( result )
        {
            Rich_SetModified ( hRich, FALSE );
            TD_UpdateTabDataEntry ( tab_data, TD_FNAME|TD_FPATH|TD_FENCOD, g_curtab, 0, encoding, 0, 0, temp );
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

BOOL MainWND_OnNotify ( HWND hwnd, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* process WM_NOTIFY events for the main window                                                                  */
{
    int    tab_idx;

    if ( g_converting ) { return FALSE; }

    switch (((LPNMHDR)lParam)->code)
    {
        case TCN_SELCHANGING: // about to change tab...
            tab_idx = Tab_GetCurSel ( g_hTab );
            BeginDraw ( hwnd ); // remove the god damn flicker >:-/ part 1.. feeze main window when about to change tabs
            if (tab_idx != -1)  // I may be incompetent, but resizing windows with children flicker free is waay too much
            {
                ShowWindow ( tab_data[tab_idx].hchild, SW_HIDE );
                tab_data[tab_idx].nCmdShow = SW_HIDE;
            }
            break;

        case TCN_SELCHANGE: // just changed tab
            tab_idx = Tab_GetCurSel ( g_hTab );
            if (tab_idx != -1)
            {
                g_curtab = tab_idx;
                ShowWindow ( tab_data[tab_idx].hchild, SW_SHOW );
                tab_data[tab_idx].nCmdShow = SW_SHOW;
                SetFocus ( tab_data[tab_idx].hchild );
                UpdateTitlebar ( g_hMain, tab_data[g_curtab].fpath, app_title );
                UpdateCaretPos ( tab_data[tab_idx].hchild, FALSE );
                UpdateModifiedStatus ( FALSE );
                UpdateEncodingStatus ( TRUE );
            }
            EndDraw ( hwnd ); // remove the god damn flicker, part 2... unfreeze the main window
            RedrawWindow ( g_hMain, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN ); // ...and this is needed to force an update
            break;

        case EN_SELCHANGE:
            UpdateCaretPos ( tab_data[g_curtab].hchild, TRUE );
            g_bmenu_needs_update = TRUE;
            break;
    }
    return TRUE;
}

BOOL MainWND_OnCreate ( HWND hwnd, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* process WM_CREATE events for the main window                                                                  */
{
    g_hStatus      = SB_MakeStatusBar ( hwnd, IDC_STATUS );
    g_hTab         = Tab_NewTab ( hwnd );
    Tab_SetImageList ( g_hTab, g_hIml );
    g_tab_old_proc = ( WNDPROC ) SetWindowLongPtr ( g_hTab, GWLP_WNDPROC, ( LONG_PTR ) TabSubclassProc );
    g_curtab       = Tab_NewTabPage ( g_hTab, 0, TEXT("Untitled") );

    if ( g_curtab == -1 )
    {
        ShowMessage ( hwnd, TEXT("Unable to create tab page :("), MB_OK, 0 );
        SendMessage ( hwnd, WM_CLOSE, 0, 0 );
        return FALSE;
    }

    // it would seem appropriate to have a "old wnd proc" array to hold the original wndproc for each richedit ctrl.
    // so, wrote it that way originally. But it turns out that, for richedit, it's always the same procedure so we keep
    // just an old proc for all 
    g_hRich = Rich_NewRichedit ( g_hInst, &g_rich_old_proc, RichSubclassProc, g_hTab, IDC_RICH, &g_hFont, g_bwrap, g_bmargin, g_szinifile );
    UpdateCaretPos ( g_hRich, TRUE );
    g_fontheight = Rich_GetFontHeight ( g_hRich );
    TD_UpdateTabDataEntry ( tab_data, TD_ALL, g_curtab, IDC_RICH, g_fencoding, SW_SHOW, g_hRich, NULL );
    Tab_SetPadding ( g_hTab, 10, 5 );
    Tab_SetText ( g_hTab, g_curtab, tab_data[g_curtab].fname ); // yes, this is actually needed .. otherwise Tab_SetPadding has no effect until next SetText :-))
    Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
    DragAcceptFiles ( hwnd, TRUE ); // signal we accept Satan :-)
    return TRUE;
}

BOOL MainWND_OnSize ( HWND hwnd, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* process WM_SIZE events for the main window                                                                    */
{
    RECT   rstat, tr;
    int    i;

    SendMessage ( g_hStatus, WM_SIZE, 0, 0 );
    GetClientRect ( g_hStatus, &rstat );
    MoveWindow ( g_hTab, 0, 0, LOWORD ( lParam ), HIWORD ( lParam ) - rstat.bottom, FALSE );
    GetClientRect ( g_hTab, &tr );
    SendMessage ( g_hTab, TCM_ADJUSTRECT, 0, (LPARAM)&tr );

    for ( i = 0; i < Tab_GetCount ( g_hTab ); i++ )
        MoveWindow ( tab_data[i].hchild, tr.left, tr.top, tr.right-tr.left, tr.bottom-tr.top, TRUE );

    return RedrawWindow ( g_hTab, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN ); // redraw the tab control... lord almighty
}

BOOL MainWND_OnInitMenuPopup ( HWND hwnd, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* process WM_ONINITMENUPOPUP events for the main window                                                         */
{
    switch ( LOWORD ( lParam ) )
    {
        case IDMM_FILE:
            // the handle from wParam is unreliable,
            // since LOWORD ( lParam ) is 0 for both the file menu and the popup edit menu
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

BOOL MainWND_OnCommand ( HWND hwnd, WPARAM wParam, LPARAM lParam )
/*****************************************************************************************************************/
/* process WM_COMMAND events for the main window                                                                 */
{
    CHARRANGE       chr;
    TCHAR           temp[TMAX_PATH];
    TCHAR           temp2[TMAX_PATH];
    TCHAR           * pbrowser;

    if ( !lParam )
    {
        switch ( LOWORD ( wParam ) )
        {
            case IDM_NEW:
                Menu_New ( g_hTab );
                break;

            case IDM_NEWINST:
                Menu_NewInstance();
                break;

            case IDM_OPEN:
                Menu_Open ( hwnd );
                break;

            case IDM_MERGE:
                Menu_MergeFile ( hwnd );
                break;

            case IDM_SAVESEL:
                g_fencoding = ( ( g_fencoding > E_ANSI ) && ( g_fencoding < E_UTF_8 ) ) ? E_UTF_8 : g_fencoding;
                Menu_SaveSelection ( hwnd );
                break;

            case IDM_SAVE:
                Menu_Save ( hwnd );
                break;

            case IDM_SAVEAS:
                Menu_Saveas ( hwnd );
                break;

            case IDM_QUIT:
                Menu_Quit ( hwnd );
                break;

            case IDM_CUT:
                SendMessage ( tab_data[g_curtab].hchild, WM_CUT, 0, 0 );
                break;

            case IDM_COPY:
                SendMessage ( tab_data[g_curtab].hchild, WM_COPY, 0, 0 );
                break;

            case IDM_PASTE:
                SendMessage ( tab_data[g_curtab].hchild, EM_PASTESPECIAL, CF_UNICODETEXT, 0 );
                break;

            case IDM_DEL:
                SendMessage ( tab_data[g_curtab].hchild, EM_REPLACESEL, TRUE, 0 );
                break;

            case IDM_UNDO:
                SendMessage ( tab_data[g_curtab].hchild, EM_UNDO, 0, 0 );
                break;

            case IDM_REDO:
                SendMessage ( tab_data[g_curtab].hchild, EM_REDO, 0, 0 );
                break;

            case IDM_SELALL:
                chr.cpMin = 0;
                chr.cpMax = -1;
                SendMessage ( tab_data[g_curtab].hchild, EM_EXSETSEL, 0, (LPARAM)&chr );
                break;

            case IDM_MARGIN:
                Menu_ToggleMargin ( g_hTab );
                break;

            case IDM_WRAP:
                Menu_WWrap ( hwnd );
                break;

            case IDM_ONTOP:
                Menu_Ontop ( hwnd );
                break;

            case IDM_GOTO:
                DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_GOTO ), hwnd, DLG_GotoDlgProc, ( LPARAM ) hwnd );
                break;

            case IDM_FIND:
                Menu_Find ( hwnd );
                break;

            case IDM_TABS:
                Menu_ConvertTabs ( hwnd );
                break;

            case IDM_FINDNEXT:
                if ( g_sztextbuf[0] != TEXT('\0') )
                    Rich_FindReplaceText ( tab_data[g_curtab].hchild, hwnd, g_sztextbuf, NULL, g_bcase, g_bwhole, FALSE, g_bfwd );
                break;

            case IDM_FINDSEL:
                Rich_FindNext ( tab_data[g_curtab].hchild, hwnd, TRUE ); // find fwd
                break;

            case IDM_FINDSEL_BK:
                Rich_FindNext ( tab_data[g_curtab].hchild, hwnd, FALSE ); // find bkwd
                break;

            case IDM_ABOUT:
                DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_ABOUT ), hwnd, DLG_AboutDlgProc, ( LPARAM ) hwnd );
                break;

            case IDM_FONT:
                Menu_SetFont ( hwnd );
                break;

            case IDM_CLEAR:
                SHAddToRecentDocs ( SHARD_PATH, NULL );
                break;

            case IDM_ECLIP:
                Menu_WipeClipboard ( hwnd );
                break;

            case IDM_RUN:
                RunFileDlg ( hwnd, g_hIcon, NULL, NULL, NULL, 0 );
                break;

            case IDM_WINFIND:
                SHFindFiles ( NULL, NULL );
                break;

            case IDM_PRINT:
                Menu_Print ( hwnd );
                break;

            case IDM_PROP:
                Menu_Properties ( hwnd );
                break;

            case IDM_VEXPLORE:
                pbrowser = GetDefaultBrowser();
                if ( pbrowser != NULL )
                {
#ifdef UNICODE
                    StringCchPrintf ( temp2, ARRAYSIZE(temp2), L"%ls ", pbrowser );
#else
                    StringCchPrintf ( temp2, ARRAYSIZE(temp2), "%s ", pbrowser );
#endif
                    StringCchCopy ( temp, ARRAYSIZE(temp), tab_data[g_curtab].fpath );
                    PathQuoteSpaces ( temp );
                    StringCchCat ( temp2, ARRAYSIZE(temp2), temp );
                    SimpleExec ( temp2, NULL );
                }
                break;

            case IDM_COLOR:
                Menu_SetBGColor ( hwnd );
                break;

            case IDM_FGCOLOR:
                Menu_SetFGColor ( hwnd );
                break;

            case IDM_UPCASE:
                Rich_ConvertCase ( tab_data[g_curtab].hchild, FALSE );
                break;

            case IDM_LOWCASE:
                Rich_ConvertCase ( tab_data[g_curtab].hchild, TRUE );
                break;

            case IDM_RUNEXT:
                if ( DialogBoxParam ( g_hInst, MAKEINTRESOURCE ( IDD_EXT ), hwnd, DLG_EXTToolDlgProc, ( LPARAM ) hwnd ) != IDCANCEL )
                    DLG_OutputWnd ( hwnd );
                break;

            case IDM_VIEWOUT:
                DLG_OutputWnd ( hwnd );
                break;

            case IDM_VIEWHEX:
                Menu_HexView ( hwnd );
                break;

            case IDM_REFR:
                Menu_RefreshFromDisk ( hwnd );
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
                CycleTabs ( hwnd, CT_UP );
                break;

            case IDM_TAB_DOWN:
                CycleTabs ( hwnd, CT_DOWN );
                break;
        }
    }
    return TRUE;
}

BOOL OpenFileListFromCMDL ( TCHAR ** filelist, int files )
/*****************************************************************************************************************/
/* Open a file list in tabs                                                                                      */
{
    int         i, encoding, free_slots;
    BOOL        result = FALSE;
    HWND        hRich = NULL;
    BOOL        newtab = FALSE;

    if ( filelist == NULL ) { return FALSE; }
    free_slots  = MAX_TABS - Tab_GetCount ( g_hTab );
    if ( free_slots == 0 ) { return FALSE; };
    if ( free_slots > files ) { free_slots = files; };

    SB_StatusSetSimple ( g_hStatus, TRUE );
    SB_StatusSetText ( g_hStatus, 255|SBT_NOBORDERS, TEXT("opening...") );

    newtab = ( Rich_GetModified ( tab_data[g_curtab].hchild ) ) || ( lstrcmp ( tab_data[g_curtab].fpath, TEXT("Untitled") ) != 0 );

    for ( i = 1; i <= free_slots; i++ ) // filelist[0] is the program.exe
    {
        encoding = ENC_FileCheckEncoding ( filelist[i] );

        if ( encoding == 0 ) // skip if some garbage (or future command param :-) )
            continue;

        // occupy the first tab if "untitled" and not modified
        // but create tabs afterwards
        if ( (i > 1) && (newtab == FALSE) )
            newtab = TRUE;

        //if ( i > 1 ) // don't make new tabs unless we've used the first one
        if ( newtab )
            if ( !Menu_New ( g_hTab ) )
                break;

        hRich = tab_data[g_curtab].hchild;
        result = Rich_Streamin ( hRich, NULL/* g_hStatus*/, (EDITSTREAMCALLBACK)STR_StreamInProc, encoding, filelist[i] );

        if ( result )
        {
            Rich_SetModified ( hRich, FALSE );
            TD_UpdateTabDataEntry ( tab_data, TD_FNAME|TD_FPATH|TD_FENCOD, g_curtab, 0, encoding, 0, 0, filelist[i] );
            Tab_SetText ( g_hTab, g_curtab, tab_data[g_curtab].fname );
            Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
        }
    }
    UpdateTitlebar ( g_hMain, tab_data[g_curtab].fpath, app_title );
    SB_StatusSetSimple ( g_hStatus, FALSE );
    UpdateModifiedStatus ( FALSE );
    UpdateEncodingStatus ( FALSE );
    UpdateCaretPos ( hRich, TRUE );
    return result;
}

BOOL OpenFileListFromOFD ( TCHAR * filelist, int fofset, BOOL newtab )
/*****************************************************************************************************************/
/* Open a file list in tabs                                                                                      */
/* filelist is ofn.lpstrFile, as returned by GetOpenFulename, and has the following structure:                   */
/* first string (NULL terminated) is the crt. dir; next ones are the selected filenames                          */
/* (NULL terminated as well); the end is marked by a double NULL                                                 */
/* fofset is ofn.nFileOffset and represents how many tchars (zero based) do we count from the start of filelist  */
/* to the beginning of the first filename                                                                        */
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
        result = Rich_Streamin ( hRich, NULL/*g_hStatus*/, (EDITSTREAMCALLBACK)STR_StreamInProc, encoding, fname );

        if ( result )
        {
            Rich_SetModified ( hRich, FALSE );
            TD_UpdateTabDataEntry ( tab_data, TD_FNAME|TD_FPATH|TD_FENCOD, g_curtab, 0, encoding, 0, 0, fname );
            Tab_SetText ( g_hTab, g_curtab, tab_data[g_curtab].fname );
            Tab_SetImg ( g_hTab, g_curtab, TIDX_SAV );
        }
        else
        {
            StringCchPrintf ( temp, ARRAYSIZE(temp), CCH_SPEC2, fname );
            ShowMessage ( g_hMain, temp, MB_OK, IDI_LARGE );
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

void CycleTabs ( HWND hwnd, int direction )
/*****************************************************************************************************************/
/* cycle tabs back and forth, on Ctrl+Tab or Shift+Tab key combs.                                                */
{
    int tab_count, cur_tab, prev_tab;

    tab_count = Tab_GetCount ( g_hTab );
    if ( tab_count == 1 ) return;
    cur_tab = Tab_GetCurSel ( g_hTab );
    if ( cur_tab == -1 ) return;
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
    prev_tab = Tab_SetCurSel ( g_hTab, cur_tab );
    g_curtab = cur_tab;
    ShowWindow ( tab_data[prev_tab].hchild, SW_HIDE );
    tab_data[prev_tab].nCmdShow = SW_HIDE;
    ShowWindow ( tab_data[cur_tab].hchild, SW_SHOW );
    tab_data[cur_tab].nCmdShow = SW_SHOW;
    SetFocus ( tab_data[cur_tab].hchild );
    UpdateTitlebar ( g_hMain, tab_data[cur_tab].fpath, app_title );
    UpdateCaretPos ( tab_data[cur_tab].hchild, FALSE );
    UpdateModifiedStatus ( FALSE );
    UpdateEncodingStatus ( TRUE );
}

HIMAGELIST InitImgList ( void )
/*******************************************************************************************************************/
/* Create a imagelist, 16x16, color                                                                                */
{
    HIMAGELIST      img;
    HINSTANCE       hinst;

    hinst = GetModuleHandle ( NULL );
    img = ImageList_Create ( 16, 16, ILC_COLOR32/*ILC_COLOR | ILC_MASK*/, 0, 8 );

    if ( img == NULL ) { return NULL; }

    if ( ImageList_AddIcon ( img, LoadIcon ( hinst, MAKEINTRESOURCE ( IDI_SAV ) ) ) == -1 ) { return NULL; }
    if ( ImageList_AddIcon ( img, LoadIcon ( hinst, MAKEINTRESOURCE ( IDI_MOD ) ) ) == -1 )  { return NULL; }

    return img;
}

BOOL Menu_Ontop ( HWND hwnd )
/*******************************************************************************************************************/
/* Set main window to be "always on top", or not :-)                                                               */
{
    LONG_PTR    ex_style;

    ex_style = GetWindowLongPtr ( hwnd, GWL_EXSTYLE );

    if ( ex_style )
    {
        g_bontop = !g_bontop;

        if ( g_bontop )
        {
            ex_style |= WS_EX_TOPMOST;
            SetWindowLongPtr ( hwnd, GWL_EXSTYLE, ex_style );
            SetWindowPos ( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
            WritePrivateProfileString ( TEXT("window"), TEXT("ontop"), TEXT("1"), g_szinifile );
        }
        else
        {
            ex_style &= ~WS_EX_TOPMOST;
            SetWindowLongPtr ( hwnd, GWL_EXSTYLE, ex_style );
            SetWindowPos ( hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
            WritePrivateProfileString ( TEXT("window"), TEXT("ontop"), TEXT("0"), g_szinifile );
        }
        return TRUE;
    }
    return FALSE;
}

BOOL Menu_Properties ( HWND hwnd )
/*******************************************************************************************************************/
/* display the "Properties" dlg. box for the current file                                                          */
{
#ifndef UNICODE // SHObjectProperties wants only unicode
    WCHAR   temp[MAX_PATH];
    int     wchars;

    wchars = MultiByteToWideChar ( CP_UTF8, 0, tab_data[g_curtab].fpath, -1, NULL, 0 );
    MultiByteToWideChar ( CP_UTF8, 0, tab_data[g_curtab].fpath, -1, (LPWSTR)temp, (wchars > MAX_PATH) ? MAX_PATH : wchars );
    return SHObjectProperties ( hwnd, SHOP_FILEPATH, (LPWSTR)temp, NULL );
#else
    return SHObjectProperties ( hwnd, SHOP_FILEPATH, tab_data[g_curtab].fpath, NULL );
#endif
}

BOOL MainWND_OnCopyData ( HWND hwnd, WPARAM wParam, LPARAM lParam )
/*******************************************************************************************************************/
/* handle WM_COPYDATA message                                                                                      */
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

    // check for our secret preshared key with the shell ctx menu extension :-))
    if ( pcd->dwData != COPYDATA_MAGIC )
        return result;

    __try
    {
        // use the same function from cmdline processing, since the received data is the same
        arglist = FILE_CommandLineToArgv ( (TCHAR *)(pcd->lpData), &argc );

        if ( argc >= 2 && arglist != NULL )
        {
            result = OpenFileListFromCMDL ( arglist, argc );
            RedrawWindow ( g_hTab, NULL, NULL, RDW_ERASE|RDW_FRAME|RDW_INVALIDATE|RDW_ALLCHILDREN );
            // bring program window to foreground by momentarily making it topmost
            // ...but only if it isn't already 8-)
            if ( !g_bontop )
            {
                if ( IsIconic ( hwnd ) )
                    ShowWindow ( hwnd, SW_RESTORE );
                else
                {
                    ShowWindow ( hwnd, SW_MINIMIZE );
                    ShowWindow ( hwnd, SW_RESTORE );
                }

                SetWindowPos ( hwnd, HWND_TOPMOST, 0, 0, 0, 0, /*SWP_NOACTIVATE*/ SWP_SHOWWINDOW| SWP_NOMOVE | SWP_NOSIZE );
                SetWindowPos ( hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, /*SWP_NOACTIVATE*/ SWP_SHOWWINDOW| SWP_NOMOVE | SWP_NOSIZE );
            }
            else
            {
                if ( IsIconic ( hwnd ) )
                    ShowWindow ( hwnd, SW_RESTORE );
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

