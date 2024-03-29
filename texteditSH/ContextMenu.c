
// contextmenu.c

/*--------------------------------------------------------------
    The IShellExtInit interface is incorporated into the 
    IContextMenu interface
--------------------------------------------------------------*/

#include "texteditsh.h"
#include <windows.h>
#include <shlobj.h>
#include <strsafe.h>
#include <stdarg.h>
#include <stdio.h>
#include "ContextMenu.h"

#define SEVERITY_SUCCESS    0
#define IDM_OWTE            0x0001
#define IDB_OWTE            8001
// sizeof TCHAR would do, but let's be generous
#define ALLOC_MULT          (sizeof(TCHAR)+2)   
// OTOH, let's put a limit on how much filenames buf we process                
#define MAX_CHARS           (0x00010000 / sizeof(TCHAR))         
#define COPYDATA_MAGIC      0x0666DEAD

#pragma warn(disable: 2231 2030 2260)                           

const TCHAR * FILE_Extract_path ( const TCHAR * src, BOOL last_bslash );

// Keep a count of DLL references
extern UINT g_uiRefThisDll;
extern HINSTANCE g_hInstance;
HBITMAP g_hMenuBitmap;

// The virtual table of functions for IContextMenu interface
IContextMenuVtbl icontextMenuVtbl = {
    CContextMenuExt_QueryInterface,
    CContextMenuExt_AddRef,
    CContextMenuExt_Release,
    CContextMenuExt_QueryContextMenu,
    CContextMenuExt_InvokeCommand,
    CContextMenuExt_GetCommandString
};

// The virtual table of functions for IShellExtInit interface
IShellExtInitVtbl ishellInitExtVtbl = {
    CShellInitExt_QueryInterface,
    CShellInitExt_AddRef,
    CShellInitExt_Release,
    CShellInitExt_Initialize
};

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_Create 
/*--------------------------------------------------------------------------*/
//           Type: IContextMenu * 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: IContextMenu constructor
/*--------------------------------------------------------------------@@-@@-*/
IContextMenu * CContextMenuExt_Create ( void )
/*--------------------------------------------------------------------------*/
{
    // Create the ContextMenuExtStruct that will contain interfaces and vars
    ContextMenuExtStruct * pCM = malloc ( sizeof(ContextMenuExtStruct) );
    if(!pCM)
        return NULL;

    // Point to the IContextMenu and IShellExtInit Vtbl's
    pCM->cm.lpVtbl = &icontextMenuVtbl;
    pCM->si.lpVtbl = &ishellInitExtVtbl;

    // increment the class reference count
    pCM->m_ulRef = 1;
    pCM->m_pszSource = NULL;

    g_uiRefThisDll++;

    // Return the IContextMenu virtual table
    return &pCM->cm;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_QueryInterface 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IContextMenu * this: 
//    Param.    2: REFIID riid        : 
//    Param.    3: LPVOID *ppv        : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: IContextMenu interface routines
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CContextMenuExt_QueryInterface 
    ( IContextMenu * this, REFIID riid, LPVOID *ppv )
/*--------------------------------------------------------------------------*/
{
    // The address of the struct is the same as the address
    // of the IContextMenu Virtual table. 
    ContextMenuExtStruct * pCM = (ContextMenuExtStruct*)this;
    
    if ( IsEqualIID ( riid, &IID_IUnknown ) 
        || IsEqualIID ( riid, &IID_IContextMenu ) )
    {
        *ppv = this;
        pCM->m_ulRef++;
        return S_OK;
    }
    else if ( IsEqualIID ( riid, &IID_IShellExtInit ) )
    {
        // Give the IShellInitExt interface
        *ppv = &pCM->si;
        pCM->m_ulRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_AddRef 
/*--------------------------------------------------------------------------*/
//           Type: ULONG STDMETHODCALLTYPE 
//    Param.    1: IContextMenu * this : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: 
/*--------------------------------------------------------------------@@-@@-*/
ULONG STDMETHODCALLTYPE CContextMenuExt_AddRef ( IContextMenu * this )
/*--------------------------------------------------------------------------*/
{
    ContextMenuExtStruct * pCM = (ContextMenuExtStruct*)this;
    return ++pCM->m_ulRef;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_Release 
/*--------------------------------------------------------------------------*/
//           Type: ULONG STDMETHODCALLTYPE 
//    Param.    1: IContextMenu * this : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
ULONG STDMETHODCALLTYPE CContextMenuExt_Release ( IContextMenu * this )
/*--------------------------------------------------------------------------*/
{
    ContextMenuExtStruct * pCM = (ContextMenuExtStruct*)this;

    if ( --pCM->m_ulRef == 0 )
    {
        free ( pCM->m_pszSource );
        free ( this );
        DeleteObject ( g_hMenuBitmap );
        g_uiRefThisDll--;
        return 0;
    }

    return pCM->m_ulRef;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_GetCommandString 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IContextMenu * this: 
//    Param.    2: UINT_PTR idCmd     : 
//    Param.    3: UINT uFlags        : 
//    Param.    4: UINT *pwReserved   : 
//    Param.    5: LPSTR pszName      : 
//    Param.    6: UINT cchMax        : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CContextMenuExt_GetCommandString 
    ( IContextMenu * this, UINT_PTR idCmd, UINT uFlags, 
    UINT *pwReserved, LPSTR pszName, UINT cchMax )
/*--------------------------------------------------------------------------*/
{
    HRESULT hr = S_OK;

    switch ( uFlags )
    {
        case GCS_HELPTEXTA:
        switch ( idCmd )
        {
            case IDM_OWTE:
            StringCchCopyA ( (LPSTR)pszName, cchMax, "Open with TextEdit" );
            hr = S_OK;
            break;
            
            default:
            hr = E_NOTIMPL;
            break;
        }
        break;

        case GCS_HELPTEXTW:
        switch ( idCmd )
        {
            case IDM_OWTE:
            StringCchCopyW ( (LPWSTR)pszName, cchMax, L"Open with TextEdit" );
            hr = S_OK;
            break;

            default:
            hr = E_NOTIMPL;
            break;
        }
        break;
    }
    return hr;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_QueryContextMenu 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IContextMenu * this: 
//    Param.    2: HMENU hMenu        : 
//    Param.    3: UINT uiIndexMenu   : 
//    Param.    4: UINT idCmdFirst    : 
//    Param.    5: UINT idCmdLast     : 
//    Param.    6: UINT uFlags        : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CContextMenuExt_QueryContextMenu 
    ( IContextMenu * this, HMENU hMenu, UINT uiIndexMenu, 
    UINT idCmdFirst, UINT idCmdLast, UINT uFlags )
/*--------------------------------------------------------------------------*/
{
    HINSTANCE        hinst;
    HBITMAP          hbmp;
    UINT             ix;
    ULONG            sev;
    MENUITEMINFO     mi;

    hinst            = g_hInstance;
    hbmp             = LoadBitmap ( hinst, MAKEINTRESOURCE(IDB_OWTE) );
    g_hMenuBitmap    = hbmp;

    ix               = uiIndexMenu;
    sev              = SEVERITY_SUCCESS;

    RtlZeroMemory ( &mi, sizeof (MENUITEMINFO) );
    mi.cbSize        = sizeof (MENUITEMINFO);
    mi.fMask         = MIIM_STRING | MIIM_STATE | MIIM_BITMAP | MIIM_FTYPE | 
                        MIIM_ID | MIIM_CHECKMARKS;
    mi.dwTypeData    = TEXT("Open with TextEdit");
    mi.wID           = idCmdFirst + IDM_OWTE;
    mi.fType         = MFT_STRING;
    mi.fState        = MFS_ENABLED;
    mi.hbmpItem      = hbmp;
    mi.hbmpChecked   = hbmp;
    mi.hbmpUnchecked = hbmp;

    __try
    {
        InsertMenu(hMenu, ix++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
        InsertMenuItem ( hMenu, ix++, TRUE, &mi );
        InsertMenu(hMenu, ix++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        sev = SEVERITY_ERROR;
    }
    
    return MAKE_HRESULT(sev, FACILITY_NULL, (USHORT)(IDM_OWTE + 1));
}

PROCESS_INFORMATION pi;
STARTUPINFO         si;

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_InvokeCommand 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IContextMenu * this         : 
//    Param.    2: LPCMINVOKECOMMANDINFO lpici : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CContextMenuExt_InvokeCommand 
    ( IContextMenu * this, LPCMINVOKECOMMANDINFO lpici )
/*--------------------------------------------------------------------------*/
{
    // here we dump a bunch of filenames user selected.. 
    // how many he selected? lol
    // pCM->m_pszSource_bsize should tell us

    TCHAR           * commandline;
    TCHAR           modulename [MAX_PATH];
    HRESULT         hr = S_OK;
    DWORD           cmdl_buf_size, alloc_size;
    HWND            hEditor;
    COPYDATASTRUCT  cd;

    ContextMenuExtStruct * pCM = ( ContextMenuExtStruct* )this;

    if ( !GetModuleFileName ( g_hInstance, modulename, ARRAYSIZE(modulename)))
    {
        hr = E_FAIL;
        return hr;
    }

    if ( pCM->m_pszSource_bsize == 0 )
    {
        hr = E_FAIL;
        return hr;
    }

    alloc_size = pCM->m_pszSource_bsize + MAX_PATH;
    cmdl_buf_size = alloc_size / sizeof(TCHAR);

    commandline = malloc ( (alloc_size+4096+1) & (~4095) );

    if ( commandline == NULL )
    {
        hr = E_FAIL;
        return hr;
    }

    // the classname for our TextEdit editor
    hEditor = FindWindow ( TEXT("TED_CLASS_666"), NULL ); 

    __try
    {
        switch ( LOWORD(lpici->lpVerb) )
        {
            case IDM_OWTE:
            {
                if ( hEditor == NULL )
                {
                    StringCchCopy ( commandline, cmdl_buf_size, 
                        FILE_Extract_path ( modulename, TRUE ) );
                    StringCchCat  ( commandline, cmdl_buf_size, 
                        TEXT("textedit.exe ") );
                    StringCchCat  ( commandline, cmdl_buf_size, 
                        pCM->m_pszSource );
                    RtlZeroMemory ( &si, sizeof(si) );
                    RtlZeroMemory ( &pi, sizeof(pi) );
                    si.cb = sizeof(si);
                    CreateProcess ( NULL, commandline, NULL, NULL, 1, 
                        NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi );
                }
                else
                {
                    // not needed, just so we use the same function for
                    // both cases (cmdline and WM_COPYDATA, see textedit.c)
                    StringCchCopy ( commandline, cmdl_buf_size, 
                        TEXT("0x0666DEAD ") ); 
                    StringCchCat  ( commandline, cmdl_buf_size, 
                        pCM->m_pszSource );
                    cd.dwData = COPYDATA_MAGIC;
                    // lstrlen (commandline) would 
                    // be more appropriate, but we don't use it anyway
                    cd.cbData = alloc_size; 
                    cd.lpData = commandline;
                    SendMessage ( hEditor, WM_COPYDATA, 
                        (WPARAM)NULL, (LPARAM)&cd ); // don't PostMessage :-)
                }
            }
                break;

            default:
                hr = E_FAIL;
                break;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = E_FAIL;
    }

    free ( commandline );
    return hr;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_ErrMessage 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: DWORD dwErrcode : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
void CContextMenuExt_ErrMessage ( DWORD dwErrcode )
/*--------------------------------------------------------------------------*/
{
    LPTSTR pMsgBuf;

    FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErrcode, 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPTSTR)&pMsgBuf, 0, NULL);

    MessageBox ( GetForegroundWindow(), pMsgBuf, 
        TEXT("TextEditSH"), MB_ICONERROR );

    LocalFree(pMsgBuf);
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CShellInitExt_QueryInterface 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IShellExtInit * this: 
//    Param.    2: REFIID riid         : 
//    Param.    3: LPVOID *ppv         : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: IShellExtInit interface routines
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CShellInitExt_QueryInterface 
    ( IShellExtInit * this, REFIID riid, LPVOID *ppv )
/*--------------------------------------------------------------------------*/
{
    /*-----------------------------------------------------------------
    IContextMenu Vtbl is the same address as ContextMenuExtStruct.
     IShellExtInit is sizeof(IContextMenu) further on.
    -----------------------------------------------------------------*/
    IContextMenu * pIContextMenu = (IContextMenu *)(this-1);
    return pIContextMenu->lpVtbl->QueryInterface ( pIContextMenu, riid, ppv );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CShellInitExt_AddRef 
/*--------------------------------------------------------------------------*/
//           Type: ULONG STDMETHODCALLTYPE 
//    Param.    1: IShellExtInit * this : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
ULONG STDMETHODCALLTYPE CShellInitExt_AddRef ( IShellExtInit * this )
/*--------------------------------------------------------------------------*/
{
    // Redirect the IShellExtInit's AddRef to the IContextMenu interface
    IContextMenu * pIContextMenu = (IContextMenu *)(this-1);
    return pIContextMenu->lpVtbl->AddRef ( pIContextMenu );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CShellInitExt_Release 
/*--------------------------------------------------------------------------*/
//           Type: ULONG STDMETHODCALLTYPE 
//    Param.    1: IShellExtInit * this : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
ULONG STDMETHODCALLTYPE CShellInitExt_Release ( IShellExtInit * this )
/*--------------------------------------------------------------------------*/
{
    // Redirect the IShellExtInit's Release to the IContextMenu interface
    IContextMenu * pIContextMenu = (IContextMenu *)(this-1);
    return pIContextMenu->lpVtbl->Release ( pIContextMenu );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CShellInitExt_Initialize 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IShellExtInit * this    : 
//    Param.    2: LPCITEMIDLIST pidlFolder: 
//    Param.    3: LPDATAOBJECT lpdobj     : 
//    Param.    4: HKEY hKeyProgID         : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Get the selected file list from Explorer, transform it into
//                 quoted and space separated file list
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CShellInitExt_Initialize 
    ( IShellExtInit * this, LPCITEMIDLIST pidlFolder, 
    LPDATAOBJECT lpdobj, HKEY hKeyProgID )
/*--------------------------------------------------------------------------*/
{
    FORMATETC      fe;
    STGMEDIUM      stgmed;
    #ifndef UNICODE
    char           * temp;
    ULONG          bSize = 0;
    #endif
    ULONG          iSize = 0;
    ULONG          i, j;

    fe.cfFormat    = CF_HDROP;
    fe.ptd         = NULL;
    fe.dwAspect    = DVASPECT_CONTENT;
    fe.lindex      = -1;
    fe.tymed       = TYMED_HGLOBAL;

    ContextMenuExtStruct * pCM = (ContextMenuExtStruct *)(this-1);
    
    // Get the storage medium from the data object.
    HRESULT hr = lpdobj->lpVtbl->GetData ( lpdobj, &fe, &stgmed );

    if ( SUCCEEDED(hr) )
    {
        if ( stgmed.hGlobal )
        {
            __try
            {
                LPDROPFILES pDropFiles = 
                    (LPDROPFILES)GlobalLock ( stgmed.hGlobal );

                LPSTR pszFiles  = NULL, pszTemp = NULL;
                LPWSTR pswFiles = NULL, pswTemp = NULL;

                if (pDropFiles->fWide)
                {
                    pswFiles    =    (LPWSTR) 
                        ((BYTE*) pDropFiles + pDropFiles->pFiles);
                    pswTemp     =    (LPWSTR) 
                        ((BYTE*) pDropFiles + pDropFiles->pFiles);
                }
                else
                {
                    pszFiles    =    (LPSTR) pDropFiles + pDropFiles->pFiles;
                    pszTemp     =    (LPSTR) pDropFiles + pDropFiles->pFiles;
                }

                while ( pszFiles && *pszFiles || pswFiles && *pswFiles )
                {
                    if ( pDropFiles->fWide )
                    {
                        //Get size of first file/folders path
                        #ifndef UNICODE
                        iSize += WideCharToMultiByte ( CP_ACP, 0, pswFiles, 
                            -1, NULL, 0, NULL, NULL );
                        #else
                        iSize += lstrlenW ( pswFiles ) + 1;
                        #endif
                        pswFiles += (lstrlenW ( pswFiles ) + 1 );
                    }
                    else
                    {
                        //Get size of first file/folders path
                        iSize += lstrlenA ( pszFiles ) + 1;
                        pszFiles += (lstrlenA ( pszFiles ) + 1 );
                    }
                }

                if ( iSize )
                {
                    if ( iSize > MAX_CHARS )
                        iSize = MAX_CHARS;

                    pCM->m_pszSource = malloc ( iSize<<ALLOC_MULT );
                    // if we fail, at least signal we have crap
                    pCM->m_pszSource_bsize = 0; 

                    if ( !pCM->m_pszSource )
                    {
                        hr = E_OUTOFMEMORY;
                        GlobalUnlock ( stgmed.hGlobal );
                        ReleaseStgMedium ( &stgmed );
                        return hr;
                    }

                    RtlZeroMemory ( pCM->m_pszSource, iSize<<ALLOC_MULT );

                    if ( pswTemp )
                    {
                        #ifndef UNICODE
                        temp = malloc ( iSize<<ALLOC_MULT );

                        if ( !temp )
                        {
                            hr = E_OUTOFMEMORY;
                            GlobalUnlock ( stgmed.hGlobal );
                            ReleaseStgMedium ( &stgmed );
                            return hr;
                        }

                        RtlZeroMemory ( temp, iSize<<ALLOC_MULT );
                        bSize = WideCharToMultiByte ( CP_ACP, 0, pswTemp, 
                            iSize, temp, iSize, NULL, NULL );
                        pCM->m_pszSource[0] = TEXT('\"');

                        for ( i = 0, j = 1; i < bSize-1; i++, j++ )
                        {
                            if ( temp[i] == '\0' )
                            {
                                pCM->m_pszSource[j++] = TEXT('\"');
                                pCM->m_pszSource[j++] = TEXT(' ');
                                pCM->m_pszSource[j]   = TEXT('\"');
                                continue;
                            }
                            pCM->m_pszSource[j] = (TCHAR)temp[i];
                        }

                        pCM->m_pszSource[j++] = TEXT('\"');
                        pCM->m_pszSource[j]   = TEXT('\0');
                        pCM->m_pszSource_bsize = j<<ALLOC_MULT;
                        free(temp);

                        #else // UNICODE build
                        pCM->m_pszSource[0] = TEXT('\"');

                        for ( i = 0, j = 1; i < iSize-1; i++, j++ )
                        {
                            if ( pswTemp[i] == TEXT('\0') )
                            {
                                pCM->m_pszSource[j++] = TEXT('\"');
                                pCM->m_pszSource[j++] = TEXT(' ');
                                pCM->m_pszSource[j]   = TEXT('\"');
                                continue;
                            }
                            pCM->m_pszSource[j] = pswTemp[i];
                        }

                        pCM->m_pszSource[j++]  = TEXT('\"');
                        pCM->m_pszSource[j]    = TEXT('\0');
                        pCM->m_pszSource_bsize = j<<ALLOC_MULT;
                        #endif
                    }
                    else
                    {
                        pCM->m_pszSource[0] = TEXT('\"');
                        for ( i = 0, j = 1; i < iSize-1; i++, j++ )
                        {
                            if ( pszTemp[i] == '\0' )
                            {
                                pCM->m_pszSource[j++] = TEXT('\"');
                                pCM->m_pszSource[j++] = TEXT(' ');
                                pCM->m_pszSource[j]   = TEXT('\"');
                                continue;
                            }
                            pCM->m_pszSource[j] = pszTemp[i];
                        }
                        pCM->m_pszSource[j++]  = TEXT('\"');
                        pCM->m_pszSource[j]    = TEXT('\0');
                        pCM->m_pszSource_bsize = j<<ALLOC_MULT;
                    }
                }
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                hr = E_UNEXPECTED;
            }
        }
        else
            hr = E_UNEXPECTED;

        GlobalUnlock ( stgmed.hGlobal );
        ReleaseStgMedium ( &stgmed );
    }
    return hr;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FILE_Extract_path 
/*--------------------------------------------------------------------------*/
//           Type: const TCHAR * 
//    Param.    1: const TCHAR * src: 
//    Param.    2: BOOL last_bslash : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
const TCHAR * FILE_Extract_path ( const TCHAR * src, BOOL last_bslash )
/*--------------------------------------------------------------------------*/
{
    DWORD           idx;
    static TCHAR    temp[MAX_PATH+1];

    if ( src == NULL ) return NULL;
    idx = lstrlen ( src )-1;
    if ( idx >= MAX_PATH ) return NULL;
    while ( ( src[idx] != TEXT('\\') ) && ( idx != 0 ) )
        idx--;
    if ( idx == 0 ) return NULL;
    if ( last_bslash )
        idx++;
    lstrcpyn ( temp, src, idx+1 );
    return temp;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: GetCurrentModule 
/*--------------------------------------------------------------------------*/
//           Type: HMODULE 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: GetModula Handle(NULL) does not work in DLL's ;-)
/*--------------------------------------------------------------------@@-@@-*/
HMODULE GetCurrentModule ( void )
/*--------------------------------------------------------------------------*/
{
    HMODULE hModule = NULL;

    GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, 
            (LPCTSTR)GetCurrentModule, &hModule);

    return hModule;
}
