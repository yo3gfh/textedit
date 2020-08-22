
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
// encoding.c - routines for dealing with unicode

#include "main.h"
#include "encoding.h"
#include <stdint.h>

const char * UTF_16_BE_BOM   = "\xFE\xFF";
const char * UTF_16_LE_BOM   = "\xFF\xFE";
const char * UTF_8_BOM       = "\xEF\xBB\xBF";
const char * UTF_32_BE_BOM   = "\x00\x00\xFE\xFF";
const char * UTF_32_LE_BOM   = "\xFF\xFE\x00\x00";

int ENC_CheckBOM ( const char * src, size_t size )
/*****************************************************************************************************************/
/* see if we have a valid BOM (Byte Order Mark)                                                                  */
/* this is from stackoverflow, somewhere in time, don't have the link anymore :(                                 */
{
    if ( src == NULL || size == 0 ) { return 0; }

    if (size >= 3) {
        if (memcmp(src, UTF_8_BOM, 3) == 0)
            return E_UTF_8_BOM;
    }
    if (size >= 4) {
        if (memcmp(src, UTF_32_LE_BOM, 4) == 0)
            return E_UTF_32_LE_BOM;
        if (memcmp(src, UTF_32_BE_BOM, 4) == 0)
            return E_UTF_32_BE_BOM;
    }
    if (size >= 2) {
        if (memcmp(src, UTF_16_LE_BOM, 2) == 0)
            return E_UTF_16_LE_BOM;
        if (memcmp(src, UTF_16_BE_BOM, 2) == 0)
            return E_UTF_16_BE_BOM;
    }
    return E_WHATEVER;
}

int ENC_IsUTF8 ( const char * src, size_t size )
/*****************************************************************************************************************/
/* this is from stackoverflow, somewhere in time, don't have the link anymore :(                                 */
/* check if file is UTF8. This one returns false if all bytes are ASCII                                          */
{
    unsigned char           byte;
    unsigned int            code_length, i;
    unsigned int            ch;
    const unsigned char     * str;
    const unsigned char     * end;
    int                     all_ascii = 1;

    if ( src == NULL || size == 0 ) { return 0; }

    str = (unsigned char*)src;
    end = str + size;

    while ( str != end )
    {
        byte = *str;
        if ( ( byte & 0x80 ) != 0 ) all_ascii = 0; // this is my great contribution :-)
        if ( byte <= 0x7F )
        {
            /* 1 byte sequence: U+0000..U+007F */
            str += 1;
            continue;
        }

        if ( 0xC2 <= byte && byte <= 0xDF )
            /* 0b110xxxxx: 2 bytes sequence */
            code_length = 2;
        else if ( 0xE0 <= byte && byte <= 0xEF )
            /* 0b1110xxxx: 3 bytes sequence */
            code_length = 3;
        else if ( 0xF0 <= byte && byte <= 0xF4 )
            /* 0b11110xxx: 4 bytes sequence */
            code_length = 4;
        else {
            /* invalid first byte of a multibyte character */
            return 0;
        }

        if ( str + ( code_length - 1 ) >= end )
        {
            /* truncated string or invalid byte sequence */
            return 0;
        }

        /* Check continuation bytes: bit 7 should be set, bit 6 should be
         * unset (b10xxxxxx). */
        for ( i=1; i < code_length; i++ )
        {
            if ( (str[i] & 0xC0 ) != 0x80 )
                return 0;
        }

        if ( code_length == 2 )
        {
            /* 2 bytes sequence: U+0080..U+07FF */
            ch = ( ( str[0] & 0x1f ) << 6 ) + ( str[1] & 0x3f );
            /* str[0] >= 0xC2, so ch >= 0x0080.
               str[0] <= 0xDF, (str[1] & 0x3f) <= 0x3f, so ch <= 0x07ff */
        } 
        else if ( code_length == 3 )
        {
            /* 3 bytes sequence: U+0800..U+FFFF */
            ch = ( ( str[0] & 0x0f ) << 12 ) + ( ( str[1] & 0x3f ) << 6 ) + ( str[2] & 0x3f );
            /* (0xff & 0x0f) << 12 | (0xff & 0x3f) << 6 | (0xff & 0x3f) = 0xffff,
               so ch <= 0xffff */
            if ( ch < 0x0800 )
                return 0;

            /* surrogates (U+D800-U+DFFF) are invalid in UTF-8:
               test if (0xD800 <= ch && ch <= 0xDFFF) */
            if ( ( ch >> 11 ) == 0x1b )
                return 0;
        }
        else if  (code_length == 4 )
        {
            /* 4 bytes sequence: U+10000..U+10FFFF */
            ch = ( (str[0] & 0x07 ) << 18 ) + ( ( str[1] & 0x3f ) << 12 ) + ( ( str[2] & 0x3f ) << 6 ) + ( str[3] & 0x3f );
            if ( ( ch < 0x10000 ) ||  (0x10FFFF < ch ) )
                return 0;
        }
        str += code_length;
    }
    if ( all_ascii ) { return 0; } // if all bytes ASCII, let it be ANSI, not UTF8
    return 1;
}

int ENC_IsUNICODE ( const char * src, size_t size )
/*****************************************************************************************************************/
/* check the file using IsTextUnicode                                                                            */
{
    int         itu_result;
    BOOL        uni;

    if ( src == NULL || size == 0 ) { return 0; }
    itu_result = ~0; // turn on all tests 

    uni = IsTextUnicode ( src, size, &itu_result );
    if ( itu_result & IS_TEXT_UNICODE_REVERSE_STATISTICS )
    {
        // check for this first, otherwise will be valid UTF16 BE :-)
        if ( ENC_CheckUtf32_BE ( (DWORD *)src, size ) )
            return E_UTF_32_BE;

        return E_UTF_16_BE;
    }

    if ( uni ) { return E_UTF_16_LE; }

    return E_WHATEVER;
}

int ENC_CheckEncoding ( const char * src, size_t size )
/*****************************************************************************************************************/
/* see what kind of file we got                                                                                  */
{
    int     enc, bom;

    if ( src == NULL || size == 0 ) { return 0; }
    
    enc = ENC_IsUNICODE ( src, size );
    bom = ENC_CheckBOM ( src, size );

    if ( enc != E_WHATEVER ) // see if it's UNICODE
    {
        if ( bom != E_WHATEVER ) // yes, do we have a BOM?
            return bom; // yes, trust the bom
        else
            return enc; // no, stick with encoding
    }

    if ( ENC_IsUTF8 ( src, size ) ) // not UNICODE, but UTF8, maybe?
    {
        if ( bom == E_UTF_8_BOM ) // have a 3 byte UTF8 bom?
            return bom;
        else
            return E_UTF_8;
    }

    // how about utf32 LE with bom?
    if ( bom == E_UTF_32_LE_BOM )
        return bom;

    // or without it :-)
    if ( ENC_CheckUtf32_LE ( (DWORD *)src, size ) )
        return E_UTF_32_LE;

    return E_ANSI; // hope it's not binary lol
}

int ENC_FileCheckEncoding ( const TCHAR * fname )
/*****************************************************************************************************************/
/* open a file, memory map it and check encoding                                                                 */
{
    HANDLE      hFile, hMap;
    void        * base;
    DWORD       size;
    int         res;

    if ( fname == NULL || fname == TEXT("") ) { return 0; }

    hFile = CreateFile ( fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );

    if ( hFile == INVALID_HANDLE_VALUE )
        return 0;

    hMap = CreateFileMapping ( hFile, NULL, PAGE_READONLY, 0, 0, NULL );

    if ( ( hMap == NULL) || ( GetLastError() == ERROR_ALREADY_EXISTS ) )
    {
        CloseHandle ( hFile );
        return 0;
    }

    base = MapViewOfFile ( hMap, FILE_MAP_READ, 0, 0, 0 );

    if ( base == NULL )
    {
        CloseHandle ( hMap );
        CloseHandle ( hFile );
        return 0;
    }

    size = GetFileSize ( hFile, NULL );

    __try
    {
        res = ENC_CheckEncoding ( (const char *)base, size );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        res = 0;
    }

    UnmapViewOfFile ( base );
    CloseHandle ( hMap );
    CloseHandle ( hFile );

    return res;
}

const TCHAR * ENC_EncTypeToStr ( int encoding )
/*****************************************************************************************************************/
/* little helper function for displaying encoding as text in status bar                                          */
{
    static const TCHAR * etypes[] = {
                TEXT("UTF-8"),
                TEXT("UTF-8 BOM"),
                TEXT("UTF-32 (LE)"),
                TEXT("UTF-32 (BE)"),
                TEXT("UTF-16 (LE)"),
                TEXT("UTF-16 (BE)"),
                TEXT("UTF-32 BOM (LE)"),
                TEXT("UTF-32 BOM (BE)"),
                TEXT("UTF-16 BOM (LE)"),
                TEXT("UTF-16 BOM (BE)"),
                TEXT("ANSI"),
                TEXT("WHATEVER"),
                TEXT("DONT_BOTHER")
        };

    return ( ( encoding - E_UTF_8 ) >= 0 ? etypes[encoding - E_UTF_8] : TEXT("") );
}

void ENC_ChangeEndian ( WCHAR * dest, const WCHAR * src, size_t chars )
/*****************************************************************************************************************/
/* make dest the reverse endian of src                                                                           */
{
    size_t  i;

    for ( i = 0; i < chars; i++, dest++, src++ )
    {
        *dest = (WCHAR) ( ( ( *src << 8 ) & 0xFF00 ) + ( ( *src >> 8 ) & 0xFF ) );
    }
}

uint32_t Utf32Get ( int bigEndian, const char *src )
/*****************************************************************************************************************/
/* helper function for ENC_Utf32Utf8_LE/BE                                                                       */
/* from FindFile sources, by John Findlay, http://www.johnfindlay.plus.com/pellesc/utils/utils.html              */
{
	uint32_t codePoint;

	if (bigEndian)
	{
		codePoint = ((uint8_t)src[0] << 24) | ((uint8_t)src[1] << 16)
            | ((uint8_t)src[2] << 8) | (uint8_t)src[3];
	}
	else
	{
		codePoint = ((uint8_t)src[3] << 24) | ((uint8_t)src[2] << 16)
            | ((uint8_t)src[1] << 8) | (uint8_t)src[0];
	}

	return codePoint;

}

int Utf8Put ( int32_t codePoint, char *dst, size_t *numUnits )
/*****************************************************************************************************************/
/* helper function for ENC_Utf32Utf8_LE/BE                                                                       */
/* from FindFile sources, by John Findlay, http://www.johnfindlay.plus.com/pellesc/utils/utils.html              */
{

	if (numUnits != NULL)
		*numUnits = 0;

	if ((codePoint < 0) || (0x10FFFF < codePoint))
		return -1;

	if (dst == NULL)
		return -1;

	// One-octet code point (Unicode range U+000000 to U+00007F)?
	if (codePoint < 0x80)
	{
		dst[0] = codePoint & 0x7F;
		if (numUnits != NULL)
			*numUnits = 1;
	}

	// Two-octet code point (Unicode range U+000080 to U+0007FF)?
	else if (codePoint < 0x800)
	{
		dst[0] = 0xC0 | ((codePoint & 0x07C0) >> 6);
		dst[1] = 0x80 | (codePoint & 0x003F);
		if (numUnits != NULL)
			*numUnits = 2;
	}

	// Three-octet code point (Unicode range U+000800 to U+00FFFF)?
	else if (codePoint < 0x10000)
	{
		dst[0] = 0xE0 | ((codePoint & 0x0F000) >> 12);
		dst[1] = 0x80 | ((codePoint & 0x00FC0) >> 6);
		dst[2] = 0x80 | (codePoint & 0x0003F);
		if (numUnits != NULL)
			*numUnits = 3;
	}

	// Four-octet code point (Unicode range U+010000 to U+10FFFF)?
	else
	{
		dst[0] = 0xF0 | ((codePoint & 0x1C0000) >> 18);
		dst[1] = 0x80 | ((codePoint & 0x03F000) >> 12);
		dst[2] = 0x80 | ((codePoint & 0x000FC0) >> 6);
		dst[3] = 0x80 | (codePoint & 0x00003F);
		if (numUnits != NULL)
			*numUnits = 4;
	}

	return 0;
}

int Utf32Bom ( const char *src )
/*****************************************************************************************************************/
/* helper function for ENC_Utf32Utf8_LE/BE                                                                       */
/* from FindFile sources, by John Findlay, http://www.johnfindlay.plus.com/pellesc/utils/utils.html              */
{
	if ( ((uint8_t)src[0] == 0 ) && ( (uint8_t)src[1] == 0 )
		&& ( (uint8_t)src[2] == 0xFE ) && ( (uint8_t)src[3] == 0xFF ))
	{
		return 1;	// BOM, big-endian
	}
	else if (((uint8_t)src[0] == 0xFF) && ((uint8_t)src[1] == 0xFE)
		&& ((uint8_t)src[2] == 0) && ((uint8_t)src[3] == 0))
	{
		return -1;	// BOM, little-endian
	}

	return 0;	// No BOM
}

size_t ENC_Utf32Utf8_LE ( const char *src, int srclen, char *dst, int dstlen )
/*****************************************************************************************************************/
/* convert UTF32 to UTF8, little endian                                                                          */
/* from FindFile sources, by John Findlay, http://www.johnfindlay.plus.com/pellesc/utils/utils.html              */
/* small changes by me :-)                                                                                       */
{
	int32_t codePoint;
	size_t length, numUnits;

	length = 0;

	// Encode the UTF-32 code points as UTF-8.
	while ( srclen > 0 )
	{
		codePoint = Utf32Get ( 0, src );
		if ( codePoint < 0 )
		{
			return -1;
		}
		src += sizeof ( uint32_t );
		srclen -= sizeof ( uint32_t );
		if ( (dstlen < 4) && ((codePoint > 0x0FFFF ) || ( (dstlen < 3)
					&& ((codePoint > 0x00800) || ((dstlen < 2)
					&& ((codePoint > 0x0007F) || (dstlen < 1)))))))
		{
			return -1;
		}
        
		if ( Utf8Put ( codePoint, dst, &numUnits) )
		{
			return -1;
		}
		dst += numUnits;
		dstlen -= numUnits;
		length += numUnits;
	}

	if ( dstlen >= 1 )
		*dst = '\0';

	return length;
}

size_t ENC_Utf32Utf8_BE ( const char *src, int srclen, char *dst, int dstlen )
/*****************************************************************************************************************/
/* convert UTF32 to UTF8, big endian                                                                             */
/* from FindFile sources, by John Findlay, http://www.johnfindlay.plus.com/pellesc/utils/utils.html              */
/* small changes by me :-)                                                                                       */
{
	int32_t codePoint;
	size_t length, numUnits;

	length = 0;

	// Encode the UTF-32 code points as UTF-8.
	while ( srclen > 0 )
	{
		codePoint = Utf32Get ( 1, src );
		if ( codePoint < 0 )
		{
			return -1;
		}
		src += sizeof ( uint32_t );
		srclen -= sizeof ( uint32_t) ;
		if ( (dstlen < 4) && ((codePoint > 0x0FFFF ) || ( (dstlen < 3)
					&& ((codePoint > 0x00800) || ((dstlen < 2)
					&& ((codePoint > 0x0007F) || (dstlen < 1)))))))
		{
			return -1;
		}
        
		if ( Utf8Put ( codePoint, dst, &numUnits ) )
		{
			return -1;
		}
		dst += numUnits;
		dstlen -= numUnits;
		length += numUnits;
	}

	if ( dstlen >= 1 )
		*dst = '\0';

	return length;
}

DWORD ENC_Uint32_to_BE ( const void * vp )
/*****************************************************************************************************************/
/* convert little endian to big endian, DWORD                                                                    */
/* from forum.pellesc.de                                                                                         */
/* small changes by me :-)                                                                                       */
{
    const unsigned char * bp = vp;
    return (DWORD)bp[0] << 24 |
           (DWORD)bp[1] << 16 |
           (DWORD)bp[2] << 8 |
           (DWORD)bp[3] << 0;
}

BOOL ENC_CheckUtf32_LE ( const DWORD * wstr, size_t src_len )
/*****************************************************************************************************************/
/* check for bomless UTF32, little endian                                                                        */
/* from FindFile sources, by John Findlay, http://www.johnfindlay.plus.com/pellesc/utils/utils.html              */
/* small changes by me :-)                                                                                       */
{
    int cntLE = 0;
    int cntHigh = 0;

    if ( wstr == NULL || src_len == 0 )
        return FALSE;

    src_len >>= 2; // we count DWORDS
    src_len--;

	while ( *wstr && src_len >= 0 )
    {
		if ( *wstr == 0x0000000D )
			cntLE++;

		if ( (wchar_t)*wstr > (wchar_t)((*wstr)>>16) )
			cntHigh++;

		wstr++;
        src_len--;
	}
    return ( cntLE && cntHigh );
}

BOOL ENC_CheckUtf32_BE ( const DWORD * wstr, size_t src_len )
/*****************************************************************************************************************/
/* check for bomless UTF32, big endian                                                                           */
/* from FindFile sources, by John Findlay, http://www.johnfindlay.plus.com/pellesc/utils/utils.html              */
/* small changes by me :-)                                                                                       */
{
    int cntBE = 0;
    int cntHigh = 0;

    if ( wstr == NULL || src_len == 0 )
        return FALSE;

    src_len >>= 2;
    src_len--;

	while (  *wstr && src_len >= 0 )
    {
		if ( *wstr == 0x0D000000 )
			cntBE++;

		if ( (wchar_t)((*wstr)>>16) > (wchar_t)*wstr )
			cntHigh++;

		wstr++;
        src_len--;
	}
    return ( cntBE && cntHigh );
}
