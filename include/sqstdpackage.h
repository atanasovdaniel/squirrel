#ifndef _SQT_PACKAGE_H_
#define _SQT_PACKAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

SQUIRREL_API SQRESULT sqstd_require(HSQUIRRELVM v, const SQChar *package);
SQUIRREL_API SQRESULT sqstd_require_fct( HSQUIRRELVM v, const SQChar *package, SQFUNCTION fct);

SQUIRREL_API SQRESULT sqstd_register_packagelib(HSQUIRRELVM v);

#ifdef __cplusplus
}
#endif

#endif // _SQT_PACKAGE_H_
