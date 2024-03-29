
#ifndef _TABS_H
#define _TABS_H

#include        <windows.h>
#include        <commctrl.h>

#define         IDC_TAB           999
#define         TAB_STYLE         WS_OVERLAPPED|WS_CLIPCHILDREN| \
                                  WS_CLIPSIBLINGS|WS_VISIBLE|WS_CHILD| \
                                  TCS_SINGLELINE|TCS_TABS|TCS_RIGHTJUSTIFY| \
                                  TCS_FOCUSNEVER

extern HWND     Tab_NewTab        ( HWND hwnd );
extern int      Tab_NewTabPage    ( HWND htab, int index, TCHAR * text );
extern BOOL     Tab_DeletePage    ( HWND htab, int index );
extern BOOL     Tab_SetText       ( HWND htab, int index, TCHAR * text );
extern int      Tab_GetCurSel     ( HWND htab );
extern int      Tab_SetCurSel     ( HWND htab, int index );
extern int      Tab_GetCurFocus   ( HWND htab );
extern int      Tab_SetCurFocus   ( HWND htab, int index );
extern int      Tab_GetCount      ( HWND htab );
extern void     Tab_SetPadding    ( HWND htab, int cx, int cy );
extern void     Tab_SetImageList  ( HWND htab, HIMAGELIST hIml );
extern BOOL     Tab_SetImg        ( HWND htab, int index, int img_index );

#endif
