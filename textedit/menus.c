
/*
    TEXTEDIT, a (somehow) small multi-file text editor with UNICODE support
    -----------------------------------------------------------------------
    Copyright (c) 2002-2020 Adrian Petrila, YO3GFH
    
    Started this because, at the time, I thought Notepad was crap and how
    hard can it be to write something better, like PFE (Programmers File Editor)?
    Well, after breaking my teeth in UNICODE, turned out that Notepad was not all
    that crappy and my kung-fu was weak.

    So welcome to more crap :-))
    
    Some random thoughts:
    ---------------------
    Originally buit with OpenWatcom, moved it to the Pelle's C compiler system
    http://www.smorgasbordet.com/pellesc/index.htm

    After working with Richedit, I can now fully understand why ppl go all the way
    and write their own custom edit controls.

    After working with the comctl tab control, I can see why OOP is good =)
    After trying to find a way to resize parent+child windows flicker-free,
    I just want to be alone and cry :-)))

    But I'm not all that good, anyway lol.

    Hope you find something useful in this pile of text ( printing and unicode streaming
    comes to mind ), doesnt't seem to have too many bugs (here's hoping lol) and, for
    me at least, gets the job done.

    And the usual disclaimer: this isn't cutting edge, elegant, NSA grade code.
    Not even my grandpa-grade, if I come to think (he was a vet, God rest his soul,
    he couldn't write code but he could make a cow sing).
    I don't work as a full-time programmer (used to, 20 years ago) so I can
    very well afford to be incompetent :-)))
    
                                * * *
                                
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                * * *

    Features
    ---------
    
        - uses Richedit v4.1 to open/edit/save text in ANSI and UNICODE
        (UTF8/16/32-LE/BE with/without BOM). You cand use Richedit 3.0/2.0
        if you want (undef HAVE_MSFTEDIT), but not 1.0 (does not support UNICODE).
        It works on win7, 8 and 10, should work on xp sp3 too. Please note that 
        it does not use features specific to latest Richedit versions (7.5 on W8
        and 8.5 on W10).

        - Multiple file support (tabbed).

        - Execute external programs, optionally capturing console output
        (you may use it as a crude IDE).

        - View current selection as HEX.

        - Unfortunately, it supports drag-n-drop

        - Exciting, misterious and surprisingly powerful bugs which will crash
        your box =) These are 100% genuine and are all mine!

    Building
    --------

        - open the workspace editor.ppw and you'll have all three projects (textedit,
        spawn and texeditSH) which you can build all at once. You can also open the 
        individual projects (*.ppj) from each project folder.

        - spawn.exe is for executing external tools (masm, for example); texteditSH
        is a MS Explorer context menu shell extension to open files in TextEdit; 
        the program works without either of these.
*/

// menus.c - menu mngmnt routines
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include "main.h"
#include "menus.h"
#include "mem.h"
#include "misc.h"
#include "resource.h"
#include <strsafe.h>
#include <tchar.h>

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: SetMenuItemText 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: BOOL 
//                 Param.    1: HMENU hmenu     : handle to menu
//                 Param.    2: int menu_id     : menu ID to set text 
//                 Param.    3: TCHAR * newtext : new text to set 
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 19.09.2020
//                 DESCRIPTION: Changes the curent text of menu with ID menu_id to newtext.
//                              Returns TRUE on success, FALSE otherwise.
/*--------------------------------------------------------------------------------------------@@-@@-*/
BOOL SetMenuItemText ( HMENU hmenu, int menu_id, TCHAR * newtext )
/*--------------------------------------------------------------------------------------------------*/
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

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: GetMenuItemText 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: TCHAR * 
//                 Param.    1: HMENU hmenu : handle to menu
//                 Param.    2: int menu_id : menu ID
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 19.09.2020
//                 DESCRIPTION: Retrieves the current string for the menu with ID menu_id.
//                              On error, the returned string has 0 in the first element.
/*--------------------------------------------------------------------------------------------@@-@@-*/
TCHAR * GetMenuItemText ( HMENU hmenu, int menu_id )
/*--------------------------------------------------------------------------------------------------*/
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
    mi.cch          = ( mi.cch > ARRAYSIZE(buf) ) ? ARRAYSIZE(buf) : mi.cch;
    mi.dwTypeData   = buf;

    GetMenuItemInfo ( hmenu, menu_id, FALSE, &mi );

    return buf;
}

/*-@@+@@--------------------------------------------------------------------------------------------*/
//               Function Name: LoadUserMenu 
/*--------------------------------------------------------------------------------------------------*/
//      Return Type, Modifiers: INT_PTR 
//                 Param.    1: HMENU hMenu           : user menu handle
//                 Param.    2: USER_MENU * puMenu    : pointer to USER_MENU array
//                 Param.    3: const TCHAR * ininame : path to config
/*--------------------------------------------------------------------------------------------------*/
//                      AUTHOR: Adrian Petrila, YO3GFH
//                        DATE: 26.09.2020
//                 DESCRIPTION: Load user defined menu items from "ininame" config file.
//                              Entries must be added under [user menu] section and should
//                              have the following structure:
//                                  <keyname>={separator}   or  
//                                  <keyname>=<caption>,<command>,<crtdir>,<capture(0/1)>
//                              where <keyname> can be anything as long as it is unique,
//                              <caption> is the menu caption, <command> is the program+cmdl
//                              to exec, <crtdir> is the current dir for the program and
//                              <capture> tells if the output of a console app should be captured.
//                              If <caption> is "{separator}", followed by no other params, a separator
//                              menu item is added instead. If <crtdir> is {p}, it will be expanded
//                              to file dir. If it's invalid (nonexisting), the startup dir. of the 
//                              editor will be used. The <command> portion supports the macros
//                              that ExpandMacro knows about, see the synoptic.
//                              You can add a maximum of 40 user defined menus (not counting
//                              separators).
/*--------------------------------------------------------------------------------------------@@-@@-*/
INT_PTR LoadUserMenu ( HMENU hMenu, USER_MENU * puMenu, const TCHAR * ininame )
/*--------------------------------------------------------------------------------------------------*/
{
    TCHAR       * keys, * key, * p;
    TCHAR       data[256];
    DWORD       alloc_size;
    INT_PTR     i, menu_items;

    if ( hMenu == NULL || puMenu == NULL || ininame == NULL )
        return -1;
    
    alloc_size  = MAX_USER_MENUS*128*sizeof(TCHAR); // 20 menus max., 128 chars max. each key name
    keys        = (TCHAR *)alloc_and_zero_mem ( (alloc_size+4096+1) & (~4095) );

    if ( keys == NULL )
        return -1;

    // let's see if we have something in our section
    if ( GetPrivateProfileString ( TEXT("user menu"), NULL, NULL, keys, alloc_size, ininame ) == 0 )
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
        if ( GetPrivateProfileString ( TEXT("user menu"), key, NULL, data, ARRAYSIZE(data), ininame ) > 0 )
        {
            // ...and start parsing and copying to USER_MENU item
            if (_tcsicmp ( data, TEXT("{separator}")) == 0 ) // is it a separator?
            {
                AppendMenu ( hMenu, MF_SEPARATOR, 0, NULL );
            }
            else
            {
                p = _tcstok ( data, TEXT(",") );

                if ( p != NULL )
                {
                    StringCchCopy ( puMenu[menu_items].caption, ARRAYSIZE(puMenu[menu_items].caption), p );
                    p = _tcstok ( NULL, TEXT(",") );

                    if ( p != NULL )
                    {
                        StringCchCopy ( puMenu[menu_items].command, ARRAYSIZE(puMenu[menu_items].command), p );
                        p = _tcstok ( NULL, TEXT(",") );

                        if ( p != NULL )
                        {
                            StringCchCopy ( puMenu[menu_items].wcurdir, ARRAYSIZE(puMenu[menu_items].wcurdir), p );
                            p = _tcstok ( NULL, TEXT(",") );

                            if ( p != NULL )
                            {
                                puMenu[menu_items].capture = (_tcscmp ( p, TEXT("1")) == 0 );
                                // we're here, we have a complete USER_MENU item, so let's add it to User Menu...
                                AppendMenu ( hMenu, MFT_STRING, IDM_USERSTART+menu_items, puMenu[menu_items].caption );
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
