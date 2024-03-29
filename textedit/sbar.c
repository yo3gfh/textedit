
//sbar.c - the humble status bar 
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include "main.h"
#include "sbar.h"

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SB_MakeStatusBar 
/*--------------------------------------------------------------------------*/
//           Type: HWND 
//    Param.    1: HWND hParent: parent to SB
//    Param.    2: int sbid    : unique control ID
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Let's make a StatusBar!
/*--------------------------------------------------------------------@@-@@-*/
HWND SB_MakeStatusBar ( HWND hParent, int sbid )
/*--------------------------------------------------------------------------*/
{
    HWND        hstatus;
    int         sbparts[4];
    ULONG_PTR   style;

    hstatus = CreateWindowEx ( WS_EX_COMPOSITED, STATUSCLASSNAME, NULL,
                                 //WS_EX_COMPOSITED, for flicker :-)
                                WS_CHILDWINDOW|WS_VISIBLE|SBS_SIZEGRIP, 
                                0, 0, 0, 0, hParent, (HMENU)sbid,
                                GetModuleHandle(NULL), NULL );

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

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SB_MenuHelp 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HINSTANCE hInstance: current hinst
//    Param.    2: HWND hsbar         : handle to SB
//    Param.    3: DWORD index        : menu index+flags
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Some function to display menu items help strings in
//                 the statusbar when hovering with the mouse. MFC style :-)
//                 This is called in response to WM_MENUSELECT, with
//                 wParam as index.
/*--------------------------------------------------------------------@@-@@-*/
void SB_MenuHelp ( HINSTANCE hInstance, HWND hsbar, DWORD index )
/*--------------------------------------------------------------------------*/
{
    TCHAR    temp[256];

    if ( HIWORD ( index ) == 0xFFFF )           // user has closed the menu
        SB_StatusSetSimple ( hsbar, FALSE );
    else
    {
        SB_StatusSetSimple ( hsbar, TRUE );
        if ( HIWORD ( index ) & MF_HILITE )     // item is highlighted
        {
            // load help string from resource
            LoadString ( hInstance, LOWORD ( index ), temp, ARRAYSIZE(temp));
            SB_StatusSetText ( hsbar, 255|SBT_NOBORDERS, temp );
        }
    }
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SB_StatusResizePanels 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hsbar : status bar
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Resize SB, according to text length
/*--------------------------------------------------------------------@@-@@-*/
BOOL SB_StatusResizePanels ( HWND hsbar )
/*--------------------------------------------------------------------------*/
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

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SB_StatusSetSimple 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hsbar  : Status bar
//    Param.    2: BOOL simple : yes or no :-)
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Make a statusbar singlepanel or restore it's original
//                 structure
/*--------------------------------------------------------------------@@-@@-*/
void SB_StatusSetSimple ( HWND hsbar, BOOL simple )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hsbar, SB_SIMPLE, (WPARAM)simple, (LPARAM)0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SB_StatusSetText 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hsbar         : status bar
//    Param.    2: int panel          : panel index (0 based)
//    Param.    3: const TCHAR * text : text to set
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Set panel with index "panel" caption to "text"
/*--------------------------------------------------------------------@@-@@-*/
BOOL SB_StatusSetText ( HWND hsbar, int panel, const TCHAR * text )
/*--------------------------------------------------------------------------*/
{
    return (BOOL)SendMessage ( hsbar, SB_SETTEXT, panel, (LPARAM)text );
}

