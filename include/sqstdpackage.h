#ifndef _SQSTD_PACKAGE_H_
#define _SQSTD_PACKAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

// package
SQUIRREL_API SQRESULT sqstd_package_require(HSQUIRRELVM v, const SQChar *package, const SQChar *hint);
SQUIRREL_API SQRESULT sqstd_package_registerfct(HSQUIRRELVM v, const SQChar *package, SQFUNCTION loader_fct);
SQUIRREL_API SQRESULT sqstd_package_getsearchers(HSQUIRRELVM v);
SQUIRREL_API SQRESULT sqstd_register_packagelib(HSQUIRRELVM v);

// path
typedef SQInteger (*sqstd_package_search_fct)(HSQUIRRELVM v, void *user);
SQInteger sqstd_package_searchcbk(HSQUIRRELVM v, const SQChar *what, const SQChar *path, const SQChar *suffixes, sqstd_package_search_fct fct, void *user);
SQInteger sqstd_package_searchfile(HSQUIRRELVM v, const SQChar *what, const SQChar *path, const SQChar *suffixes);

// nut
SQUIRREL_API SQRESULT sqstd_package_nutaddpath(HSQUIRRELVM v, const SQChar *path);

#ifdef __cplusplus
}
#endif

#endif // _SQSTD_PACKAGE_H_
