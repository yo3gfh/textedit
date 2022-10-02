
// menus.c - menu mngmnt routines
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include "main.h"
#include "menus.h"
#include "mem.h"
#include "misc.h"
#include "resource.h"
#include <strsafe.h>
#include <tchar.h>

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SetMenuItemText 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HMENU hmenu     : handle to menu
//    Param.    2: int menu_id     : menu ID to set text
//    Param.    3: TCHAR * newtext : new text to set
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Changes the curent text of menu with ID menu_id to newtext.
//                 Returns TRUE on success, FALSE otherwise.
/*--------------------------------------------------------------------@@-@@-*/
BOOL SetMenuItemText ( HMENU hmenu, int menu_id, TCHAR * newtext )
/*--------------------------------------------------------------------------*/
{
    MENUITEMINFO    mi;

    RtlZeroMemory ( &mi, sizeof(MENUITEMINFO) );

    mi.cbSize       = sizeof ( MENUITEMINFO );
    mi.fMask        = MIIM_DATA | MIIM_STRING;
    mi.fType        = MFT_STRING;
    mi.dwTypeData   = NULL;

    if ( !GetMenuItemInfo ( hmenu, menu_id, FALSE, &mi ) )
        return FALSE;

    mi.dwTypeData   = newtext;

    return SetMenuItemInfo ( hmenu, menu_id, FALSE, &mi );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: GetMenuItemText 
/*--------------------------------------------------------------------------*/
//           Type: TCHAR * 
//    Param.    1: HMENU hmenu : handle to menu
//    Param.    2: int menu_id : menu ID
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 19.09.2020
//    DESCRIPTION: Retrieves the current string for the menu with ID menu_id.
//                 On error, the returned string has 0 in the first element.
/*--------------------------------------------------------------------@@-@@-*/
TCHAR * GetMenuItemText ( HMENU hmenu, int menu_id )
/*--------------------------------------------------------------------------*/
{
    MENUITEMINFO    mi;
    static TCHAR    buf[256];

    RtlZeroMemory ( &mi, sizeof(MENUITEMINFO) );
    RtlZeroMemory ( buf, sizeof(buf) );

    mi.cbSize       = sizeof ( MENUITEMINFO );
    mi.fMask        = MIIM_DATA | MIIM_STRING;
    mi.fType        = MFT_STRING;
    mi.dwTypeData   = NULL;

    GetMenuItemInfo ( hmenu, menu_id, FALSE, &mi );

    mi.cch++;
    mi.cch          = ( mi.cch > ARRAYSIZE(buf) )
                        ? ARRAYSIZE(buf) : mi.cch;

    mi.dwTypeData   = buf;

    GetMenuItemInfo ( hmenu, menu_id, FALSE, &mi );

    return buf;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LoadUserMenu 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR 
//    Param.    1: HMENU hMenu           : user menu handle
//    Param.    2: USER_MENU * puMenu    : pointer to USER_MENU array
//    Param.    3: const TCHAR * ininame : path to config
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 26.09.2020
//    DESCRIPTION: Load user defined menu items from "ininame" config file.
//                 Entries must be added under [user menu] section and should
//                 have the following structure:
//                   <keyname>={separator}   or  
//                   <keyname>=<caption>,<command>,<crtdir>,<capture(0/1)>
//                   where <keyname> can be anything as long as it is unique,
//                   <caption> is the menu caption, <command> is the
//                   program+cmdl to exec, <crtdir> is the current dir for 
//                   the program and <capture> tells if the output of a
//                   console app should be captured. If <caption> is
//                   "{separator}", followed by no other params, a separator
//                   menu item is added instead. If <crtdir> is {p}, it will
//                   be expanded to file dir. If it's invalid (nonexisting),
//                   the startup dir. of the editor will be used. The
//                   <command> portion supports the macros that ExpandMacro
//                   knows about, see the synoptic. You can add a maximum of
//                   50 user defined menus (not counting separators).
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR LoadUserMenu ( HMENU hMenu, USER_MENU * puMenu, const TCHAR * ininame )
/*--------------------------------------------------------------------------*/
{
    TCHAR       * keys, * key, * p;
    TCHAR       data[256];
    INT_PTR     alloc_size;
    INT_PTR     i, menu_items;

    if ( hMenu == NULL || puMenu == NULL || ininame == NULL )
        return -1;
    
    // max menus., 128 chars max. each key name
    alloc_size  = MAX_USER_MENUS*128*sizeof(TCHAR); 
    keys        = (TCHAR *)alloc_and_zero_mem (ALIGN_4K(alloc_size));
    
    if ( keys == NULL )
        return -1;

    // let's see if we have something in our section
    if ( GetPrivateProfileString ( TEXT("user menu"),
            NULL, NULL, keys, (DWORD)alloc_size, ininame ) == 0 )
    {
        free_mem ( keys );
        return -1;
    }

    i           = 0;
    menu_items  = 0;

    // start enum the keys we just got
    do
    {
        key = (TCHAR*)(&(keys[i]));
        i += lstrlen ( key );

        // get key value...
        if ( GetPrivateProfileString ( TEXT("user menu"),
                key, NULL, data, ARRAYSIZE(data), ininame ) > 0 )
        {
            // ...and start parsing and copying to USER_MENU item
            // is it a separator?
            if (_tcsicmp ( data, TEXT("{separator}")) == 0 ) 
            {
                AppendMenu ( hMenu, MF_SEPARATOR, 0, NULL );
            }
            else
            {
                p = _tcstok ( data, TEXT(",") );

                if ( p != NULL )
                {
                    StringCchCopy ( puMenu[menu_items].caption,
                        ARRAYSIZE(puMenu[menu_items].caption), p );

                    p = _tcstok ( NULL, TEXT(",") );

                    if ( p != NULL )
                    {
                        StringCchCopy ( puMenu[menu_items].command,
                            ARRAYSIZE(puMenu[menu_items].command), p );

                        p = _tcstok ( NULL, TEXT(",") );

                        if ( p != NULL )
                        {
                            StringCchCopy ( puMenu[menu_items].wcurdir,
                                ARRAYSIZE(puMenu[menu_items].wcurdir), p );

                            p = _tcstok ( NULL, TEXT(",") );

                            if ( p != NULL )
                            {
                                puMenu[menu_items].capture = 
                                    (_tcscmp ( p, TEXT("1")) == 0 );

                                // we're here, we have a complete USER_MENU
                                // item, so let's add it to User Menu...
                                AppendMenu ( hMenu, MFT_STRING,
                                    IDM_USERSTART+menu_items,
                                    puMenu[menu_items].caption );

                                menu_items++; // ...and increment count
                            }
                        }
                    }
                }
            }
        }

        i++;

    }
    while ( (keys[i] != 0) && (menu_items < MAX_USER_MENUS) );

    free_mem ( keys );

    return menu_items;
}
