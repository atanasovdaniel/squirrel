/*  see copyright notice in sqcvoschar.cpp */
#ifndef _SQSTD_CVOSCHAR_H_
#define _SQSTD_CVOSCHAR_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #define scoschar_t wchar_t
#else
    #define scoschar_t char
#endif

SQUIRREL_API const scoschar_t** sccvtoos( const SQChar **in, SQUnsignedInteger in_cnt, SQUnsignedInteger *palloc);
SQUIRREL_API const SQChar** sccvfromos( const scoschar_t **in, SQUnsignedInteger in_cnt, SQUnsignedInteger *palloc);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // _SQSTD_CVOSCHAR_H_
