
#ifndef _CONFIG_H
#define _CONFIG_H

#include <windows.h>

extern HFONT LoadFontFromInifile ( const TCHAR * inifile );
extern void SaveWindowPosition ( HWND hwnd, const TCHAR * inifile );
extern BOOL LoadWindowPosition ( RECT * r, const TCHAR * inifile );

extern void SaveDlgPosition 
    ( HWND hwnd, const TCHAR * inifile, const TCHAR * section );

extern BOOL LoadDlgPosition 
    ( RECT * r, const TCHAR * inifile, const TCHAR * section );

extern BOOL LoadCBBHistoryFromINI 
    ( HWND hDlg, int cbbid, const TCHAR * inisection, 
    const TCHAR * inifile, BOOL restoresel );

extern BOOL SaveCBBHistoryToINI 
    ( HWND hDlg, int cbbid, const TCHAR * inisection, 
    const TCHAR * inifile );

extern TCHAR * IniFromModule ( HMODULE hMod );
extern TCHAR * ReadAuthorFromIni ( const TCHAR * inifile );

#endif
