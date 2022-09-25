
#ifndef _MISC_H
#define _MISC_H

#include                    <windows.h>

#define                     HEX_DUMP_WIDTH          16

typedef BOOL                ( CALLBACK * PBPROC )       ( TCHAR *, LPARAM );
typedef BOOL                ( CALLBACK * HEXDUMPPROC )  ( TCHAR *, LPARAM );

extern TCHAR * opn_filter;
extern TCHAR * opn_title;
extern TCHAR * sav_title;
extern TCHAR * opn_defext;
extern TCHAR * app_classname;
extern TCHAR * app_classname2;
extern TCHAR * app_title;
extern TCHAR * macro_hlp;

extern int ShowMessage ( HWND hown, TCHAR * msg, DWORD style, DWORD icon_id );
extern HWND PB_NewProgressBar ( HWND hParent, DWORD left, DWORD width );
extern void BeginDraw ( HWND hwnd );
extern void EndDraw ( HWND hwnd );
extern DWORD undiv ( DWORD val, DWORD * quot );
extern TCHAR * untoa ( DWORD value, TCHAR * buffer, int radix );
extern TCHAR * intoa ( int value, TCHAR * buffer, int radix );
extern DWORD topxy ( DWORD wwidth, DWORD swidth );
extern BOOL Parse_OUT_Buf ( const TCHAR * src, PBPROC pbproc, LPARAM lParam );
extern LRESULT CALLBACK Parse_OUT_Buf_Proc ( const TCHAR * s, LPARAM lParam );
extern void HexDump 
    ( const TCHAR * src, DWORD dwsrclen, 
    HEXDUMPPROC hexdumpproc, LPARAM lParam );

extern BOOL CALLBACK HexDumpProc ( const TCHAR * line, LPARAM lParam );
extern BOOL ExecAndCapture 
    ( TCHAR * cmdline, TCHAR * workindir, PBPROC pbproc, LPARAM lParam );

extern BOOL SimpleExec ( TCHAR * cmdline, const TCHAR * workindir );
extern TCHAR * GetDefaultBrowser ( void );
extern BOOL CopyListToClipboard ( HWND hparent, HWND hList );
extern BOOL CopyListSelToClipboard ( HWND hparent, HWND hList );
extern BOOL CenterDlg ( HWND hdlg, HWND hparent );
extern BOOL IsThereAnotherInstance ( const TCHAR * classname );
extern TCHAR * Today ( void );
extern TCHAR * Now ( void );
extern TCHAR * ExpandMacro ( const TCHAR * src, const TCHAR * filespec );
extern TCHAR * ChmFromModule ( HMODULE hMod );
extern TCHAR * FileVersionAsString ( const TCHAR * fpath );

#endif
