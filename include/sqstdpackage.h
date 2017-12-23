#ifndef _SQT_PACKAGE_H_
#define _SQT_PACKAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

extern SQUIRREL_API_VAR const struct tagSQRegClass _sqstd_dynlib_decl;
#define SQSTD_DYNLIB_TYPE_TAG ((SQUserPointer)(SQHash)&_sqstd_dynlib_decl)

typedef void *SQDYNLIB;

SQUIRREL_API SQDYNLIB sqstd_dynlib_rawload( const SQChar *path);
SQUIRREL_API SQUserPointer sqstd_dynlib_rawsym( SQDYNLIB lib, const SQChar *name);
SQUIRREL_API SQBool sqstd_dynlib_rawclose( SQDYNLIB lib);

SQUIRREL_API SQRESULT sqstd_dynlib_error( HSQUIRRELVM v);
SQUIRREL_API SQRESULT sqstd_dynlib_load(HSQUIRRELVM v, const SQChar *path, SQBool is_private, SQDYNLIB *plib);
SQUIRREL_API SQRESULT sqstd_dynlib_sym(HSQUIRRELVM v,SQDYNLIB lib, const SQChar *sym_name, SQUserPointer *psym);
SQUIRREL_API SQRESULT sqstd_dynlib_register(HSQUIRRELVM v, SQDYNLIB lib, const SQChar *path);

SQUIRREL_API SQRESULT sqstd_package_require(HSQUIRRELVM v, const SQChar *package);
SQUIRREL_API SQRESULT sqstd_package_registerfct( HSQUIRRELVM v, const SQChar *package, SQFUNCTION fct);

SQUIRREL_API SQRESULT sqstd_register_packagelib(HSQUIRRELVM v);

#ifdef __cplusplus
}
#endif

#endif // _SQT_PACKAGE_H_
