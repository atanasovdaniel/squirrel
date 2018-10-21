/*  see copyright notice in squirrel.h */
#ifndef _SQSTD_STREAM_H_
#define _SQSTD_STREAM_H_

#ifdef __cplusplus

struct SQStream {
    virtual void Release() = 0;
    virtual SQInteger Read(void *buffer, SQInteger size) = 0;
    virtual SQInteger Write(const void *buffer, SQInteger size) = 0;
    virtual SQInteger Flush() = 0;
    virtual SQInteger Tell() = 0;
    virtual SQInteger Len() = 0;
    virtual SQInteger Seek(SQInteger offset, SQInteger origin) = 0;
    virtual bool IsValid() = 0;
    virtual bool EOS() = 0;
    virtual SQInteger Close() { return 0; };
};

extern "C" {
#endif

#define SQSTD_STREAM_TYPE_TAG 0x80000000

#define SQ_SEEK_CUR 0
#define SQ_SEEK_END 1
#define SQ_SEEK_SET 2

typedef void *SQSTREAM;

SQUIRREL_API SQInteger sqstd_sread(void*, SQInteger, SQSTREAM);
SQUIRREL_API SQInteger sqstd_swrite(const void*, SQInteger, SQSTREAM);
SQUIRREL_API SQInteger sqstd_sseek(SQSTREAM , SQInteger , SQInteger);
SQUIRREL_API SQInteger sqstd_stell(SQSTREAM);
SQUIRREL_API SQInteger sqstd_sflush(SQSTREAM);
SQUIRREL_API SQInteger sqstd_seof(SQSTREAM);
SQUIRREL_API SQInteger sqstd_sclose(SQSTREAM);
SQUIRREL_API void sqstd_srelease(SQSTREAM);

SQUIRREL_API SQRESULT sqstd_getstream(HSQUIRRELVM v, SQInteger idx, SQSTREAM *stream);

SQRESULT declare_stream(HSQUIRRELVM v,const SQChar* name,SQUserPointer typetag,const SQChar* reg_name,const SQRegFunction *methods,const SQRegFunction *globals);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif /*_SQSTD_STREAM_H_*/
