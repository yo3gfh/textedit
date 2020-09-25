#pragma warn(disable: 2008 2118 2228 2231 2030 2260) 

#include "main.h"
#include <windows.h>
#include <strsafe.h>
#include "misc.h"
#include "mem.h"
#include "rich.h"
#include "cfunction.h"
#include "threads.h"
#include "textedit.h"

/*-@@+@@--------------------------------------------------------------------------------------------*/
//
// NOTE:    These functions are called by _beginthreadex and start in a separate thread. They  
//       are passed a pointer to a structure with data of interest. They signal completion by  
//       posting a private message to main window queue; they return the received pointer as  
//       lParam and an exit code in wParam (0 = FAIL, >0 SUCCESS).
//          They are not designed to be exec. more than one at a time. If so desired, then their
//       called functions must either write to separate buffers, or use critical sections (or other
//       sync. means), otherwise great booboo happens =))
//          Their purpose was to only free the main thread so you can quit app or be amazed 
//       at the aboutbox, lol.
/*--------------------------------------------------------------------------------------------@@-@@-*/

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: Thread_ConvertTabsToSpaces 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: UINT __stdcall 
//                 Param.    1: void * thData : pointer to a TABREP_THREAD_DATA, as sent by 
//                              _beginthreadex
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Tab replace (in separate thread)
/*--------------------------------------------------------------------------------------------@@-@@-*/
UINT __stdcall Thread_ConvertTabsToSpaces ( void * thData )
/*--------------------------------------------------------------------------------------------------*/
{
    TABREP_THREAD_DATA  * pttd;

    if ( thData == NULL )
        return FALSE;

    pttd                = (TABREP_THREAD_DATA *)thData;

    Rich_ConvertTabsToSpaces ( pttd->hRich, pttd->dwTabs, pttd->proc, pttd->lParam );

    return (UINT)PostMessage ( (HWND)(pttd->lParam), WM_ENDCVTABS, 1, (LPARAM)thData );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: Thread_CFFindDefs 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: UINT __stdcall 
//                 Param.    1: void * thData : pointer to a FINDC_THREAD_DATA, as sent by 
//                                              _beginthreadex
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Find C function defs (in separate thread)
/*--------------------------------------------------------------------------------------------@@-@@-*/
UINT __stdcall Thread_CFFindDefs ( void * thData )
/*--------------------------------------------------------------------------------------------------*/
{
    FINDC_THREAD_DATA   * pftd;
    DWORD               crtline, lcount;
    INT_PTR             size;

    if ( thData == NULL )
        return FALSE;

    pftd                = (FINDC_THREAD_DATA *)thData;
    crtline             = Rich_GetCrtLine ( pftd->hRich );
    lcount              = Rich_GetLineCount ( pftd->hRich );

    size = CF_FindCFDefinitions ( pftd->hRich, crtline, lcount-1, pftd->proc, pftd->lParam );

    return (UINT)PostMessage ( (HWND)(pftd->lParam), WM_ENDCFDEFS, (WPARAM)size, (LPARAM)thData );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: Thread_CFFindDefsSel 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: UINT __stdcall 
//                 Param.    1: void * thData : pointer to a FINDC_THREAD_DATA, as sent by 
//                                              _beginthreadex
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Find C function defs from selection (in separate thread)
/*--------------------------------------------------------------------------------------------@@-@@-*/
UINT __stdcall Thread_CFFindDefsSel ( void * thData )
/*--------------------------------------------------------------------------------------------------*/
{
    FINDC_THREAD_DATA    * pftd;
    DWORD               start_line, end_line;
    CHARRANGE           chr;
    INT_PTR             size;

    if ( thData == NULL )
        return FALSE;

    pftd                = (FINDC_THREAD_DATA *)thData;

    Rich_GetSel ( pftd->hRich, &chr );
    start_line          = Rich_LineFromChar ( pftd->hRich, chr.cpMin );
    end_line            = Rich_LineFromChar ( pftd->hRich, chr.cpMax );//Rich_GetCrtLine ( hRich );

    size = CF_FindCFDefinitions ( pftd->hRich, start_line, end_line, pftd->proc, pftd->lParam );

    return (UINT)PostMessage ( (HWND)(pftd->lParam), WM_ENDCFDEFSSEL, (WPARAM)size, (LPARAM)thData );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: Thread_CFFindDecls 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: UINT __stdcall 
//                 Param.    1: void * thData : pointer to a FINDC_THREAD_DATA, as sent by 
//                                              _beginthreadex
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Find C function decls. (in separate thread)
/*--------------------------------------------------------------------------------------------@@-@@-*/
UINT __stdcall Thread_CFFindDecls ( void * thData )
/*--------------------------------------------------------------------------------------------------*/
{
    FINDC_THREAD_DATA   * pftd;
    DWORD               crtline, lcount;
    INT_PTR             size;

    if ( thData == NULL )
        return FALSE;

    pftd                = (FINDC_THREAD_DATA *)thData;
    crtline             = Rich_GetCrtLine ( pftd->hRich );
    lcount              = Rich_GetLineCount ( pftd->hRich );

    size = CF_FindCFDeclarations ( pftd->hRich, crtline, lcount-1, pftd->proc, pftd->lParam );

    return (UINT)PostMessage ( (HWND)(pftd->lParam), WM_ENDCFDECL, (WPARAM)size, (LPARAM)thData );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: Thread_CFFindDeclsSel 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: UINT __stdcall 
//                 Param.    1: void * thData : pointer to a FINDC_THREAD_DATA, as sent by 
//                                              _beginthreadex
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Find C function decls from selection (in separate thread)
/*--------------------------------------------------------------------------------------------@@-@@-*/
UINT __stdcall Thread_CFFindDeclsSel ( void * thData )
/*--------------------------------------------------------------------------------------------------*/
{
    FINDC_THREAD_DATA   * pftd;
    DWORD               start_line, end_line;
    CHARRANGE           chr;
    INT_PTR             size;

    if ( thData == NULL )
        return FALSE;

    pftd                = (FINDC_THREAD_DATA *)thData;
    Rich_GetSel ( pftd->hRich, &chr );
    start_line          = Rich_LineFromChar ( pftd->hRich, chr.cpMin );
    end_line            = Rich_LineFromChar ( pftd->hRich, chr.cpMax );//Rich_GetCrtLine ( hRich );

    size = CF_FindCFDeclarations ( pftd->hRich, /*pftd->out_buf, pftd->cchMax, */start_line, end_line, pftd->proc, pftd->lParam );

    return (UINT)PostMessage ( (HWND)(pftd->lParam), WM_ENDCFDECLSEL, (WPARAM)size, (LPARAM)thData );
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: Thread_ExecTool 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: UINT __stdcall 
//                 Param.    1: void * thData : pointer to a EXEC_THREAD_DATA, as sent by 
//                                              _beginthreadex
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 20.09.2020
//                 DESCRIPTION: Start the selected command line as a process in a separate
//                              thread
/*--------------------------------------------------------------------------------------------@@-@@-*/
UINT __stdcall Thread_ExecTool ( void * thData )
/*--------------------------------------------------------------------------------------------------*/
{
    TCHAR               * cmd, * fcmd, * curdir;
    EXEC_THREAD_DATA    * petd;
    BOOL                res;
    INT_PTR             size, alloc_size;

    res                 = FALSE;

    if ( thData == NULL )
        return FALSE;

    petd                = (EXEC_THREAD_DATA *) thData;
    cmd                 = ExpandMacro ( petd->cmd, petd->filename );

    if ( cmd == NULL )
        return (UINT)PostMessage ( (HWND)(petd->lParam), WM_ENDEXECTOOL, (WPARAM)res, (LPARAM)thData );

    curdir              = NULL;

    if ( petd->curdir[0] != TEXT('\0') )
        curdir = petd->curdir;

    __try
    {
        if ( petd->capture )
        {
            size        = lstrlen (cmd);
            size        += MAX_PATH;
            alloc_size  = size * sizeof(TCHAR);
            fcmd        = alloc_and_zero_mem ( (alloc_size+4096+1) & (~4095) );

            if ( fcmd != NULL )
            {
                StringCchCopy ( fcmd, size, TEXT("spawn.exe ") );
                StringCchCat ( fcmd, size, cmd );

                res = ExecAndCapture ( fcmd, curdir, petd->proc, petd->lParam );

                free_mem ( fcmd );
            }

        }
        else
        {
            res = SimpleExec ( cmd, curdir ); 
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        res = FALSE;
    }

    free_mem ( cmd );

    return (UINT)PostMessage ( (HWND)(petd->lParam), WM_ENDEXECTOOL, (WPARAM)res, (LPARAM)thData );
}
