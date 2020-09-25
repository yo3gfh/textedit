#ifndef _THREADS_H
#define _THREADS_H

#include        <windows.h>
#include        "encoding.h"
#include        "cfunction.h"
#include        "rich.h"

#define         WM_ENDCFDECL        WM_APP + 1      // signal end of CF_FindDecls   ->
#define         WM_UPDCFDECL        WM_APP + 2      // signal a sbar update         ->
#define         WM_BUFCFDECL        WM_APP + 3      // signal a Outw listbox update ->
#define         WM_ABTCFDECL        WM_APP + 100    // signal user abort            <-

#define         WM_ENDCFDEFS        WM_APP + 4      // signal end of CF_FindDefs    ->
#define         WM_UPDCFDEFS        WM_APP + 5      // signal a sbar update         ->
#define         WM_BUFCFDEFS        WM_APP + 6      // signal a Outw listbox update ->
#define         WM_ABTCFDEFS        WM_APP + 101    // signal user abort            <-

#define         WM_ENDCFDECLSEL     WM_APP + 7      // signal end of CF_FindDecls   ->
#define         WM_UPDCFDECLSEL     WM_APP + 8      // signal a sbar update         ->
#define         WM_BUFCFDECLSEL     WM_APP + 9      // signal a Outw listbox update ->
#define         WM_ABTCFDECLSEL     WM_APP + 102    // signal user abort            <-

#define         WM_ENDCFDEFSSEL     WM_APP + 10     // signal end of CF_FindDefs    ->
#define         WM_UPDCFDEFSSEL     WM_APP + 11     // signal a sbar update         ->
#define         WM_BUFCFDEFSSEL     WM_APP + 12     // signal a Outw listbox update ->
#define         WM_ABTCFDEFSSEL     WM_APP + 103    // signal user abort            <-

#define         WM_ENDCVTABS        WM_APP + 13     // signal end of tab conversion ->
#define         WM_UPDCVTABS        WM_APP + 14     // signal a sbar update         ->
#define         WM_ABTCVTABS        WM_APP + 104    // signal user abort            <-

#define         WM_ENDEXECTOOL      WM_APP + 15     // signal end of tab exectool   ->
#define         WM_BUFEXECTOOL      WM_APP + 16     // signal a Outw listbox update ->
#define         WM_ABTEXECTOOL      WM_APP + 105    // signal user abort            <-

#define         WM_RAGEQUIT         WM_APP + 199    // signal abort by quit         <-

typedef struct _exec_thread_data
{
    PBPROC          proc;
    TCHAR           filename[TMAX_PATH];
    TCHAR           curdir[TMAX_PATH];
    TCHAR           cmd[TMAX_PATH];
    LPARAM          lParam;
    WPARAM          wParam;
    BOOL            capture;
    BOOL            lastcmd;
    UINT            errcode;
} EXEC_THREAD_DATA;

typedef struct _findc_thread_data
{
    CFSEARCHPROC    proc;
    LPARAM          lParam;
    HWND            hRich;
    UINT            errcode;
} FINDC_THREAD_DATA;

typedef struct _tabrep_thread_data
{
    CVTTABSPROC     proc;
    LPARAM          lParam;
    HWND            hRich;
    DWORD           dwTabs;
    UINT            errcode;
} TABREP_THREAD_DATA;

extern UINT __stdcall      Thread_ConvertTabsToSpaces   ( void * thData );
extern UINT __stdcall      Thread_CFFindDecls           ( void * thData );
extern UINT __stdcall      Thread_CFFindDeclsSel        ( void * thData );
extern UINT __stdcall      Thread_CFFindDefs            ( void * thData );
extern UINT __stdcall      Thread_CFFindDefsSel         ( void * thData );
extern UINT __stdcall      Thread_ExecTool              ( void * thData );

#endif


