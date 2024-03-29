
// tabs.c - tab control handling galore
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)


#include "main.h"
#include "tabs.h"

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_NewTab 
/*--------------------------------------------------------------------------*/
//           Type: HWND 
//    Param.    1: HWND hWnd : parent wnd
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Creates a tab control as a child to hWnd
/*--------------------------------------------------------------------@@-@@-*/
HWND Tab_NewTab ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    RECT        wr;
    HWND        hTab;
    ULONG_PTR   style;
    HFONT       font;

    GetClientRect ( hWnd, &wr );
    hTab = CreateWindowEx ( 0, WC_TABCONTROL, NULL, TAB_STYLE, wr.left, wr.top, 
                            wr.right-wr.left, wr.bottom-wr.top, hWnd,
                            (HMENU) IDC_TAB, NULL, NULL);

    style = GetClassLongPtr ( hTab, GCL_STYLE );
    if ( style )
    {
        style &= ~( CS_HREDRAW|CS_VREDRAW );
        style |= ( CS_BYTEALIGNWINDOW|CS_BYTEALIGNCLIENT|CS_PARENTDC );
        SetClassLongPtr ( hTab, GCL_STYLE, style );
    }

    SetClassLongPtr ( hTab, GCLP_HBRBACKGROUND,
        (LONG_PTR)GetStockObject ( NULL_BRUSH ) );

    style = GetWindowLongPtr ( hTab, GWL_STYLE );

    if ( style )
    {
        style &= ~TCS_HOTTRACK;
        SetWindowLongPtr ( hTab, GWL_STYLE, style );
    }

    font = (HFONT) GetStockObject ( DEFAULT_GUI_FONT );
    SendMessage ( hTab, WM_SETFONT, ( WPARAM ) font, MAKELPARAM (TRUE,0) );

    return hTab;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_NewTabPage 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hTab    : tab control
//    Param.    2: int index    : index of the new tab
//    Param.    3: TCHAR * text : caption of the new tab
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Inserts a new tab on a tab control; on success, returns the
//                 new tab index, on failure -1
/*--------------------------------------------------------------------@@-@@-*/
int Tab_NewTabPage ( HWND hTab, int index, TCHAR * text )
/*--------------------------------------------------------------------------*/
{
    TCITEM          tie;

    tie.mask = TCIF_TEXT|TCIF_IMAGE;
    tie.pszText = ( text == NULL ) ? TEXT("") : text;

    return (int)SendMessage ( hTab, TCM_INSERTITEM,
        index, (LPARAM) (LPTCITEM) &tie);
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_DeletePage 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hTab : tab control
//    Param.    2: int index : index of the tab to remove
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Removes a tab froma tab control; TRUE on success,
//                 FALSE otherwise
/*--------------------------------------------------------------------@@-@@-*/
BOOL Tab_DeletePage ( HWND hTab, int index )
/*--------------------------------------------------------------------------*/
{
    return (BOOL)SendMessage ( hTab, TCM_DELETEITEM, index, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_GetCurSel 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hTab : tab control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Return currently selected tab
/*--------------------------------------------------------------------@@-@@-*/
int Tab_GetCurSel ( HWND hTab )
/*--------------------------------------------------------------------------*/
{
    return (int)SendMessage ( hTab, TCM_GETCURSEL, 0, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_SetCurSel 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hTab : tab control
//    Param.    2: int index : tab page to select
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Selects the tab page index and returns prev. selected item,
//                 or -1 on failure
/*--------------------------------------------------------------------@@-@@-*/
int Tab_SetCurSel ( HWND hTab, int index )
/*--------------------------------------------------------------------------*/
{
    return (int)SendMessage ( hTab, TCM_SETCURSEL, (WPARAM)index, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_GetCurFocus 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hTab : tab control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Returns the tab index that has the focus
/*--------------------------------------------------------------------@@-@@-*/
int Tab_GetCurFocus ( HWND hTab )
/*--------------------------------------------------------------------------*/
{
    return (int)SendMessage ( hTab, TCM_GETCURFOCUS, 0, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_SetCurFocus 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hTab : tab control
//    Param.    2: int index : index to set focus
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Set focus on tab with index
/*--------------------------------------------------------------------@@-@@-*/
int Tab_SetCurFocus ( HWND hTab, int index )
/*--------------------------------------------------------------------------*/
{
    return (int)SendMessage ( hTab, TCM_SETCURFOCUS, (WPARAM)index, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_GetCount 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hTab : tab control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Return tab item count
/*--------------------------------------------------------------------@@-@@-*/
int Tab_GetCount ( HWND hTab )
/*--------------------------------------------------------------------------*/
{
    return (int)SendMessage ( hTab, TCM_GETITEMCOUNT, 0, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_SetText 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hTab    : tab control
//    Param.    2: int index    : index of the tab to set text
//    Param.    3: TCHAR * text : text to set
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Set the tab text to index
/*--------------------------------------------------------------------@@-@@-*/
BOOL Tab_SetText ( HWND hTab, int index, TCHAR * text )
/*--------------------------------------------------------------------------*/
{
    TCITEM          tie;

    tie.mask = TCIF_TEXT;
    tie.pszText = ( text == NULL ) ? TEXT("") : text;

    return (BOOL)SendMessage ( hTab, TCM_SETITEM,
        index, (LPARAM) (LPTCITEM) &tie);
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_SetPadding 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hTab: tab control
//    Param.    2: int cx   : horiz. padding
//    Param.    3: int cy   : vert. padding
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Set the title text padding for a tab page
/*--------------------------------------------------------------------@@-@@-*/
void Tab_SetPadding ( HWND hTab, int cx, int cy )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hTab, TCM_SETPADDING, 0, MAKELPARAM ( cx, cy ) );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_SetImageList 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hTab       : tab control
//    Param.    2: HIMAGELIST hIml : image list handle
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Set the image list for a tab control
/*--------------------------------------------------------------------@@-@@-*/
void Tab_SetImageList ( HWND hTab, HIMAGELIST hIml )
/*--------------------------------------------------------------------------*/
{
    SendMessage ( hTab, TCM_SETIMAGELIST, 0, (LPARAM) hIml );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Tab_SetImg 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hTab     : tab control
//    Param.    2: int index     : tab index for the tab to receive img
//    Param.    3: int img_index : img index in the imagelist
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.09.2020
//    DESCRIPTION: Set the image index for a tab page
/*--------------------------------------------------------------------@@-@@-*/
BOOL Tab_SetImg ( HWND hTab, int index, int img_index )
/*--------------------------------------------------------------------------*/
{
    TCITEM          tie;

    tie.mask        = TCIF_IMAGE;
    tie.iImage      = img_index;
    return (BOOL)SendMessage ( hTab, TCM_SETITEM,
        index, (LPARAM) (LPTCITEM) &tie);
}
