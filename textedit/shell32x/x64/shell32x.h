// shell32x.h
// Include for some of the undoc functions from shell32.dll, exported by ordinal only 

#ifndef _SHELL32X_H_
#define _SHELL32X_H_

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>

// flags for RunFileDlg
#define     RFF_NOBROWSE        0x00000001
#define     RFF_NODEFAULT       0x00000002
#define     RFF_CALCDIRECTORY   0x00000004
#define     RFF_NOLABEL         0x00000008
#define     RFF_NOSEPARATEMEM   0x00000020

// flags for ShellExecCmdLine
#define     SECL_NO_UI          0x2
#define     SECL_LOG_USAGE      0x8
#define     SECL_USE_IDLIST     0x10
#define     SECL_RUNAS          0x40

// flags for Int64ToString
#define     FMT_USE_NUMDIGITS   0x1
#define     FMT_USE_LEADZERO    0x2
#define     FMT_USE_GROUPING    0x4
#define     FMT_USE_DECIMAL     0x8
#define     FMT_USE_THOUSAND    0x10
#define     FMT_USE_NEGNUMBER   0x20

#ifdef __cplusplus
extern "C" {
#endif

/*
RunFileDlg
The "Start->Run" dialog
*/
void WINAPI RunFileDlg ( HWND hwndOwner, HICON hIcon, TCHAR * lpstrDirectory, TCHAR * lpstrTitle, TCHAR * lpstrDescription, UINT uFlags );

/*
ShutDownDlg
Shut down Windows dialog
*/
void WINAPI ShutDownDlg ( HWND hwndOwner );

/*
ShellExecCmdLine
Parameters

    hwnd
        Message box owner window if any are produced by ShellExecuteEx
    pwszCommand
        The command to run. Can be a URL or anything else accepted by ShellExecute
    pwszStartDir
        The initial directory for any new process created
    nShow
        A SW_ value such as SW_SHOW to be passed to ShellExecuteEx
    pUnused
        Unused
    dwSeclFlags
        A set of bitflags. This can be zero or a combination of the following:

        SECL_NO_UI (0x2)
            Don't display error messages if they occur. Adds SEE_MASK_FLAG_NO_UI to ShellExecuteInfo's dwFlags
        SECL_LOG_USAGE (0x8)
            Keep track of the number of times the application has been launched. Adds SEE_MASK_FLAG_LOG_USAGE to ShellExecuteInfo's dwFlags
        SECL_USE_IDLIST (0x10)
            Allow IContextMenu verbs instead of registry verbs. Adds SEE_MASK_INVOKEIDLIST to ShellExecuteInfo's dwFlags
        SECL_RUNAS (0x40)
            Use the RunAs verb with ShellExecuteEx if AssocQueryString(0, ASSOCSTR_COMMAND, pwszCommand, L"RunAs", NULL, &strSize) doesn't fail and strSize is non-zero on return.

Return Value

    S_OK on success or HRESULT_FROM_WIN32(GetLastError()) if ShellExecuteEx fails

Remarks

    ShellExecuteInfo.dwMask defaults to SEE_MASK_DOENVSUBST | SEE_MASK_NOASYNC before any modification is made according to the value of dwSeclFlags

    This function is used to execute the contents of the Run dialog box.

    This function is also known as _ShellExecCmdLine@24.
*/
HRESULT WINAPI ShellExecCmdLine ( HWND hwnd, LPCWSTR pwszCommand, LPCWSTR pwszStartDir, int nShow, LPVOID pUnused, DWORD dwSeclFlags );

/*
Int64ToString
Syntax

    int WINAPI Int64ToString (
        INT64 number,
        LPWSTR pwszBuf,
        UINT bufLen,
        BOOL useNumberFormat,
        NUMBERFMT* pFormatInfo,
        DWORD dwFlags
    ) 

Parameters

    number
        The number to format
    pwszBuf
        A pointer to the converted string
    bufLen
        The size of pwszBuf in WCHARs
    useNumberFormat
        A flag specifying whether to format the number
    pFormatInfo
        Structure describing the formatting to apply to the number
    dwFlags
        Bit flag specifying which members of pFormatInfo to use

Return Value

    Number of WCHARs written to pszBuf, not including the terminating nul

Remarks

    The NUMBERFMT structure is defined in the SDK.

    The valid values for dwFlags are:

    #define FMT_USE_NUMDIGITS 0x1
    #define FMT_USE_LEADZERO  0x2
    #define FMT_USE_GROUPING  0x4
    #define FMT_USE_DECIMAL   0x8
    #define FMT_USE_THOUSAND  0x10
    #define FMT_USE_NEGNUMBER 0x20

    If useNumberFormat is FALSE, the function converts number to a string and returns. Otherwise, if pFormatInfo is not NULL, the function applies the formatting in pFormatInfo according to which flags are present (i.e. pFormatInfo->NumDigits will only be used if the FMT_USE_NUMDIGITS flag is set). For those flags which aren't set, the corresponding formatting comes from GetLocaleInfo.

    If pFormatInfo is NULL, the function behaves as if dwFlags was 0 and all formatting information comes from GetLocaleInfo.

    Note, due to a bug in the function, if pFormatInfo is NULL, useNumberFormat is nonzero, and the digit grouping in the user's locale setting is anything other than "123,456,789" in control panel (that's GetLocaleInfo(LOCALE_SGROUPING) returning a string other than '3;0'), then the grouping will not be properly applied, with only the digit before the semi-colon being taken into account.

    This function is also known as _Int64ToString@28.
*/
int WINAPI Int64ToString ( INT64 number, LPWSTR pwszBuf, UINT bufLen, BOOL useNumberFormat, NUMBERFMT * pFormatInfo, DWORD dwFlags );


#ifdef __cplusplus
}
#endif

#endif // _SHELL32X_H_

