
// this is included by all modules, to control unicode/ansi build

#ifndef _MAIN_H
#define _MAIN_H

#define         UNICODE
#ifdef          UNICODE
    #define     _UNICODE
    #define     HAVE_MSFTEDIT   //riched ver. 4.1 on msftedit.dll, 
                                // undef to use riched ver. 3.0 on riched20.dll
#endif

#include        <windows.h>

#define         TMAX_PATH       (MAX_PATH+1)*2 //((MAX_PATH+1)*(sizeof(TCHAR)))
#define         HAVE_HTMLHELP   // if you plan to include a helpfile in chm 
                                // format; if you do, don't forget to make
                                // [MAP] and [ALIAS] sections in the helpfile
                                // for all menu/control ids that you document
#endif
