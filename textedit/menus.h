
#ifndef _MENUS_H
#define _MENUS_H

#include <windows.h>

#define MAX_USER_MENUS 50 // max entries in user menu

typedef struct _tag_usermenu
{
    TCHAR   caption[MAX_PATH];
    TCHAR   command[MAX_PATH];
    TCHAR   wcurdir[MAX_PATH];
    BOOL    capture;
} USER_MENU;

extern INT_PTR LoadUserMenu 
    ( HMENU hMenu, USER_MENU * puMenu, const TCHAR * ininame );
extern TCHAR * GetMenuItemText ( HMENU hmenu, int menu_id );
extern BOOL SetMenuItemText ( HMENU hmenu, int menu_id, TCHAR * newtext );

#endif

