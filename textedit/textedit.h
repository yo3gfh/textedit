
/*
    TEXTEDIT, a (somehow) small multi-file text editor with UNICODE support
    -----------------------------------------------------------------------
    Copyright (c) 2002-2020 Adrian Petrila, YO3GFH

    Started this because, at the time, I thought Notepad was crap and how
    hard can it be to write something better, like PFE (Programmers File
    Editor)? Well, after breaking my teeth in UNICODE, turned out that
    Notepad was not all that crappy and my kung-fu was weak.

    So welcome to more crap :-))

    Some random thoughts:
    ---------------------
    Originally buit with OpenWatcom, moved it to the Pelle's C compiler
    system (http://www.smorgasbordet.com/pellesc/index.htm)

    After working with Richedit, I can now fully understand why ppl go all
    the way and write their own custom edit controls.

    After working with the comctl tab control, I can see why OOP is good =)
    After trying to find a way to resize parent+child windows flicker-free,
    I just want to be alone and cry :-)))

    But I'm not all that good, anyway lol.

    Hope you find something useful in this pile of text ( printing and
    unicode streaming comes to mind ), doesnt't seem to have too many bugs
    (here's hoping lol) and, for me at least, gets the job done.

    And the usual disclaimer: this isn't cutting edge, elegant,
    NSA grade code. Not even my grandpa-grade, if I come to think
    (he was a vet, God rest his soul, he couldn't write code but he could
    make a cow sing). I don't work as a full-time programmer (used to,
    20 years ago) so I can very well afford to be incompetent :-)))

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
    --------

    * Uses Richedit v4.1 to open/edit/save text in ANSI and UNICODE
    (UTF8/16/32-LE/BE with/without BOM). You can use Richedit 3.0/2.0
    if you want (undef HAVE_MSFTEDIT), but not 1.0 (does not support UNICODE).
    It works on win7, 8 and 10, should work on xp sp3 too. Please note that
    it does not use any features specific to latest Richedit versions (7.5
    on W8 and 8.5 on W10).
    
    * Multiple file support (tabbed).
    * User Defined menus (custom apps)
    * Execute external programs in a separate thread, optionally capturing
      console output (you may use it as a crude IDE).
    * View current selection as HEX.
    * Parse source files for C function definitions/declarations in order to
      generate header files
    * Generate a synoptic (as comment) for the C function under cursor or on
      selection
    * Unfortunately, it supports drag-n-drop
    * Exciting, misterious and surprisingly powerful bugs which will crash
      your box =) These are 100% genuine and are all mine!

    Building
    --------

    * open the workspace editor.ppw and you'll have all three projects
      (textedit, spawn and texeditSH)
      which you can build all at once. You can also open the individual 
      projects (.ppj) from each project folder.

    * spawn.exe is for executing external tools (masm, for example); 
      texteditSH is a MS Explorer context menu
      shell extension to open files in TextEdit; the program works without
      either of these.

    * there is another completely optional "feature", detour_selcolor.dll, 
      which is a API hook dll for GetSysColor,
      so you can customize text selection ("highlight") in a Richedit control.
      Starting with v2.0 and up, Richedit does not offer such a possibility
      anymore :-) so we must resort to such hackery :)). Basically, selection
      fore/back will be the same as the one from textedit.ini.

    * you can compile all projects as x64/x86, UNICODE/non UNICODE;
      please note: regardless of build type, the richedit control will be in
      UNICODE mode, to support the above mentioned file encodings.

    * IMPORTANT: build both the app and the extension in the same way
      (both as UNICODE or ANSI, 32 or 64 bit), otherwise interesting things
      start to happen :-))
*/

#ifndef _TEXTEDIT_H
#define _TEXTEDIT_H

#include        <windows.h>
#include        <richedit.h>
#include        "resource.h"
#include        "encoding.h"

#define         RICH_VERT_SCROLL    3       // scroll three lines at a time
#define         IDC_STATUS          666
#define         IDFIND              1
#define         IDREPLACE           2
#define         IDFINDALL           3
#define         OFN_BUF_SIZE        0x0000FFFF
#define         CAPTURE_BUF_SIZE    0x80000 // 128k
#define         CARET_WIDTH         8       // 2 for "normal" caret
#define         MAX_TABS            32      // this is enough for *me* :-)
                                            // change it or switch to a 
                                            // dynamic TAB_DATA array
#define         WIN_CLASS           CS_DBLCLKS|CS_BYTEALIGNWINDOW| \
                                    CS_BYTEALIGNCLIENT

#define         TD_TINDEX           0x00000001
#define         TD_CHLDID           0x00000002
#define         TD_FENCOD           0x00000004
#define         TD_CHLDHW           0x00000008
#define         TD_FPATH            0x00000010
#define         TD_FNAME            0x00000020
#define         TD_FCMDSW           0x00000040
#define         TD_ALL              0x0000007F

#define         CT_UP               0x00000080
#define         CT_DOWN             0x00000160

#define         TIDX_SAV            0
#define         TIDX_MOD            1

#define         COPYDATA_MAGIC      0x0666DEAD

#ifdef UNICODE
    #define CCH_SPEC L"%ls, filesize is %.2f %ls"
    #define CCH_SPEC1 L"File saved as %ls, %.2f %ls saved to disk"
    #define CCH_SPEC2 L"Error opening %ls"
    #define CCH_SPEC3 L"Error saving %ls"
  
    #define CCH_SPEC4 L"Original file encoding (%ls)"\
                      L" differs from merged file encoding (%ls). "\
                      L"Take care when saving! (select a UNICODE "\
                      L"mode to be sure)"

    #define CCH_SPEC5 L"Library \"%ls\" not present :(" 
#else
    #define CCH_SPEC "%s, filesize is %.2f %s"
    #define CCH_SPEC1 "File saved as %s, %.2f %s saved to disk"
    #define CCH_SPEC2 "Error opening %s"
    #define CCH_SPEC3 "Error saving %s"

    #define CCH_SPEC4 "Original file encoding (%s)"\
                      "differs from merged file encoding (%s). "\
                      "Take care when saving! (select a UNICODE "\
                      "mode to be sure)"

    #define CCH_SPEC5 "Library \"%s\" not present :("
#endif

#ifdef _WIN64
    #define DETOUR_DLL TEXT("detour_selcolorX64.dll")
#else
    #define DETOUR_DLL TEXT("detour_selcolorX86.dll")
#endif

typedef struct _hex_data
{
    HGLOBAL     hg;
    DWORD       dwlength;
} HEX_DATA;

typedef struct _tab_data
{
    int         tab_index;
    int         child_id;
    int         fencoding;
    int         nCmdShow;
    HWND        hchild;
    TCHAR       fpath[TMAX_PATH+1];
    TCHAR       fname[TMAX_PATH+1];
} TAB_DATA;

#endif
