/*  see copyright notice in sqstdstreamio.cpp */
#ifndef _SQSTDSTREAMIO_H_
#define _SQSTDSTREAMIO_H_

#ifdef __cplusplus
extern "C" {
#endif

SQUIRREL_API SQUserPointer _sqstd_streamreader_type_tag(void);
#define SQSTD_STREAMREADER_TYPE_TAG _sqstd_streamreader_type_tag()

typedef struct tagSQSRDR *SQSRDR;

SQUIRREL_API SQSRDR sqstd_streamreader(SQSTREAM stream,SQBool owns,SQInteger buffer_size);
SQUIRREL_API SQInteger sqstd_srdrmark(SQSRDR srdr,SQInteger readAheadLimit);
SQUIRREL_API SQInteger sqstd_srdrreset(SQSRDR srdr);

SQUIRREL_API SQRESULT sqstd_register_streamiolib(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // _SQSTDSTREAMIO_H_
