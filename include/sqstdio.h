/*  see copyright notice in squirrel.h */
#ifndef _SQSTDIO_H_
#define _SQSTDIO_H_

#include <sqstdstream.h>

#ifdef __cplusplus
extern "C" {
#endif

SQUIRREL_API SQUserPointer _sqstd_file_type_tag(void);
#define SQSTD_FILE_TYPE_TAG _sqstd_file_type_tag()
SQUIRREL_API SQUserPointer _sqstd_popen_type_tag(void);
#define SQSTD_POPEN_TYPE_TAG _sqstd_popen_type_tag()

typedef void* SQFILE;

SQUIRREL_API SQFILE sqstd_fopen(const SQChar *,const SQChar *);
SQUIRREL_API SQInteger sqstd_fread(void*, SQInteger, SQInteger, SQFILE);
SQUIRREL_API SQInteger sqstd_fwrite(const void *, SQInteger, SQInteger, SQFILE);
//SQUIRREL_API SQInteger sqstd_fseek(SQFILE , SQInteger , SQInteger);
#define sqstd_fseek(_f,_p,_o)   sqstd_sseek((SQSTREAM)(_f),_p,_o)
//SQUIRREL_API SQInteger sqstd_ftell(SQFILE);
#define sqstd_ftell(_f)         sqstd_stell((SQSTREAM)(_f))
//SQUIRREL_API SQInteger sqstd_fflush(SQFILE);
#define sqstd_fflush(_f)        sqstd_sflush((SQSTREAM)(_f))
//SQUIRREL_API SQInteger sqstd_feof(SQFILE);
#define sqstd_feof(_f)          sqstd_seof((SQSTREAM)(_f))
SQUIRREL_API SQInteger sqstd_fclose(SQFILE);

SQUIRREL_API SQRESULT sqstd_createfile(HSQUIRRELVM v, SQUserPointer file,SQBool own);
SQUIRREL_API SQRESULT sqstd_getfile(HSQUIRRELVM v, SQInteger idx, SQUserPointer *file);
SQUIRREL_API SQRESULT sqstd_opensqfile(HSQUIRRELVM v, const SQChar *filename ,const SQChar *mode);
SQUIRREL_API SQRESULT sqstd_createsqfile(HSQUIRRELVM v, SQFILE file,SQBool own);
SQUIRREL_API SQRESULT sqstd_getsqfile(HSQUIRRELVM v, SQInteger idx, SQFILE *file);

//compiler helpers
SQUIRREL_API SQRESULT sqstd_loadfile(HSQUIRRELVM v,const SQChar *filename,SQBool printerror);
SQUIRREL_API SQRESULT sqstd_dofile(HSQUIRRELVM v,const SQChar *filename,SQBool retval,SQBool printerror);
SQUIRREL_API SQRESULT sqstd_writeclosuretofile(HSQUIRRELVM v,const SQChar *filename);

SQUIRREL_API SQRESULT sqstd_register_iolib(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_SQSTDIO_H_*/

