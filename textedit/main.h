
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
#endif
