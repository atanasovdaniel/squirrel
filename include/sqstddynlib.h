#ifndef _SQSTD_DYNLIB_H_
#define _SQSTD_DYNLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

SQUIRREL_API SQUserPointer _sqstd_dynlib_type_tag(void);
#define SQSTD_DYNLIB_TYPE_TAG _sqstd_dynlib_type_tag()

typedef void *SQDYNLIB;

SQUIRREL_API SQDYNLIB sqstd_dynlib_rawload( const SQChar *path);
SQUIRREL_API SQUserPointer sqstd_dynlib_rawsym( SQDYNLIB lib, const SQChar *name);
SQUIRREL_API SQBool sqstd_dynlib_rawclose( SQDYNLIB lib);

SQUIRREL_API SQRESULT sqstd_dynlib_error( HSQUIRRELVM v);
SQUIRREL_API SQRESULT sqstd_dynlib_load(HSQUIRRELVM v, const SQChar *path, SQBool is_private, SQDYNLIB *plib);
SQUIRREL_API SQRESULT sqstd_dynlib_sym(HSQUIRRELVM v,SQDYNLIB lib, const SQChar *sym_name, SQUserPointer *psym);
SQUIRREL_API SQRESULT sqstd_dynlib_register(HSQUIRRELVM v, SQDYNLIB lib, const SQChar *path);

SQUIRREL_API SQRESULT sqstd_register_dynlib(HSQUIRRELVM v);

#ifdef __cplusplus
}
#endif

#endif // _SQSTD_PACKAGE_H_
