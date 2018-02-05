/*  see copyright notice in sqstdtextio.cpp */
#ifndef _SQSTDTEXTIO_H_
#define _SQSTDTEXTIO_H_

#ifdef __cplusplus
extern "C" {
#endif

SQUIRREL_API SQUserPointer _sqstd_textreader_type_tag(void);
#define SQSTD_TEXTREADER_TYPE_TAG _sqstd_textreader_type_tag()
SQUIRREL_API SQUserPointer _sqstd_textwriter_type_tag(void);
#define SQSTD_TEXTWRITER_TYPE_TAG _sqstd_textwriter_type_tag()

SQUIRREL_API SQFILE sqstd_textreader(SQSTREAM stream,SQBool owns,const SQChar *encoding,SQBool guess);
SQUIRREL_API SQFILE sqstd_textwriter(SQSTREAM stream,SQBool owns,const SQChar *encoding);

SQUIRREL_API const SQChar *sqstd_guessencoding_srdr(SQSRDR srdr);
SQUIRREL_API SQFILE sqstd_textreader_srdr(SQSRDR srdr,SQBool owns_close,SQBool owns_release,const SQChar *encoding,SQBool guess);

SQUIRREL_API SQRESULT sqstd_register_textiolib(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // _SQSTDTEXTIO_H_
