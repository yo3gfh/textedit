
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

#ifndef _RESOURCE_H
#define _RESOURCE_H

#define IDR_MAINMENU                    102
#define IDR_OUTWMENU                    2001
#define IDR_ACCEL                       104

#define IDD_GOTO                        105
#define IDD_OUT                         106
#define IDD_EXT                         107
#define IDD_FIND                        108

#define IDI_SMALL                       508
#define IDI_LARGE                       508
#define IDI_SAV                         509
#define IDI_MOD                         510

#define IDD_ABOUT                       111
#define IDD_TABS                        112
#define IDD_HEX                         113

#define IDC_GOTO                        1000
#define IDC_CLOSE                       1001
#define IDC_LINES                       1002
#define IDC_CRTLINE                     1003
#define IDC_DEST                        1004
#define IDC_FINDSTR                     1005
#define IDC_FIND                        1006
#define IDC_CANCEL                      1007
#define IDC_CASE                        1008
#define IDC_WHOLE                       1009
#define IDC_REPALL                      1010
#define IDC_REPPROMPT                   1011
#define IDC_REPSTR                      1012
#define IDC_TABSTOPS                    1013
#define IDC_CB                          1014
#define IDC_CKREDIR                     1019
#define IDC_WKDIR                       1020
#define IDC_LIST                        1021
#define IDC_HEXLIST                     1022
#define IDC_SFWD                        1023

#define IDM_NEW                         40001
#define IDM_OPEN                        40002
#define IDM_SAVE                        40003
#define IDM_SAVEAS                      40004
#define IDM_QUIT                        40005
#define IDM_UNDO                        40006
#define IDM_CUT                         40007
#define IDM_COPY                        40008
#define IDM_PASTE                       40009
#define IDM_DEL                         40010
#define IDM_SELALL                      40011
#define IDM_ABOUT                       40012
#define IDM_FIND                        40013
#define IDM_FINDNEXT                    40014
#define IDM_MARGIN                      40015
#define IDM_GOTO                        40016
#define IDM_NEWINST                     40017
#define IDM_FONT                        40018
#define IDM_WRAP                        40019
#define IDM_SAVESEL                     40020
#define IDM_MERGE                       40021
#define IDM_CLEAR                       40022
#define IDM_VEXPLORE                    40023
#define IDM_ECLIP                       40025
#define IDM_FINDSEL                     40026
#define IDM_RUN                         40027
#define IDM_WINFIND                     40028
#define IDM_PRINT                       40029
#define IDM_PROP                        40030
#define IDM_TABS                        40031
#define IDM_COLOR                       40032
#define IDM_UPCASE                      40033
#define IDM_LOWCASE                     40034
#define IDM_FGCOLOR                     40035
#define IDM_RUNEXT                      40036
#define IDM_VIEWOUT                     40037
#define IDM_VIEWHEX                     40038
#define IDM_REFR                        40039
#define IDM_CPLINES                     6001
#define IDM_UTF8                        6002
#define IDM_UTF8_BOM                    6003
#define IDM_UTF16_LE                    6004
#define IDM_UTF16_BE                    6005
#define IDM_UTF16_LE_BOM                6006
#define IDM_UTF16_BE_BOM                6007
#define IDM_UTF32_LE                    6012
#define IDM_UTF32_BE                    6013
#define IDM_UTF32_LE_BOM                6014
#define IDM_UTF32_BE_BOM                6015
#define IDM_ANSI                        6008
#define IDM_FINDSEL_BK                  6009
#define IDM_TAB_UP                      6010
#define IDM_TAB_DOWN                    6011

#define IDMM_FILE                       0
#define IDMM_EDIT                       1
#define IDMM_TOOLS                      2
#define IDMM_HELP                       3

#endif
