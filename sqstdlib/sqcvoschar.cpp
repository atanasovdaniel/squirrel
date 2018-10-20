/*
Copyright (c) 2017-2018 Daniel Atanasov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <string.h>
#include <wchar.h>
#include <squirrel.h>
#include <sqcvoschar.h>

#ifdef _WIN32
    #ifdef SQUNICODE
        // win32 and squnicode
        // UTF-16 <-> UTF-16
        //  no conversion needed
        #define _NOOP_CV
    #else // SQUNICODE
        // win32 and !squnicode
        // OEM <-> UTF-16
        #define _MBSRTOWCS_USED 1
    #endif // SQUNICODE
#else // _WIN32
    #ifdef SQUNICODE
        // !win32 and squnicode
        // UTF-32 <-> UTF-8
        #define _MBSRTOWCS_USED 1
    #else // SQUNICODE
        // !win32 and !squnicode
        // UTF-8 <-> UTF-8
        //  no conversion needed
        #define _NOOP_CV
    #endif // SQUNICODE
#endif // _WIN32


#ifdef _MBSRTOWCS_USED

#define GROW_MBTOWC( _mbcount)  ((_mbcount)*sizeof(wchar_t))

static size_t chars_left( const char *first, const char **next, size_t in_cnt)
{
    size_t left = 1 + strlen(first);
    while( in_cnt) {
        left += 1 + strlen( *next);
        next++;
        in_cnt--;
    }
    return left;
}

static const wchar_t** multibyte_to_wchar( const char **in, SQUnsignedInteger in_cnt, SQUnsignedInteger *palloc)
{
    size_t in_len = chars_left( *in, in+1, in_cnt-1);
    size_t in_idx;
    size_t out_alloc;
    size_t out_pos;
    wchar_t *out_buff;
    out_alloc = (in_cnt + 1)*sizeof(wchar_t*) + GROW_MBTOWC(in_len);  // in_len*sizeof(wchar_t);
    out_buff = (wchar_t*)sq_malloc( out_alloc);
    memset( out_buff, 0, (in_cnt + 1)*sizeof(wchar_t*));
    out_pos = ((in_cnt+1)*sizeof(wchar_t*) + sizeof(wchar_t*)-1)/sizeof(wchar_t);
    in_idx = 0;
    while( in_idx < in_cnt) {
        mbstate_t ps;
        const char *src = in[in_idx];
        size_t r;
        memset( &ps, 0, sizeof(ps));
        ((wchar_t**)out_buff)[in_idx] = (wchar_t*)out_pos;
        do {
            if( out_alloc/sizeof(wchar_t) <= out_pos) {
                size_t left = chars_left( src, in+in_idx+1, in_cnt-in_idx-1);
                size_t re_alloc = out_alloc + GROW_MBTOWC(left);    //left*sizeof(wchar_t);
                out_buff = (wchar_t*)sq_realloc( out_buff, out_alloc, re_alloc);
                out_alloc = re_alloc;
            }
            r = mbsrtowcs( out_buff + out_pos, &src, out_alloc/sizeof(wchar_t)-out_pos, &ps);
            if( r != (size_t)-1) {
                out_pos += r;
            }
            else {
                //out_buff[out_pos] = '\0';
                //break;
                sq_free( out_buff, out_alloc);
                *palloc = 0;
                return 0;
            }
        } while( src);
        out_pos++;
        in_idx++;
    }
    in_idx = 0;
    while( in_idx < in_cnt) {
        ((wchar_t**)out_buff)[in_idx] = (wchar_t*)((wchar_t*)out_buff + (size_t)(((wchar_t**)out_buff)[in_idx]));
        in_idx++;
    }
    *palloc = out_alloc;
    return (const wchar_t**)out_buff;
}

static size_t wchars_left( const wchar_t *first, const wchar_t **next, size_t in_cnt)
{
    size_t left = 1 + wcslen(first);
    while( in_cnt) {
        left += 1 + wcslen( *next);
        next++;
        in_cnt--;
    }
    return left;
}

#define GROW_WCTOMB( _wccount)  ((_wccount) + ((_wccount)+1)/2)

static const char** wchar_to_multibyte( const wchar_t **in, SQUnsignedInteger in_cnt, SQUnsignedInteger *palloc)
{
    size_t in_len = wchars_left( *in, in+1, in_cnt-1);
    size_t in_idx;
    size_t out_alloc;
    size_t out_pos;
    char *out_buff;
    out_alloc = (in_cnt + 1)*sizeof(char*) + GROW_WCTOMB(in_len);
    out_buff = (char*)sq_malloc( out_alloc);
    memset( out_buff, 0, (in_cnt + 1)*sizeof(char*));
    out_pos = ((in_cnt+1)*sizeof(char*) + sizeof(char)-1)/sizeof(char);
    in_idx = 0;
    while( in_idx < in_cnt) {
        mbstate_t ps;
        const wchar_t *src = in[in_idx];
        size_t r;
        memset( &ps, 0, sizeof(ps));
        ((char**)out_buff)[in_idx] = (char*)out_pos;
        do {
            if( out_alloc/sizeof(char) <= out_pos) {
                size_t left = wchars_left( src, in+in_idx+1, in_cnt-in_idx-1);
                size_t re_alloc = out_alloc + GROW_WCTOMB(left);
                out_buff = (char*)sq_realloc( out_buff, out_alloc, re_alloc);
                out_alloc = re_alloc;
            }
            r = wcsrtombs( out_buff + out_pos, &src, out_alloc/sizeof(char)-out_pos, &ps);
            if( r != (size_t)-1) {
                out_pos += r;
            }
            else {
                //out_buff[out_pos] = '\0';
                //break;
                sq_free( out_buff, out_alloc);
                *palloc = 0;
                return 0;
            }
        } while( src);
        out_pos++;
        in_idx++;
    }
    in_idx = 0;
    while( in_idx < in_cnt) {
        ((char**)out_buff)[in_idx] = (char*)((char*)out_buff + (size_t)(((char**)out_buff)[in_idx]));
        in_idx++;
    }
    *palloc = out_alloc;
    return (const char**)out_buff;
}

#endif // _MBSRTOWCS_USED

#ifdef _NOOP_CV
    const scoschar_t** sccvtoos( const SQChar **in, SQUnsignedInteger SQ_UNUSED_ARG(in_cnt), SQUnsignedInteger *palloc)
    {
        *palloc = 0; return in;
    }
    const SQChar** sccvfromos( const scoschar_t **in, SQUnsignedInteger SQ_UNUSED_ARG(in_cnt), SQUnsignedInteger *palloc)
    {
        *palloc = 0; return in;
    }
#else // _NOOP_CV
    #ifdef _WIN32
        const scoschar_t** sccvtoos( const SQChar **in, SQUnsignedInteger in_cnt, SQUnsignedInteger *palloc)
        {
            return multibyte_to_wchar( in, in_cnt, palloc);
        }
        const SQChar** sccvfromos( const scoschar_t **in, SQUnsignedInteger in_cnt, SQUnsignedInteger *palloc)
        {
            return wchar_to_multibyte( in, in_cnt, palloc);
        }
    #else // _WIN32
        const scoschar_t** sccvtoos( const SQChar **in, SQUnsignedInteger in_cnt, SQUnsignedInteger *palloc)
        {
            return wchar_to_multibyte( in, in_cnt, palloc);
        }
        const SQChar** sccvfromos( const scoschar_t **in, SQUnsignedInteger in_cnt, SQUnsignedInteger *palloc)
        {
            return multibyte_to_wchar( in, in_cnt, palloc);
        }
    #endif // _WIN32
#endif // _NOOP_CV
