
#ifndef _SBAR_H
#define _SBAR_H

#include        <windows.h>
#include        <commctrl.h>

#define         IDC_STATUS              666

extern HWND SB_MakeStatusBar ( HWND hParent, int sbid );
extern void SB_MenuHelp ( HINSTANCE hInstance, HWND hsbar, DWORD index );
extern BOOL SB_StatusResizePanels ( HWND hsbar );
extern void SB_StatusSetSimple ( HWND hsbar, BOOL simple );
extern BOOL SB_StatusSetText ( HWND hsbar, int panel, const TCHAR * text );

#endif
