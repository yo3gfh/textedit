
// cfunction.c - module with C parsing related functions
// quite a mess, really :-) should be rewritten to read whole
// file into memory, then parse it. 

//enum not used in switch, = used in conditional, deprecated
#pragma warn(disable: 2008 2118 2228 2231 2030 2260) 

#include "main.h"
#include "rich.h"
#include "cfunction.h"
#include "config.h"
#include "misc.h"
#include "mem.h"
#include <tchar.h>
#include <strsafe.h>
#include <richedit.h>

static INT_PTR CF_GetLinesDecl ( HWND hRich, DWORD line, 
    TCHAR * dest, WORD cchMax );

static INT_PTR CF_GetLinesDef ( HWND hRich, DWORD line, 
    TCHAR * dest, WORD cchMax );

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_GetUserSelection 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich     : richedit control
//    Param.    2: TCHAR * srcline: buffer to receive line of text
//    Param.    3: size_t cchMax  : buffer max size
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Copy cchMax chars of text selection or line under caret
//                 to srcline. On error, returns FALSE and srcline[0] is
//                 set to null. If cchMax is too small, returns FALSE 
//                 and srcline[0] is set to null.
/*--------------------------------------------------------------------@@-@@-*/
BOOL CF_GetUserSelection ( HWND hRich, TCHAR * srcline, size_t cchMax )
/*--------------------------------------------------------------------------*/
{
    CHARRANGE       chr;
    DWORD           line;

    if ( srcline == NULL || cchMax == 0 )
        return FALSE;

    srcline[0] = TEXT('\0');
    
    Rich_GetSel ( hRich, &chr );

    if ( chr.cpMax > chr.cpMin )
    {
        if ( (size_t)(chr.cpMax - chr.cpMin) > cchMax )
            return FALSE;

        Rich_GetSelText ( hRich, srcline, cchMax );
    }
    else
    {
        line = Rich_GetCrtLine ( hRich );
        Rich_GetLineText ( hRich, line, srcline, (WORD)cchMax );
    }

    return ( srcline[0] != TEXT('\0') );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_IsEndl 
/*--------------------------------------------------------------------------*/
//           Type: static BOOL 
//    Param.    1: TCHAR tc : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Is tc a line terminator? (CR/LF)
/*--------------------------------------------------------------------@@-@@-*/
static BOOL CF_IsEndl ( TCHAR tc )
/*--------------------------------------------------------------------------*/
{
    return ( tc == TEXT('\r') || tc == TEXT('\n') );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_IsWhite 
/*--------------------------------------------------------------------------*/
//           Type: static BOOL 
//    Param.    1: TCHAR tc : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Is tc a whitespace?
/*--------------------------------------------------------------------@@-@@-*/
static BOOL CF_IsWhite ( TCHAR tc )
/*--------------------------------------------------------------------------*/
{
    return ( tc == TEXT(' ') || tc == TEXT('\t') );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_IsCFunction 
/*--------------------------------------------------------------------------*/
//           Type: static BOOL 
//    Param.    1: const TCHAR * ws: our candidate
//    Param.    2: CFUNCTION * cf  : pointer to CFUNCTION to receive data
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: some crude attempt to assess if the text we got is a C
//                 function or not :-) - basically, it needs 1 pair of ()
//                 or 2 pairs if declspec is present, also checks if it's
//                 inside quotes, on a comment line or in a if/else/switch/
//                 block and fails accordingly. It can be easily fooled and
//                 it's not designed to be standalone, but rather as a 
//                 pre-stage for CF_ParseCFunction
/*--------------------------------------------------------------------@@-@@-*/
static BOOL CF_IsCFunction ( const TCHAR * ws, CFUNCTION * cf )
/*--------------------------------------------------------------------------*/
{
    INT_PTR i, left, right, len;

    BOOL        is_declsp;
    TCHAR       * quot, * comma, 
                * s, * semi_c;
    TCHAR       temp[CF_MAX_LEN];

    left        = 0;
    right       = 0;
    is_declsp   = FALSE;

    quot        = _tcschr ( ws, TEXT('"') ); // any quotes?

    if ( quot != NULL )
        return FALSE;

    quot        = _tcschr ( ws, TEXT('=') );

    if ( quot != NULL )
        return FALSE;

    quot        = _tcschr ( ws, TEXT('+') );

    if ( quot != NULL )
        return FALSE;

    quot        = _tcschr ( ws, TEXT('-') );

    if ( quot != NULL )
        return FALSE;

    quot        = _tcschr ( ws, TEXT('?') );

    if ( quot != NULL )
        return FALSE;

    quot        = _tcschr ( ws, TEXT('@') );

    if ( quot != NULL )
        return FALSE;

    quot        = _tcschr ( ws, TEXT('.') );

    if ( quot != NULL )
        return FALSE;

    quot        = _tcschr ( ws, TEXT('!') );

    if ( quot != NULL )
        return FALSE;

    quot        = _tcschr ( ws, TEXT('|') );

    if ( quot != NULL )
        return FALSE;

    quot        = _tcschr ( ws, TEXT(':') );

    if ( quot != NULL )
        return FALSE;

    quot        = _tcschr ( ws, TEXT('&') );

    if ( quot != NULL )
        return FALSE;

    semi_c      = _tcschr ( ws, TEXT(';'));
    comma       = _tcschr ( ws, TEXT(',') );

    // make a copy for _tcstok 
    StringCchCopy ( temp, ARRAYSIZE(temp), ws );

    s =         _tcstok ( temp, TEXT(" ()[]{},;") );

    if ( s != NULL )
    {
        if ( ( _tcscmp ( s, TEXT("_declspec"))  == 0 )  ||
             ( _tcscmp ( s, TEXT("__declspec")) == 0 ) )
                is_declsp = TRUE;

        if ( ( _tcscmp ( s, TEXT("if"))         == 0 )  ||
             ( _tcscmp ( s, TEXT("else"))       == 0 )  ||
             ( _tcscmp ( s, TEXT("while"))      == 0 )  ||
             ( _tcscmp ( s, TEXT("for"))        == 0 )  ||
             ( _tcscmp ( s, TEXT("switch"))     == 0 )  ||
             ( _tcscmp ( s, TEXT("case"))       == 0 )  ||
             ( _tcscmp ( s, TEXT("default"))    == 0 )  ||
             ( _tcscmp ( s, TEXT("__except"))   == 0 )  ||
             ( _tcscmp ( s, TEXT("defined"))    == 0 )  ||
             ( _tcscmp ( s, TEXT("sizeof"))     == 0 )  ||
             ( _tcscmp ( s, TEXT("assert"))     == 0 )  ||
             ( _tcscmp ( s, TEXT("return"))     == 0 ) )
                return FALSE;

        while ( (s = _tcstok ( NULL,  TEXT(" ()[]{},;") )) != NULL )
        {
            if ( ( _tcscmp ( s, TEXT("_declspec"))  == 0 )  ||
                 ( _tcscmp ( s, TEXT("__declspec")) == 0 ) )
            {
                is_declsp = TRUE;
                continue;
            }

            if ( ( _tcscmp ( s, TEXT("if"))         == 0 )  ||
                 ( _tcscmp ( s, TEXT("else"))       == 0 )  ||
                 ( _tcscmp ( s, TEXT("while"))      == 0 )  ||
                 ( _tcscmp ( s, TEXT("for"))        == 0 )  ||
                 ( _tcscmp ( s, TEXT("switch"))     == 0 )  ||
                 ( _tcscmp ( s, TEXT("case"))       == 0 )  ||
                 ( _tcscmp ( s, TEXT("default"))    == 0 )  ||
                 ( _tcscmp ( s, TEXT("__except"))   == 0 )  ||
                 ( _tcscmp ( s, TEXT("defined"))    == 0 )  ||
                 ( _tcscmp ( s, TEXT("sizeof"))     == 0 )  ||
                 ( _tcscmp ( s, TEXT("assert"))     == 0 )  ||
                 ( _tcscmp ( s, TEXT("return"))     == 0 ) )
                    return FALSE;
        }
    }

    len     = lstrlen ( ws );
    i       = len;

    while ( i >= 0)
    {
        if ( ws[i] == TEXT('(') )
        {
            left++;

            if ( semi_c != NULL )
                if ( semi_c < ws+i )
                    return FALSE;

            if ( comma != NULL )
                if ( comma < ws+i )
                    return FALSE; // shouldn't have a comma before '('
        }

        if ( ws[i] == TEXT(')') )
        {
            right++;

            if ( semi_c != NULL )
                if ( semi_c < ws+i )
                    return FALSE;

            if ( comma != NULL )
                if ( comma > ws+i )
                    return FALSE;   // we've hit some multi-line
                                    //  function call so get out
        }

        i--;
    }

    if ( semi_c != NULL )
    {
        //cf->is_decl = ( ws+(len-1) == semi_c );
        cf->is_decl = TRUE;
    }
    else
        cf->is_decl = FALSE;

    cf->is_declspec = is_declsp;

    if ( is_declsp )
    {
        if ( (left == 2) && (right == 2) )
            return TRUE;
        else
            return FALSE;
    }

    if ( (left == 1) && (right == 1) )
        return TRUE;

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_StripCrap 
/*--------------------------------------------------------------------------*/
//           Type: static TCHAR * 
//    Param.    1: const TCHAR * wsrc: raw text validated by IsCFunction
//    Param.    2: TCHAR * wdest     : where to put cleaned text
//    Param.    3: int cchMax        : max. destination, in characters
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Strips all CR/LF, extra tabs/spaces, comments, etc. 
//                 from wsrc. NULL terminates wdest.
/*--------------------------------------------------------------------@@-@@-*/
static TCHAR * CF_StripCrap ( const TCHAR * wsrc, TCHAR * wdest, int cchMax )
/*--------------------------------------------------------------------------*/
{
    INT_PTR i, len, start;
    TCHAR   * p, * s;
    BOOL    skipping = TRUE;
    BOOL    cr_skipping = TRUE;
    BOOL    in_comment = FALSE;
    BOOL    in_macro = FALSE;
    BOOL    block_comment = FALSE;

    p       = wdest;
    len     = lstrlen ( wsrc );
    start   = 0;

    if ( len > cchMax )
        len = cchMax;

    s = _tcschr ( wsrc, TEXT(';') );

    // see how many ';' we have and skip
    // everything left of them, except the last
    while ( s != NULL )
    {
        s++;

        while ( CF_IsWhite(*s) || CF_IsEndl(*s) )
            s++;

        if ( (*s == TEXT('\0')) /*|| 
            (s == _tcsstr ( wsrc, TEXT("//"))) || 
            (s == _tcsstr ( wsrc, TEXT("/*"))) */)
            break;

        start = (s - wsrc);
        
        s = _tcschr ( wsrc+start, TEXT(';') );
    }

    // start where last ';' found
    for ( i = start; i < len; i++ )
    {
        if ( block_comment )
        {
            if ( (i+1 < len) && (wsrc[i] == TEXT('*')) && 
                (wsrc[i+1] == TEXT('/')))
            {
                i++;
                block_comment = FALSE;
            }

            continue;
        }

        if ( in_comment )
        {
            if ( CF_IsEndl ( wsrc[i]) )
                in_comment = FALSE;

            continue;
        }

        if ( in_macro )
        {
            if ( CF_IsEndl ( wsrc[i]) )
                in_macro = FALSE;

            continue;
        }

        if ( (i+1 < len) && (wsrc[i] == TEXT('/')) && 
            (wsrc[i+1] == TEXT('/')))
        {
            i++;
            in_comment = TRUE;
            continue;
        }

        if ( (i+1 < len) && (wsrc[i] == TEXT('/')) && 
            (wsrc[i+1] == TEXT('*')))
        {
            i++;
            block_comment = TRUE;
            continue;
        }

        if ( wsrc[i] == TEXT('#'))
        {
            in_macro = TRUE;
            continue;
        }

        // try to accomodate MS style
        // multiline function, gobble one
        // cr and make it a space
        if ( CF_IsEndl ( wsrc[i]) )
        {
            if ( cr_skipping == FALSE )
            {
                *p++ = TEXT(' ');
                cr_skipping = TRUE;
            }
            continue;
        }

        if ( CF_IsWhite(wsrc[i]) )
        {
            if ( skipping == FALSE )
            {
                *p++ = wsrc[i]; // keep at least one tab/space
                skipping = TRUE;
            }
            continue;
        }
        else
        {
            *p++ = wsrc[i];
            skipping = FALSE;
            cr_skipping = FALSE;
        }
    }

    *p = TEXT('\0');
    return wdest;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_ParseCFunction 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: const TCHAR * csrc: const TCHAR * csrc: raw text, as
//                 returned by CF_GetUserSelection
//    Param.    2: CFUNCTION * cf    : pointer to a CFUNCTION to receive
//                 function data
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: This will take a block of text, validate it as a C
//                 function, then parse it to extract params and modifiers
//                 and to put them as pointers to null-term. strings in the
//                 cf->params ans cf->modifs. Will also fill other members
//                 of CFUNCTION with meaningful info. Returns TRUE if it
//                 thinks it has a valid C function decl./def. or FALSE
//                 otherwise; it can return FALSE even if CF_IsCFunction
//                 said OK - this one checks for a function name and one
//                 param at least.
/*--------------------------------------------------------------------@@-@@-*/
BOOL CF_ParseCFunction ( const TCHAR * csrc, CFUNCTION * cf )
/*--------------------------------------------------------------------------*/
{
    static TCHAR    work_buf[CF_MAX_LEN];
    static TCHAR    next_buf[CF_MAX_LEN];
    static TCHAR    fun_name[CF_MAX_LEN];
    TCHAR           * pos, * p;
    TCHAR           * par_start;
    TCHAR           * mod_start;
    INT_PTR         i, j, len;
    BOOL            is_declsp;

    RtlZeroMemory ( cf, sizeof(CFUNCTION) );
    RtlZeroMemory ( work_buf, sizeof(work_buf) );
    RtlZeroMemory ( next_buf, sizeof(next_buf) );

    // stage 1: clean source from CR/LF, tabs, etc.
    // rough pre-evaluation :-)
    pos             = CF_StripCrap ( csrc, work_buf, ARRAYSIZE(work_buf) );

    if ( !CF_IsCFunction ( pos, cf ) )
        return FALSE;

    // a small hack, clean fn name from any left
    // right curls; could do it in the CF_StripCrap,
    // but the function synopsis won't work correctly.
    // a very good example of bad design :-)
    p = _tcschr ( pos, TEXT('}'));

    if ( p != NULL )
    {
        p++;

        while (((*p == TEXT(' '))   || 
                (*p == TEXT('}')))  && 
                (*p != TEXT('\0')))
            p++;

        pos = p;
    }

    StringCchCopy ( fun_name, ARRAYSIZE(fun_name), pos );
    i               = lstrlen ( fun_name );

    // we could combine this with above loop
    // we could :)
    while ( i >= 0 )
    {
        if ( (fun_name[i] == TEXT(')')) )
            break;

        i--;
    }

    fun_name[i+1]   = TEXT('\0');
    cf->fn_name     = fun_name;

    mod_start       = pos;
    par_start       = pos; // just in case
    i               = 0;
    len             = lstrlen ( pos );
    is_declsp       = cf->is_declspec;
    
    // stage 2: put a null where '(' is; this will be the boundary between 
    // fn. name+modifiers and parameter list; also put a null on ')', since
    // we don't need anything after that
    // if we have declspec(...) replace () with spaces and redo stage 1
    for ( j = 0; j < len; j++ )
    {
        if ( pos[j] == TEXT('(') )
        {
            if ( is_declsp )
                pos[j++] = TEXT(' ');
            else
            {
                pos[j++] = TEXT('\0');
                par_start = pos + j;
            }
            continue;
        }

        if ( pos[j] == TEXT(')') )
        {
            if ( is_declsp ) // redo stage 1
            {
                pos[j] = TEXT(' ');
                is_declsp = FALSE;
                pos = CF_StripCrap ( work_buf, next_buf, 
                    ARRAYSIZE(next_buf) );
                mod_start = pos;
                par_start = pos;
                j = 0;
                continue;
            }
            else
                pos[j] = TEXT('\0');
        }
    }

    pos             = mod_start;
    len             = lstrlen ( mod_start );

    // stage 3: parse modifiers and replace every space with a null;
    // set the modifiers pointer table from the CFUNCTION struct
    for ( j = 0; j < len; j++ )
    {
        if ( CF_IsWhite(pos[j]) )
        {
            pos[j++] = TEXT('\0');
            cf->modifs[i++] = mod_start;
            mod_start = pos + j;
        }
        else
        {
            if (pos[j+1] == TEXT('\0'))
                cf->modifs[i++] = mod_start;
        }

        // we have a glorious function with 127 modifiers...
        if ( i >= CF_MAX_MODIFS-1 ) 
            break;
    }

    cf->modifs[i]   = 0;
    cf->n_mod       = i;

    if ( i == 0 ) 
        return FALSE; // not ok with 0 modifs. (no function name, basically)

    pos             = par_start;
    i               = 0;
    len             = lstrlen ( par_start );

    // stage 4: parse parameter list and replace every ',' with a null;
    // set the params pointer table from the CFUNCTION struct
    for ( j = 0; j < len; j++ )
    {
        if (pos[j] == TEXT(','))
        {
            pos[j++] = TEXT('\0');

            // skip leading space, if any
            while ( CF_IsWhite(*par_start) && *par_start != TEXT('\0') ) 
                par_start++;

            cf->params[i++] = par_start;
            par_start = pos + j;
        }
        else
        {
            if (pos[j+1] == TEXT('\0'))
            {
                while ( CF_IsWhite(*par_start) && *par_start != TEXT('\0') )
                    par_start++;

                cf->params[i++] = par_start;
            }
        }

        // ...or another glorious function with 127 params :-)
        if ( i >= CF_MAX_PARAMS-1 ) 
            break;
    }
    
    cf->params[i]   = 0; 
    cf->n_par       = i;

    if ( i == 0 )
        return FALSE; // not ok with 0 params

    // save the longest param/mod for later 
    for ( j = 0; j < cf->n_mod; j++ )
    {
        len = lstrlen ( cf->modifs[j] );

        if ( cf->mod_maxw < len )
            cf->mod_maxw = len;
    }

    for ( j = 0; j < cf->n_par; j++ )
    {
        len = lstrlen ( cf->params[j] );

        if ( cf->par_maxw < len )
            cf->par_maxw = len;
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_FindCFDefinitions 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR 
//    Param.    1: HWND hRich             : editor to search for fn.
//                                          definitions
//    Param.    2: int startl             : startig line to seek from..
//    Param.    3: int endl               : .. until here
//    Param.    4: CFSEARCHPROC searchproc: callback to exec on each
//                                          processed line
//    Param.    5: LPARAM lParam          : extra param to pass to searchproc
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Parses a richedit control lines for C function definitions
//                 and calls searchproc for each one found. On success, 
//                 returns total # of chars written, on failure returns -1
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CF_FindCFDefinitions ( HWND hRich, int startl, int endl,
                                CFSEARCHPROC searchproc, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int         i, len, lines;
    INT_PTR     size;
    TCHAR       line[CF_MAX_LEN];
    CFUNCTION   cf;
    BOOL        in_comment;

    lines = Rich_GetLineCount ( hRich );

    if ( endl >= lines )
        endl = lines-1;

    if ( startl > endl )
        return -1;

    size = 0;
    len  = 0;

    in_comment  = FALSE;
    lines       = (endl - startl)+1;

    for ( i = startl; i <= endl; i++ )
    {
        if ( !searchproc ( FALSE, lines, i+1, (WPARAM)NULL, lParam ) )
            break;

        i += (int)CF_GetLinesDef ( hRich, i, line, ARRAYSIZE(line) );

        if ( CF_ParseCFunction ( line, &cf ) )
        {
            if ( cf.is_decl )
                continue;

            // can't be a def. with function name only (or no function name)
            if ( cf.n_mod == 1 ) 
                continue;

            StringCchCopy ( line, ARRAYSIZE(line), cf.fn_name );
            len = lstrlen (cf.fn_name);
            StringCchCat ( line, ARRAYSIZE(line), TEXT(";") );
            
            if (len < (int)(ARRAYSIZE(line)-1))
                len += 1;

            if ( !searchproc ( TRUE, lines, i+1, (WPARAM)line, lParam ) )
            {
                size += len;
                break;
            }

            size += len;
        }
    }

    return size;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_FindCFDeclarations 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR 
//    Param.    1: HWND hRich             : editor to search for fn.
//                                          declarations
//    Param.    2: int startl             : startig line to seek from..
//    Param.    3: int endl               : .. until here
//    Param.    4: CFSEARCHPROC searchproc: callback to exec on each
//                                          processed line
//    Param.    5: LPARAM lParam          : extra param to pass to searchproc
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Parses a richedit control lines for C function declarations
//                 and calls searchproc for each one found. On success,
//                 returns total # of chars written, on failure returns -1
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CF_FindCFDeclarations ( HWND hRich, int startl, int endl,
                                CFSEARCHPROC searchproc, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    int         i, j, len, lines;
    INT_PTR     size;
    TCHAR       line[CF_MAX_LEN];
    CFUNCTION   cf;
    BOOL        in_comment;

    lines       = Rich_GetLineCount ( hRich );

    if ( endl >= lines )
        endl = lines-1;

    if ( startl > endl )
        return -1;

    size        = 0;
    len         = 0;
    j           = 0;

    in_comment  = FALSE;
    lines       = (endl - startl)+1;

    for ( i = startl; i <= endl; i++ )
    {
        if ( !searchproc ( FALSE, lines, i+1, (WPARAM)NULL, lParam ) )
            break;
        
        i += (int)CF_GetLinesDecl ( hRich, i, line, ARRAYSIZE(line) );

        if ( CF_ParseCFunction ( line, &cf ) )
        {
            if ( !cf.is_decl ) // skip, not a declaration
                continue;

            // can't be a decl. with function name only (or no function name)
            if ( cf.n_mod == 1 ) 
                continue;

            StringCchCopy ( line, ARRAYSIZE(line), cf.fn_name );
            len = lstrlen (cf.fn_name);
            StringCchCat ( line, ARRAYSIZE(line), TEXT(";") );

            if ( len < (int)(ARRAYSIZE(line)-1) )
                len += 1;

            if ( !searchproc ( TRUE, lines, i+1, (WPARAM)line, lParam ) )
            {
                size += len;
                break;
            }

            size += len;
        }
    }

    return size;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_GetCrtSourceLine 
/*--------------------------------------------------------------------------*/
//           Type: static DWORD 
//    Param.    1: HWND hRich : HWND to a richedit control
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: This is in fact a modified Rich_GetCrtLine, so that in case
//                 of a selection, it returns the line with sel. START
/*--------------------------------------------------------------------@@-@@-*/
static DWORD CF_GetCrtSourceLine ( HWND hRich )
/*--------------------------------------------------------------------------*/
{
    CHARRANGE       chr;

    Rich_GetSel ( hRich, &chr );
    return Rich_LineFromChar ( hRich, chr.cpMin );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_SourceCopyFunctionInfo 
/*--------------------------------------------------------------------------*/
//           Type: static BOOL 
//    Param.    1: HWND hRich        : handle to source window
//    Param.    2: BOOL is_decl      : it's a function declaration?
//    Param.    3: const TCHAR * buf : formatted text
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Insert function info above function definition/declaration;
//                 if we have a definition, also insert a comment bar just
//                 above the opening '{'
/*--------------------------------------------------------------------@@-@@-*/
static BOOL CF_SourceCopyFunctionInfo ( HWND hRich, BOOL is_decl,
                                        const TCHAR * buf )
/*--------------------------------------------------------------------------*/
{
    CHARRANGE       chr;
    INT_PTR         retries, lcount;
    DWORD           line;
    TCHAR           temp[CF_MAX_LINE];

    if ( buf == NULL )
        return FALSE;

    if ( *buf == TEXT('\0') )
        return FALSE;

    // get current line and go to line start
    line = CF_GetCrtSourceLine ( hRich );

    // give up if already commented 
    if ( line > 0 )
    {
        Rich_GetLineText ( hRich, (DWORD)line-1, temp, ARRAYSIZE(temp) );
        if ( _tcsstr ( temp, TEXT("@@-@@") ) != NULL )
            return FALSE;
    }

    chr.cpMin = chr.cpMax = Rich_LineIndex ( hRich, line );

    // move caret and insert formatted text
    Rich_SetSel ( hRich, &chr );
    Rich_ReplaceSelText ( hRich, buf, TRUE );

    // no use in adding body separator since we have just 
    // a function declaration
    // so we get out :-)
    if ( is_decl )
        return TRUE;

    // update line idx, seek for first curly for a max. of
    // 30 lines (or file end,
    // whichever comes first
    line = CF_GetCrtSourceLine ( hRich );
    line++;
    retries = 0;
    lcount = Rich_GetLineCount ( hRich );

    do
    {
        Rich_GetLineText ( hRich, (DWORD)line, temp, ARRAYSIZE(temp) );

        if ( _tcschr ( temp, TEXT('{') ) != NULL )
        {
            chr.cpMax = chr.cpMin = Rich_LineIndex ( hRich, line );
            Rich_SetSel ( hRich, &chr );
            Rich_ReplaceSelText ( hRich,
                TEXT("/*---------------------------------------"
                    "-----------------------------------*/\n"), TRUE );
            break;
        }
        line++;
        retries++;
    } 
    while ( retries < 31 && line < lcount );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_CopyFunctionInfo 
/*--------------------------------------------------------------------------*/
//           Type: static BOOL 
//    Param.    1: HWND hRich           : the source window
//    Param.    2: const CFUNCTION * cf : addr. of a CFUNCTION struct,
//                                        filled by CF_ParseCFunction
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: take a CFUNCTION *, allocate mem. , make it into nice
//                 formatted text and shove it around the current function
//                 declaration/definition by calling CF_SourceCopyFunctionInfo
/*--------------------------------------------------------------------@@-@@-*/
static BOOL CF_CopyFunctionInfo ( HWND hRich, const CFUNCTION * cf )
/*--------------------------------------------------------------------------*/
{
    TCHAR       * ini, * buf, line[CF_MAX_LINE*2];
    HGLOBAL     hMem;
    INT_PTR     buf_size, alloc_size;
    INT_PTR     i;

    if ( cf == NULL )
        return FALSE;

    if ( cf->n_mod == 0 || cf->n_par == 0 )
        return FALSE;

    // n_mod lines for modifiers, n_par lines for param list,
    // 3 lines header+footer, 
    // 3 for author, date, and description - 128 chars each
    buf_size    = ((cf->n_mod + cf->n_par + 6) * CF_MAX_LINE);
    buf_size    *= sizeof(TCHAR);
    alloc_size  = ALIGN_4K(buf_size); // allocate in 4k chunks
    
    if ( alloc_size > CF_MAX_ALLOCSIZE )
        return FALSE;

    hMem = GlobalAlloc ( GHND, alloc_size );

    if ( hMem == NULL )
        return FALSE;

    ini         = IniFromModule ( GetModuleHandle ( NULL ) );
    buf         = (TCHAR *)GlobalLock ( hMem );

    StringCchPrintf ( buf, buf_size,
                    TEXT("/*-@@+@@--------------------------------------"
                        "------------------------------*/\n") );
    #ifdef UNICODE
    StringCchPrintf ( line, ARRAYSIZE(line),
                    TEXT("//       Function: %ls \n"), 
                    cf->modifs[cf->n_mod-1] );
    #else
    StringCchPrintf ( line, ARRAYSIZE(line),
                    TEXT("//       Function: %s \n"),
                    cf->modifs[cf->n_mod-1] );
    #endif
    StringCchCat ( buf, buf_size, line );
    StringCchPrintf ( line, ARRAYSIZE(line),
                    TEXT("/*--------------------------------------------"
                        "------------------------------*/\n") );
    StringCchCat ( buf, buf_size, line );
    StringCchPrintf ( line, ARRAYSIZE(line),
                    TEXT("//           Type: ") );

    StringCchCat ( buf, buf_size, line );

    for ( i = 0; i < cf->n_mod-1; i++ )
    {
    #ifdef UNICODE
        StringCchPrintf ( line, ARRAYSIZE(line), TEXT("%ls "), 
            cf->modifs[i] );
    #else
        StringCchPrintf ( line, ARRAYSIZE(line), TEXT("%s "), 
            cf->modifs[i] );
    #endif
        StringCchCat ( buf, buf_size, line );
    }

    StringCchPrintf ( line, ARRAYSIZE(line),     TEXT("\n") );
    StringCchCat ( buf, buf_size, line );

    for ( i = 0; i < cf->n_par; i++ )
    {
    #ifdef UNICODE
        StringCchPrintf ( line, ARRAYSIZE(line),
                        TEXT("//    Param. %4zd: %-*ls: \n"), i+1,
                        (int)cf->par_maxw, cf->params[i] );
    #else
        StringCchPrintf ( line, ARRAYSIZE(line),
                        TEXT("//    Param. %4zd: %-*s: \n"), i+1,
                        (int)cf->par_maxw, cf->params[i] );
    #endif
        StringCchCat ( buf, buf_size, line );
    }

    StringCchPrintf ( line, ARRAYSIZE(line),
                        TEXT("/*----------------------------------------"
                        "----------------------------------*/\n") );
    StringCchCat ( buf, buf_size, line );
    #ifdef UNICODE
    StringCchPrintf ( line, ARRAYSIZE(line),
                        TEXT("//         AUTHOR: %ls\n//           DATE: "
                            "%ls\n//    DESCRIPTION: <lol>\n//\n"),
                        ReadAuthorFromIni ( ini ), Today() );
    #else
    StringCchPrintf ( line, ARRAYSIZE(line),
                        TEXT("//         AUTHOR: %s\n//           DATE: "
                            "%s\n//    DESCRIPTION: <lol>\n//\n"),
                        ReadAuthorFromIni ( ini ), Today() );
    #endif
    StringCchCat ( buf, buf_size, line );
    StringCchPrintf ( line, ARRAYSIZE(line),
                        TEXT("/*----------------------------------------"
                            "----------------------------@@-@@-*/\n") );
    StringCchCat ( buf, buf_size, line );

    CF_SourceCopyFunctionInfo ( hRich, cf->is_decl, buf );

    GlobalUnlock ( hMem );

    GlobalFree ( hMem );
    return TRUE; // nothing more to do, so close the shop
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_CommentCFunction 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hRich         : source window
//    Param.    2: const TCHAR * csrc : text returned by CF_GetUserSelection
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Wrapper function, and one of the only few exported from
//                 this module :-)
/*--------------------------------------------------------------------@@-@@-*/
BOOL CF_CommentCFunction ( HWND hRich, const TCHAR * csrc )
/*--------------------------------------------------------------------------*/
{
    CFUNCTION   cf;

    if ( !CF_ParseCFunction ( csrc, &cf ) )
        return FALSE;

    return CF_CopyFunctionInfo ( hRich, &cf );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_GetLinesDecl 
/*--------------------------------------------------------------------------*/
//           Type: static INT_PTR 
//    Param.    1: HWND hRich  : richedit
//    Param.    2: DWORD line  : where to start
//    Param.    3: TCHAR * dest: buf to copy to
//    Param.    4: WORD cchMax : buf max size, in chars
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 06.10.2020
//    DESCRIPTION: Copy lines from hRich to dest until ; or {, or
//                 cchMax reached. Used by CF_FindCFDeclarations.
/*--------------------------------------------------------------------@@-@@-*/
static INT_PTR CF_GetLinesDecl ( HWND hRich, DWORD line, 
    TCHAR * dest, WORD cchMax )
/*--------------------------------------------------------------------------*/
{
    INT_PTR     lines, len, i, j, term, curl;
    TCHAR       * comm_start, * comm_end, * lcomm, * p;
    TCHAR       buf[CF_MAX_LEN];
    BOOL        in_quote, in_comm;

    if ( dest == NULL || cchMax == 0 )
        return 0;

    lines       = 0;
    len         = 0;
    term        = 0;
    curl        = 0;
    in_comm     = FALSE;
    dest[0]     = TEXT('\0');

    while ( ((WORD)len) < cchMax )
    {
        in_quote = FALSE;

        if ( Rich_GetLineText ( hRich, line+((DWORD)lines),
                buf, ARRAYSIZE(buf)) == 0 )
        {
            lines++;
            break;
        }

        comm_start  = _tcsstr ( buf, TEXT("/*"));
        comm_end    = _tcsstr ( buf, TEXT("*/"));
        lcomm       = _tcsstr ( buf, TEXT("//"));
        p           = buf;

        // skip starting whitespace
        while ( CF_IsWhite ( *p ) && *p != TEXT('\0') )
            p++;

        if ( comm_start )
        {
            if ( comm_end == NULL )
            {
                in_comm = TRUE;
                lines++;
                continue;
            }
        }

        if ( comm_end )
        {
            if ( comm_start == NULL )
            {
                in_comm = FALSE;
                lines++;
                continue;
            }
        }

        if ( lcomm == p || in_comm == TRUE )
        {
            lines++;
            continue;
        }

        i = lstrlen ( p );
        j = i;

        while ( i >= 0 )
        {
            if ( p[i] == '\'' || p[i] == '\"' )
                in_quote = !in_quote;

            if ( !in_quote )
            {
                if ( (p[i] == TEXT(';')) && ((p+i > comm_end) || 
                    (p+i < comm_start) || (p+i < lcomm)) )
                {
                    term++;
                    break; // we can afford to get out
                }

                if ( (p[i] == TEXT('{')) && ((p+i > comm_end) || 
                    (p+i < comm_start) || (p+i < lcomm)) )
                {
                    curl++;
                    break; // we can afford to get out
                }
            }

            i--;
        }

        StringCchCat ( dest, cchMax-((WORD)len), p );
        len += j;
        lines++;

        if ( term > 0 || curl > 0 )
            break;
    }

    return lines-1;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CF_GetLinesDef 
/*--------------------------------------------------------------------------*/
//           Type: static INT_PTR 
//    Param.    1: HWND hRich  : richedit
//    Param.    2: DWORD line  : where to start
//    Param.    3: TCHAR * dest: buf to copy to
//    Param.    4: WORD cchMax : buf max size, in chars
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 07.10.2020
//    DESCRIPTION: Copy lines from hRich to dest until conds. for a fn. 
//                 definition are met or cchMax reached. Used by 
//                 CF_FindCFDefinitions. 
/*--------------------------------------------------------------------@@-@@-*/
static INT_PTR CF_GetLinesDef ( HWND hRich, DWORD line, 
    TCHAR * dest, WORD cchMax )
/*--------------------------------------------------------------------------*/
{
    INT_PTR     lines, len, i, j, 
                left_curl, right_curl, 
                right_par;
    TCHAR       buf[CF_MAX_LEN];
    TCHAR       * comm_start, * comm_end, 
                * lcomm, *p;
    BOOL        in_quote, in_comm;

    if ( dest == NULL || cchMax == 0 )
        return 0;

    lines       = 0;
    len         = 0;
    left_curl   = 0;
    right_curl  = 0;
    right_par   = 0;
    dest[0]     = TEXT('\0');
    in_comm     = FALSE;

    while ( ((WORD)len) < cchMax )
    {
        in_quote = FALSE;

        if ( Rich_GetLineText ( hRich, line+((DWORD)lines), 
                buf, ARRAYSIZE(buf)) == 0 )
        {
            lines++;
            break;
        }

        comm_start  = _tcsstr ( buf, TEXT("/*"));
        comm_end    = _tcsstr ( buf, TEXT("*/"));
        lcomm       = _tcsstr ( buf, TEXT("//"));
        p           = buf;

        // skip starting whitespace
        while ( CF_IsWhite ( *p ) && *p != TEXT('\0') )
            p++;

        // weak hack, lol
        if ( _tcsstr ( p, TEXT("else")) != NULL )
        {
            lines++;
            continue;
        }

        if ( comm_start )
        {
            if ( comm_end == NULL )
            {
                in_comm = TRUE;
                lines++;
                continue;
            }
        }

        if ( comm_end )
        {
            if ( comm_start == NULL )
            {
                in_comm = FALSE;
                lines++;
                continue;
            }
        }

        if ( lcomm == p || in_comm == TRUE )
        {
            lines++;
            continue;
        }

        i = lstrlen ( p );
        j = i;

        while ( i >= 0 )
        {
            if ( p[i] == '\'' || p[i] == '\"' )
            {
                in_quote = !in_quote;
                i--;
                continue;
            }

            if ( !in_quote )
            {
                if ( (p[i] == TEXT(')')) && ((p+i > comm_end) || 
                    (p+i < comm_start) || (p+i < lcomm)) )
                        right_par++;

                if ( (p[i] == TEXT('{')) && ((p+i > comm_end) || 
                    (p+i < comm_start) || (p+i < lcomm)) )
                        left_curl++;

                if ( (p[i] == TEXT('}')) && ((p+i > comm_end) || 
                    (p+i < comm_start) || (p+i < lcomm)) )
                        right_curl++;
            }

            i--;
        }

        if ( left_curl > 0 )
        {
            if ( left_curl != right_curl && right_par == 0 )
            {
                lines++;
                continue;
            }
        }

        StringCchCat ( dest, cchMax-((WORD)len), p );
        len += j;
        lines++;

        if ( left_curl > 0 )
            break;
    }

    return lines-1;
}
