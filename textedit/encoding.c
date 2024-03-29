
// encoding.c - routines for dealing with unicode
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include "main.h"
#include "encoding.h"
#include <stdint.h>



char * UTF_16_BE_BOM   = "\xFE\xFF";
char * UTF_16_LE_BOM   = "\xFF\xFE";
char * UTF_8_BOM       = "\xEF\xBB\xBF";
char * UTF_32_BE_BOM   = "\x00\x00\xFE\xFF";
char * UTF_32_LE_BOM   = "\xFF\xFE\x00\x00";

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_CheckBOM 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: const char * src: const char * src: source buffer to check
//    Param.    2: size_t size     : src size, in bytes
/*--------------------------------------------------------------------------*/
//         AUTHOR: stackoverflow / Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Checks if we have a valid BOM (Byte Order Mark).
//                 From stackoverflow, somewhere in time, don't have
//                 the link anymore :(
/*--------------------------------------------------------------------@@-@@-*/
int ENC_CheckBOM ( const char * src, size_t size )
/*--------------------------------------------------------------------------*/
{
    if ( src == NULL || size == 0 ) 
        return 0;

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

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_IsUTF8 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: const char * src: stream to check
//    Param.    2: size_t size     : sizeof src (in bytes)
/*--------------------------------------------------------------------------*/
//         AUTHOR: stackoverflow / Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Check if src is UTF8. This one returns false if all bytes
//                 are ASCII From stackoverflow, somewhere in time,
//                 don't have the link anymore :(
/*--------------------------------------------------------------------@@-@@-*/
int ENC_IsUTF8 ( const char * src, size_t size )
/*--------------------------------------------------------------------------*/
{
    unsigned char           byte;
    unsigned int            code_length, i;
    unsigned int            ch;
    const unsigned char     * str;
    const unsigned char     * end;
    int                     all_ascii = 1;

    if ( src == NULL || size == 0 ) 
        return 0;

    str = (unsigned char*)src;
    end = str + size;

    while ( str != end )
    {
        byte = *str;

        if ( ( byte & 0x80 ) != 0 ) 
            all_ascii = 0; // this is my great contribution :-)

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
            ch = ( ( str[0] & 0x0f ) << 12 ) + ( ( str[1] & 0x3f ) << 6 )
                + ( str[2] & 0x3f );
            /* (0xff & 0x0f) << 12 | (0xff & 0x3f) << 6 |
                (0xff & 0x3f) = 0xffff, so ch <= 0xffff */
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
            ch = ( (str[0] & 0x07 ) << 18 ) + ( ( str[1] & 0x3f ) << 12 )
                + ( ( str[2] & 0x3f ) << 6 ) + ( str[3] & 0x3f );

            if ( ( ch < 0x10000 ) ||  (0x10FFFF < ch ) )
                return 0;
        }
        str += code_length;
    }

    if ( all_ascii )
        return 0; // if all bytes ASCII, let it be ANSI, not UTF8

    return 1;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_IsUNICODE 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: const char * src: data to check
//    Param.    2: size_t size     : data size, in bytes
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Some wrapper around IsTextUnicode WIN API; support routine
//                 for ENC_CheckEncoding. Returns 0 on failure, or E_UTF_xx
//                 otherwise. If source is not UTF16/32, returns E_WHATEVER,
//                 for further sorrow :)
/*--------------------------------------------------------------------@@-@@-*/
int ENC_IsUNICODE ( const char * src, size_t size )
/*--------------------------------------------------------------------------*/
{
    int         itu_result;
    BOOL        uni;

    if ( src == NULL || size == 0 )
        return 0;

    itu_result = ~0; // turn on all tests 

    uni = IsTextUnicode ( src, size, &itu_result );

    if ( itu_result & IS_TEXT_UNICODE_REVERSE_STATISTICS )
    {
        // check for this first, otherwise will be valid UTF16 BE :-)
        if ( ENC_CheckUtf32_BE ( (DWORD *)src, size ) )
            return E_UTF_32_BE;

        return E_UTF_16_BE;
    }

    if ( uni )
        return E_UTF_16_LE;

    return E_WHATEVER;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_CheckEncoding 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: const char * src: data to check
//    Param.    2: size_t size     : data size, in bytes
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Check what possible encoding we have in src. Checks for
//                 BOM-s as well and tries to fall through all possible
//                 situations. Tries :) Returns 0 on failure, E_UTF_xx on
//                 success and E_ANSI for "dunno"
/*--------------------------------------------------------------------@@-@@-*/
int ENC_CheckEncoding ( const char * src, size_t size )
/*--------------------------------------------------------------------------*/
{
    int     enc, bom;

    if ( src == NULL || size == 0 )
        return 0;
    
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

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_FileCheckEncoding 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: const TCHAR * fname : full path to file
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Opens a file, memory maps it and calls ENC_CheckEncoding
//                 on the map. Returns 0 on failure, E_UTF_xx or E_ANSI on
//                 success.
/*--------------------------------------------------------------------@@-@@-*/
int ENC_FileCheckEncoding ( const TCHAR * fname )
/*--------------------------------------------------------------------------*/
{
    HANDLE      hFile, hMap;
    void        * base;
    DWORD       size;
    int         res;

    if ( fname == NULL || lstrlen ( fname ) == 0 )
        return 0;

    hFile = CreateFile ( fname, GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
            /*FILE_ATTRIBUTE_NORMAL*/0, 0 );

    if ( hFile == INVALID_HANDLE_VALUE )
        return 0;

    size = GetFileSize ( hFile, NULL );

    // no reason to continue, since we can't map a 0 byte file,
    // but we want to keep it :-)
    if ( size == 0 ) 
    {
        CloseHandle ( hFile );
        return E_ANSI;
    }

    hMap = CreateFileMapping ( hFile, NULL, PAGE_READONLY, 
        0, 0, NULL );

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

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_EncTypeToStr 
/*--------------------------------------------------------------------------*/
//           Type: const TCHAR * 
//    Param.    1: int encoding : encoding, as returned by ENC_CheckEncoding
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Helper routine to get encoding as a string.
/*--------------------------------------------------------------------@@-@@-*/
const TCHAR * ENC_EncTypeToStr ( int encoding )
/*--------------------------------------------------------------------------*/
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

    return ( ( encoding - E_UTF_8 ) >= 0
        ? etypes[encoding - E_UTF_8] : TEXT("") );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_ChangeEndian 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: WCHAR * dest     : destination data data
//    Param.    2: const WCHAR * src: source data
//    Param.    3: size_t chars     : dest capacity, in chars
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Make dest the reverse endian of src
/*--------------------------------------------------------------------@@-@@-*/
void ENC_ChangeEndian ( WCHAR * dest, const WCHAR * src, size_t chars )
/*--------------------------------------------------------------------------*/
{
    size_t  i;

    for ( i = 0; i < chars; i++, dest++, src++ )
    {
        *dest = (WCHAR) ( ( ( *src << 8 ) & 0xFF00 )
            + ( ( *src >> 8 ) & 0xFF ) );
    }
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Utf32Get 
/*--------------------------------------------------------------------------*/
//           Type: uint32_t 
//    Param.    1: int bigEndian   : BE or not
//    Param.    2: const char *src : source data
/*--------------------------------------------------------------------------*/
//         AUTHOR: John Findlay, Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Helper function for ENC_Utf32Utf8_LE/BE from FindFile
//                 sources, by John Findlay,
//                 http://www.johnfindlay.plus.com/pellesc/utils/utils.html
/*--------------------------------------------------------------------@@-@@-*/
uint32_t Utf32Get ( int bigEndian, const char *src )
/*--------------------------------------------------------------------------*/
{
    uint32_t codePoint;

    if ( bigEndian )
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

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Utf8Put 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: int32_t codePoint: unicode char to write to dst as utf8
//    Param.    2: char *dst        : destination buf
//    Param.    3: size_t *numUnits : var to get how many chars written
/*--------------------------------------------------------------------------*/
//         AUTHOR: John Findlay, Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Helper function for ENC_Utf32Utf8_LE/BE from FindFile
//                 sources, by John Findlay,
//                 http://www.johnfindlay.plus.com/pellesc/utils/utils.html
//                 Takes a UNICODE char and writes it as UTF8 in dst,
//                 updating numUnits according to how many bytes were 
//                 necessary.
/*--------------------------------------------------------------------@@-@@-*/
int Utf8Put ( int32_t codePoint, char *dst, size_t *numUnits )
/*--------------------------------------------------------------------------*/
{
    if ( numUnits != NULL )
        *numUnits = 0;

    if ( (codePoint < 0) || (0x10FFFF < codePoint) )
        return -1;

    if ( dst == NULL )
        return -1;

    // One-octet code point (Unicode range U+000000 to U+00007F)?
    if ( codePoint < 0x80 )
    {
        dst[0] = codePoint & 0x7F;

        if ( numUnits != NULL )
            *numUnits = 1;
    }

    // Two-octet code point (Unicode range U+000080 to U+0007FF)?
    else if ( codePoint < 0x800 )
    {
        dst[0] = 0xC0 | ((codePoint & 0x07C0) >> 6);
        dst[1] = 0x80 | (codePoint & 0x003F);

        if ( numUnits != NULL )
            *numUnits = 2;
    }

    // Three-octet code point (Unicode range U+000800 to U+00FFFF)?
    else if ( codePoint < 0x10000 )
    {
        dst[0] = 0xE0 | ((codePoint & 0x0F000) >> 12);
        dst[1] = 0x80 | ((codePoint & 0x00FC0) >> 6);
        dst[2] = 0x80 | (codePoint & 0x0003F);

        if ( numUnits != NULL )
            *numUnits = 3;
    }

    // Four-octet code point (Unicode range U+010000 to U+10FFFF)?
    else
    {
        dst[0] = 0xF0 | ((codePoint & 0x1C0000) >> 18);
        dst[1] = 0x80 | ((codePoint & 0x03F000) >> 12);
        dst[2] = 0x80 | ((codePoint & 0x000FC0) >> 6);
        dst[3] = 0x80 | (codePoint & 0x00003F);

        if ( numUnits != NULL )
            *numUnits = 4;
    }

    return 0;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Utf32Bom 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: const char *src : data to ckeck
/*--------------------------------------------------------------------------*/
//         AUTHOR: John Findlay, Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Helper function for ENC_Utf32Utf8_LE/BE from FindFile
//                 sources, by John Findlay,
//                 http://www.johnfindlay.plus.com/pellesc/utils/utils.html
//                 Returns 1 for UTF32 BE BOM, -1 for UTF32 LE BOM,
//                 0 if no BOM
/*--------------------------------------------------------------------@@-@@-*/
int Utf32Bom ( const char *src )
/*--------------------------------------------------------------------------*/
{
    if ( ((uint8_t)src[0] == 0 ) && ( (uint8_t)src[1] == 0 )
        && ( (uint8_t)src[2] == 0xFE ) && ( (uint8_t)src[3] == 0xFF ))
    {
        return 1;    // BOM, big-endian
    }

    else if (((uint8_t)src[0] == 0xFF) && ((uint8_t)src[1] == 0xFE)
        && ((uint8_t)src[2] == 0) && ((uint8_t)src[3] == 0))
    {
        return -1;    // BOM, little-endian
    }

    return 0;    // No BOM
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_Utf32Utf8_LE 
/*--------------------------------------------------------------------------*/
//           Type: size_t 
//    Param.    1: const char *src: data to convert
//    Param.    2: int srclen     : data length (in bytes)
//    Param.    3: char *dst      : where to write
//    Param.    4: int dstlen     : data length (in bytes)
/*--------------------------------------------------------------------------*/
//         AUTHOR: John Findlay, Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Convert UTF32 to UTF8, little endian. From FindFile
//                 sources, by John Findlay,
//                 http://www.johnfindlay.plus.com/pellesc/utils/utils.html
//                 small changes by me :-)
/*--------------------------------------------------------------------@@-@@-*/
size_t ENC_Utf32Utf8_LE ( const char *src, int srclen, char *dst, int dstlen )
/*--------------------------------------------------------------------------*/
{
    int32_t     codePoint;
    size_t      length, numUnits;

    length = 0;

    // Encode the UTF-32 code points as UTF-8.
    while ( srclen > 0 )
    {
        codePoint = Utf32Get ( 0, src );

        if ( codePoint < 0 )
        {
            return (size_t)-1;
        }

        src     += sizeof ( uint32_t );
        srclen  -= sizeof ( uint32_t );

        if ( (dstlen < 4) && ((codePoint > 0x0FFFF ) || ( (dstlen < 3)
                    && ((codePoint > 0x00800) || ((dstlen < 2)
                    && ((codePoint > 0x0007F) || (dstlen < 1)))))))
        {
            return (size_t)-1;
        }
        
        if ( Utf8Put ( codePoint, dst, &numUnits) )
        {
            return (size_t)-1;
        }

        dst     += numUnits;
        dstlen  -= numUnits;
        length  += numUnits;
    }

    if ( dstlen >= 1 )
        *dst = '\0';

    return length;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_Utf32Utf8_BE 
/*--------------------------------------------------------------------------*/
//           Type: size_t 
//    Param.    1: const char *src: data to convert
//    Param.    2: int srclen     : data length (in bytes)
//    Param.    3: char *dst      : where to write
//    Param.    4: int dstlen     : data length (in bytes)
/*--------------------------------------------------------------------------*/
//         AUTHOR: John Findlay, Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Convert UTF32 to UTF8, big endian. From FindFile sources,
//                 by John Findlay,
//                 http://www.johnfindlay.plus.com/pellesc/utils/utils.html
//                 small changes by me :-)
/*--------------------------------------------------------------------@@-@@-*/
size_t ENC_Utf32Utf8_BE ( const char *src, int srclen, char *dst, int dstlen )
/*--------------------------------------------------------------------------*/
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
            return (size_t)-1;
        }

        src     += sizeof ( uint32_t );
        srclen  -= sizeof ( uint32_t);

        if ( (dstlen < 4) && ((codePoint > 0x0FFFF ) || ( (dstlen < 3)
                    && ((codePoint > 0x00800) || ((dstlen < 2)
                    && ((codePoint > 0x0007F) || (dstlen < 1)))))))
        {
            return (size_t)-1;
        }
        
        if ( Utf8Put ( codePoint, dst, &numUnits ) )
        {
            return (size_t)-1;
        }

        dst     += numUnits;
        dstlen  -= numUnits;
        length  += numUnits;
    }

    if ( dstlen >= 1 )
        *dst = '\0';

    return length;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_Uint32_to_BE 
/*--------------------------------------------------------------------------*/
//           Type: DWORD 
//    Param.    1: const void * vp : dword to convert
/*--------------------------------------------------------------------------*/
//         AUTHOR: forum.pellesc.de, Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Convert little endian to big endian, DWORD from
//                 forum.pellesc.de Small changes by me :-)
/*--------------------------------------------------------------------@@-@@-*/
DWORD ENC_Uint32_to_BE ( const void * vp )
/*--------------------------------------------------------------------------*/
{
    const unsigned char * bp = vp;
    return (DWORD)bp[0] << 24 |
           (DWORD)bp[1] << 16 |
           (DWORD)bp[2] << 8 |
           (DWORD)bp[3] << 0;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_CheckUtf32_LE 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: const DWORD * wstr: source data
//    Param.    2: size_t src_len    : data length (in bytes)
/*--------------------------------------------------------------------------*/
//         AUTHOR: John Findlay, Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Check for bomless UTF32, little endian From FindFile
//                 sources, by John Findlay,
//                 http://www.johnfindlay.plus.com/pellesc/utils/utils.html
//                 Small changes by me :-)
/*--------------------------------------------------------------------@@-@@-*/
BOOL ENC_CheckUtf32_LE ( const DWORD * wstr, size_t src_len )
/*--------------------------------------------------------------------------*/
{
    int cntLE   = 0;
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

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ENC_CheckUtf32_BE 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: const DWORD * wstr: source data
//    Param.    2: size_t src_len    : data length (in bytes)
/*--------------------------------------------------------------------------*/
//         AUTHOR: John Findlay, Adrian Petrila, YO3GFH
//           DATE: 28.09.2020
//    DESCRIPTION: Check for bomless UTF32, big endian From FindFile sources,
//                 by John Findlay,
//                 http://www.johnfindlay.plus.com/pellesc/utils/utils.html
//                 Small changes by me :-)
/*--------------------------------------------------------------------@@-@@-*/
BOOL ENC_CheckUtf32_BE ( const DWORD * wstr, size_t src_len )
/*--------------------------------------------------------------------------*/
{
    int cntBE   = 0;
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
