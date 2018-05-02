#ifndef _SQSTD_PACKAGE_H_
#define _SQSTD_PACKAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagSQSTDPackageList {
    const SQChar *name;
    SQFUNCTION fct;
} SQSTDPackageList;

SQUIRREL_API void sqstd_package_addbuiltins( const SQSTDPackageList *list);

SQUIRREL_API SQRESULT sqstd_package_require(HSQUIRRELVM v, const SQChar *package);
SQUIRREL_API SQRESULT sqstd_package_registerfct( HSQUIRRELVM v, const SQChar *package, SQFUNCTION fct);

SQUIRREL_API SQRESULT sqstd_package_addpath( HSQUIRRELVM v, const SQChar *path);
SQUIRREL_API SQRESULT sqstd_package_addcpath( HSQUIRRELVM v, const SQChar *path);

SQUIRREL_API SQRESULT sqstd_register_packagelib(HSQUIRRELVM v);

#ifdef __cplusplus
}
#endif

#endif // _SQSTD_PACKAGE_H_
