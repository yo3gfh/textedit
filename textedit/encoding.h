
#ifndef _ENCODING_H
#define _ENCODING_H

#include <windows.h>

extern char * UTF_16_BE_BOM;
extern char * UTF_16_LE_BOM;
extern char * UTF_8_BOM;
extern char * UTF_32_BE_BOM;
extern char * UTF_32_LE_BOM;

extern int      ENC_CheckBOM             ( const char * src, size_t size );
extern int      ENC_IsUNICODE            ( const char * src, size_t size );
extern int      ENC_IsUTF8               ( const char * src, size_t size );
extern int      ENC_CheckEncoding        ( const char * src, size_t size );
extern int      ENC_FileCheckEncoding    ( const TCHAR * fname );
extern const    TCHAR * ENC_EncTypeToStr ( int encoding );
extern void ENC_ChangeEndian ( WCHAR * dest, const WCHAR * src, size_t chars );
extern size_t ENC_Utf32Utf8_LE 
    ( const char *src, int srclen, char *dst, int dstlen );
extern size_t ENC_Utf32Utf8_BE 
    ( const char *src, int srclen, char *dst, int dstlen );
extern DWORD    ENC_Uint32_to_BE        ( const void * vp );
BOOL            ENC_CheckUtf32_LE       ( const DWORD * wstr, size_t src_len );
BOOL            ENC_CheckUtf32_BE       ( const DWORD * wstr, size_t src_len );

typedef enum {
    E_UTF_8              = 10,
    E_UTF_8_BOM          = 11,
    E_UTF_32_LE          = 12,
    E_UTF_32_BE          = 13,
    E_UTF_16_LE          = 14,
    E_UTF_16_BE          = 15,
    E_UTF_32_LE_BOM      = 16,
    E_UTF_32_BE_BOM      = 17,
    E_UTF_16_LE_BOM      = 18,
    E_UTF_16_BE_BOM      = 19,
    E_ANSI               = 20,
    E_WHATEVER           = 21,
    E_DONT_BOTHER        = 22
} ENC_TYPE;

#endif
