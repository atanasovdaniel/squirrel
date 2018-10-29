#ifndef _SQSTD_PACKAGE_H_
#define _SQSTD_PACKAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

// package
SQUIRREL_API SQRESULT sqstd_package_require(HSQUIRRELVM v, const SQChar *package);
SQUIRREL_API SQRESULT sqstd_package_registerfct(HSQUIRRELVM v, const SQChar *package, SQFUNCTION fct);
SQUIRREL_API SQRESULT sqstd_package_getsearchers(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sqstd_register_packagelib(HSQUIRRELVM v);

// path
SQUIRREL_API SQRESULT sqstd_package_replacechar(HSQUIRRELVM v, SQChar the_char);
SQUIRREL_API SQRESULT sqstd_package_pathadd( HSQUIRRELVM v, SQInteger path_idx);
SQUIRREL_API SQRESULT sqstd_package_pathaddlist( HSQUIRRELVM v, SQInteger path_idx);
SQUIRREL_API SQInteger sqstd_package_pathsearch(HSQUIRRELVM v, SQInteger path_idx);

// nut
SQUIRREL_API SQRESULT sqstd_package_nutaddpath(HSQUIRRELVM v, const SQChar *path);

#ifdef __cplusplus
}
#endif

#endif // _SQSTD_PACKAGE_H_
