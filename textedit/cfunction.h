#ifndef _CFUNCTION_H
#define _CFUNCTION_H

#include <windows.h>

#define         CF_MAX_LEN             25000
#define         CF_MAX_ALLOCSIZE       0x00100000 // cca 1MB
#define         CF_MAX_LINE            128
#define         CF_MAX_PARAMS          127
#define         CF_MAX_MODIFS          127

typedef
struct tag_cfunction
{
    TCHAR   * params[CF_MAX_PARAMS];    // param. list
    TCHAR   * modifs[CF_MAX_MODIFS];    // return type, modifiers, fn name
    TCHAR   * fn_name;                  // function name, post CF_StripCrap :)
    INT_PTR n_par;                      // # of params
    INT_PTR n_mod;                      // # of modifiers
    INT_PTR par_maxw;                   // strlen of the longest param.
    INT_PTR mod_maxw;                   // strlen of the longest modif.
    BOOL    is_decl;                    // is fn. declaration, or definition
    BOOL    is_declspec;                // is there a declspec(...) present?
} CFUNCTION;

typedef BOOL ( CALLBACK * CFSEARCHPROC ) 
    ( BOOL valid/*a good line?*/, 
    DWORD/*lines*/, 
    DWORD/*crtline*/, 
    WPARAM wParam, 
    LPARAM lParam/*user_def*/ );

extern BOOL CF_GetUserSelection 
    ( HWND hRich, TCHAR * srcline, size_t cchmax );

extern BOOL CF_CommentCFunction 
    ( HWND hRich, const TCHAR * csrc );

extern BOOL CF_ParseCFunction 
    ( const TCHAR * csrc, CFUNCTION * cf );

extern INT_PTR CF_FindCFDefinitions 
    ( HWND hRich, int startl, int endl, 
    CFSEARCHPROC searchproc, LPARAM lParam );

extern INT_PTR CF_FindCFDeclarations 
    ( HWND hRich, int startl, int endl, 
    CFSEARCHPROC searchproc, LPARAM lParam );

#endif
