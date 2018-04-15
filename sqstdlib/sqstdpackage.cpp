/*
Copyright (c) 2017-2018 Daniel Atanasov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <stdio.h>
#include <string.h>
#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdio.h>
#include <sqstdsystem.h>
#ifdef SQ_ENABLE_DYNLIB
#include <sqstddynlib.h>
#endif // SQ_ENABLE_DYNLIB

#include <sqstdpackage.h>

#ifdef _WIN32
#define SQ_PACKAGE_DEFAULT_PATH     _SC(".\\?.nut;.\\?\\init.nut")
#define SQ_PACKAGE_DEFAULT_CPATH    _SC(".\\?.dll")
#else // _WIN32
#define SQ_PACKAGE_DEFAULT_PATH     _SC("./?.nut;./?/init.nut")
#define SQ_PACKAGE_DEFAULT_CPATH    _SC("./?.so")
#endif // _WIN32

#define LOAD_FCT_TEMPLATE           _SC("sqload_?")

#define SQ_PACKAGE_PATH_ENV         _SC("SQ_PATH")
#define SQ_PACKAGE_CPATH_ENV        _SC("SQ_CPATH")

#ifdef _WIN32
#define DIR_SEP_CHAR        _SC('\\')
#else // _WIN32
#define DIR_SEP_CHAR        _SC('/')
#endif // _WIN32

#define PATH_SEP_CHAR       _SC(';')
#define PATH_REPLACE_CHAR   _SC('?')

#define LOADED_TABLE_NAME   _SC("_PACKAGE_LOADED")
#define CLOADED_TABLE_NAME  _SC("_PACKAGE_CLOADED")
#define PATH_VAR_NAME       _SC("_PACKAGE_PATH")
#define CPATH_VAR_NAME      _SC("_PACKAGE_CPATH")
#define CCHAIN_VAR_NAME     _SC("_PACKAGE_CCHAIN")

static char PACKAGE_ID;
static char SEARCHERS_ARRAY_ID;
static char LOADED_TABLE_ID;

static const SQSTDPackageList *builtin_packages = 0;

/* ====================================
		package
==================================== */

/* ----------------
    paths
---------------- */

static SQInteger replace_char(HSQUIRRELVM v, SQChar the_char)
{
    // original, repl
    const SQChar *src, *repl, *p;
    SQChar *res, *r;
    SQInteger repl_len;
    SQInteger res_len;

    sq_getstringandsize(v,-1,&repl,&repl_len);
    sq_getstring(v,-2,&src);

    p = src; res_len = 0;
    while( *p != _SC('\0')) {
        if( *p == the_char) {
            res_len += repl_len;
        }
        else {
            res_len++;
        }
        p++;
    }
    r = res = sq_getscratchpad(v, sizeof(SQChar)*res_len);
    p = src;
    while( *p != _SC('\0')) {
        if( *p == the_char) {
            memcpy( r, repl, sizeof(SQChar)*repl_len);
            r += repl_len;
        }
        else {
            *r++ = *p;
        }
        p++;
    }
    sq_pushstring(v,res,res_len);   // original, repl, result
    sq_remove(v,-2);                // original, result
    sq_remove(v,-2);                // result
    return 1;
}

struct GetStrElems
{
    const SQChar *from;
    const SQChar *to;
    const SQChar delimer;

    GetStrElems( const SQChar *from, SQChar delimer) : from(from), to(from), delimer(delimer) {}

    GetStrElems( HSQUIRRELVM v, SQInteger idx, SQChar delimer) : delimer(delimer)
    {
        sq_getstring(v,idx,&from);
        to = from;
    }

    SQInteger next( void)
    {
        while(1) {
            if( *to == _SC('\0')) return 0;
            from = to;
            while( *from == delimer) from++;
            to = from;
            while( (*to != _SC('\0')) && (*to != delimer)) to++;
            if( from != to) return 1;
        }
    }

    SQInteger push_next(HSQUIRRELVM v)
    {
        SQInteger r = next();
        if( r == 1) {
            sq_pushstring(v,from,to-from);
        }
        return r;
    }
};

static SQFILE search_path(HSQUIRRELVM v)
{
    GetStrElems els(v, -1, PATH_SEP_CHAR);              // path_to_search
    while( els.push_next(v))                            // path_to_search, filename
    {
        const SQChar *filename;
        sq_getstring(v,-1,&filename);
#ifdef _DEBUG
        scprintf("Try:%s\n",filename);
#endif // _DEBUG
        SQFILE file = sqstd_fopen(filename,_SC("rb"));
        if( file) {
            sq_remove(v,-2);                            // [path_to_search], filename
            return file;                                // filename
        }
        sq_poptop(v);                                   // path_to_search, [filename]
    }
    sq_poptop(v);                                       // [path_to_search]
    return 0;                                           //
}

/* ----------------
    searchers
---------------- */

void sqstd_package_addbuiltins( const SQSTDPackageList *list)
{
    builtin_packages = list;
}

static SQInteger package_search_builtin(HSQUIRRELVM v)
{
    if( builtin_packages) {
        const SQChar *pkg_name;
        const SQSTDPackageList *builtin = builtin_packages;
        sq_getstring(v,2,&pkg_name);
        while( builtin->name) {
            if( scstrcmp(pkg_name,builtin->name) == 0) {
                sq_newclosure(v,builtin->fct,0);             // closure
                return 1;
            }
            builtin++;
        }
    }
    return 0;
}

static const SQChar _dots_repl[2] = { DIR_SEP_CHAR, _SC('\0') };

static SQInteger package_search_nut(HSQUIRRELVM v)
{
    // root, pkg_name
    SQFILE file;
    sq_pushregistrytable(v);                            // registry
    sq_pushuserpointer(v,&PACKAGE_ID);                  // registry, PACKAGE_ID
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // registry, package
        sq_poptop(v);                                   // [registry]
        return SQ_ERROR;                                //
    }
    sq_remove(v,-2);                                    // [registry], package
    sq_pushstring(v, _SC("PATH"),-1);                   // package, "PATH"
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // package, path
        sq_poptop(v);                                   // [package]
        return SQ_ERROR;                                //
    }
    sq_remove(v,-2);                                    // [package], path
    sq_push(v,2);                                       // path, pkg_name
    sq_pushstring(v,_dots_repl,-1);                     // path, pkg_name, dots_repl
    replace_char(v, _SC('.'));                          // path, search_name
    replace_char(v, PATH_REPLACE_CHAR);                 // path_to_search
    file = search_path(v);
    if( file) {                                         // filename
        const SQChar *filename;
        SQRESULT r;
        sq_getstring(v,-1,&filename);
        r = sqstd_loadstream(v,file,filename,SQTrue);
        sqstd_fclose( file);
        if(SQ_FAILED(r)) {
            sq_poptop(v);                               // [path_to_search, filename]
            return sq_throwerror(v,_SC("failed to load package"));  //
        }
        sq_remove(v,-2);                                // [filename], closure
        return 1;
    }
    return 0;
}

#ifdef SQ_ENABLE_DYNLIB
static SQInteger package_search_c(HSQUIRRELVM v)
{
    // root, pkg_name
    SQFILE file = 0;
    const SQChar *pkg_name;
    SQInteger pkg_dot;
    sq_pushregistrytable(v);                            // registry
    sq_pushuserpointer(v,&PACKAGE_ID);                  // registry, PACKAGE_ID
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // registry, package
        sq_poptop(v);                                   // [registry]
        return SQ_ERROR;                                //
    }
    sq_remove(v,-2);                                    // [registry], package
    sq_pushstring(v, _SC("CPATH"),-1);                  // package, "CPATH"
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // package, cpath
        sq_poptop(v);                                   // [package]
        return SQ_ERROR;                                //
    }
    sq_remove(v,-2);                                    // [package], cpath
    sq_getstringandsize(v,2,&pkg_name,&pkg_dot);
	do {												// cpath
		sq_push(v,-1);									// cpath, cpath
        sq_pushstring(v, pkg_name, pkg_dot);            // cpath, cpath, pkg_name
        sq_pushstring(v,_dots_repl,-1);                 // cpath, cpath, pkg_name, dots_repl
        replace_char(v, _SC('.'));                      // cpath, cpath, search_name
        replace_char(v, PATH_REPLACE_CHAR);             // cpath, path_to_search
        file = search_path(v);
		if( file) break;								// cpath, filename
		pkg_dot--;
		while((pkg_dot > 0) && (pkg_name[pkg_dot] != _SC('.'))) pkg_dot--;
	} while( pkg_dot > 0);
	if( file) {											// cpath, filename
        SQDYNLIB lib;
        const SQChar *filename;
        const SQChar *sym_name;
        SQUserPointer fct = 0;
        sqstd_fclose( file);
		sq_remove(v,-2);								// [cpath], filename
        sq_getstring(v,-1,&filename);                   // filename
        if(SQ_FAILED(sqstd_dynlib_load(v,filename,SQFalse,&lib))) {
            sq_poptop(v);                               // [filename]
            return SQ_ERROR;
        }
        sq_poptop(v);                                   // [filename]
        sq_pushstring(v, LOAD_FCT_TEMPLATE, -1);        // fct_template
		sq_push(v,2);									// fct_template, pkg_name
		sq_pushstring(v,_SC("_"),-1);					// fct_template, pkg_name, "_"
        replace_char(v, _SC('.'));                      // fct_template, pkg_name_nodot
        replace_char(v, _SC('?'));                      // pkg_fct
        sq_getstring(v,-1,&sym_name);
        if(SQ_FAILED(sqstd_dynlib_sym(v, lib, sym_name, &fct))) {
            sq_poptop(v);                               // [pkg_fct]
            return SQ_ERROR;
        }
        sq_newclosure(v,(SQFUNCTION)fct,0);             // pkg_fct, closure
        sq_setnativeclosurename(v,-1,sym_name);
        sq_remove(v,-2);                                // [pkg_fct], closure
        return 1;
	}
	else {												// cpath
		sq_poptop(v);
		return 0;
	}
}
#endif // SQ_ENABLE_DYNLIB

static SQInteger run_searchers(HSQUIRRELVM v, SQInteger idx)
{
    sq_pushregistrytable(v);                            // registry
    sq_pushuserpointer(v,&SEARCHERS_ARRAY_ID);          // registry, SEARCHERS_ID
    if(SQ_FAILED(sq_rawget(v,-2))) return SQ_ERROR;     // registry, searchers_array
    sq_remove(v,-2);                                    // [registry], searchers_array
    sq_pushnull(v);								        // searchers_array, iterator
    while(SQ_SUCCEEDED(sq_next(v,-2))) {   		        // searchers_array, iterator, key, value
        sq_remove(v,-2);                                // searchers_array, iterator, [key], search_fct
        sq_pushroottable(v);                            // searchers_array, iterator, search_fct, root
        sq_push(v,idx);                                 // searchers_array, iterator, search_fct, root, package_name
        if(SQ_FAILED(sq_call(v,2,SQTrue,SQFalse))) {    // searchers_array, iterator, search_fct, [root, package_name]
            sq_pop(v,3);                                // [searchers_array, iterator, search_fct]
            return SQ_ERROR;
        }
        if(sq_gettype(v,-1) != OT_NULL) {               // searchers_array, iterator, search_fct, load_fct
            sq_remove(v,-2);                            // searchers_array, iterator, [search_fct], load_fct
            sq_remove(v,-2);                            // searchers_array, [iterator], load_fct
            sq_remove(v,-2);                            // [searchers_array], load_fct
            return 1;
        }
        sq_pop(v,2);                                    // searchers_array, iterator, [search_fct, load_fct]
    }
    sq_pop(v,2);                                        // [searchers_array, iterator]
    return 0;
}

static SQInteger load_package(HSQUIRRELVM v, SQInteger idx)
{
    SQRESULT is_found = run_searchers(v,idx);
    if(SQ_FAILED(is_found)) return SQ_ERROR;
    if(is_found) {
        sq_pushroottable(v);                            // load_fct, root
        if(SQ_SUCCEEDED(sq_call(v,1,SQTrue,SQTrue))) {  // load_fct, [root]
            sq_remove(v,-2);                            // [load_fct], package
            return 1;                                   // package
        }
        else {                                          // load_fct
            sq_pop(v,1);                                // [load_fct]
            return sq_throwerror(v,_SC("failed to load package")); //
        }
    }
    else {
        // package is not found
        return 0;                                       //
    }
}

/* ----------------
    require
---------------- */

static SQChar _recursive_require[] = _SC("recursive require");

static SQInteger require( HSQUIRRELVM v, SQInteger idx)
{
    if( idx < 0) idx = sq_gettop(v) + idx + 1;
    sq_pushregistrytable(v);                            // registry
    sq_pushuserpointer(v,&LOADED_TABLE_ID);             // registry, LOADED_ID
    if(SQ_FAILED(sq_rawget(v,-2))) return SQ_ERROR;     // registry, loaded_table
    sq_remove(v,-2);                                    // [registry], loaded_table
    sq_push(v,idx);                                     // loaded_table, pkg_name
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // loaded_table
        SQInteger res;
        // mark package for recursion
        sq_push(v,idx);                                 // loaded_table, pkg_name
        sq_pushuserpointer(v,(SQUserPointer)_recursive_require);       // loaded_table, name, val
        sq_rawset(v,-3);                                // loaded_table
        sq_push(v,idx);                                 // loaded_table, pkg_name
        // then load it
//        res = search_package(v, idx);
        res = load_package(v, idx);
        if( res == 1) {
            // package is found
            HSQOBJECT tmp;                              // loaded_table, pkg_name, package
            sq_resetobject(&tmp);
            sq_getstackobj(v,-1,&tmp);
            sq_rawset(v,-3);                            // loaded_table, [pkg_name, package]
            sq_poptop(v);                               // [loaded_table]
            sq_pushobject(v,tmp);                       // package
            return 1;                                   // package
        }
        else {
            // res == 0 - package not found
            // res == SQ_ERROR - package load failed
            // remove recursion mark
            sq_rawdeleteslot(v,-2,SQFalse);             // loaded_table, [pkg_name]
            sq_pop(v,1);                                // [loaded_table]
            return res;                                 //
        }
    }
    else {
        // package is allready loaded, check for recursion
        if( sq_gettype(v,-1) == OT_USERPOINTER) {       // loaded_table, package
            SQUserPointer p;
            sq_getuserpointer(v,-1,&p);
            if( p == (SQUserPointer)_recursive_require) {
                // recursion detected, remove recursion mark
                sq_poptop(v);                           // loaded_table, [package]
                sq_push(v,idx);                         // loaded_table, pkg_name
                sq_rawdeleteslot(v,-2,SQFalse);         // loaded_table, [pkg_name]
                sq_poptop(v);                           // [loaded_table]
                return sq_throwerror(v,_recursive_require); //
            }
        }
        sq_remove(v,-2);                                // [loaded_table], package
        return 1;                                       // package
    }
}

SQRESULT sqstd_package_registerfct( HSQUIRRELVM v, const SQChar *package, SQFUNCTION fct)
{
    sq_pushregistrytable(v);                            // registry
    sq_pushuserpointer(v,&LOADED_TABLE_ID);             // registry, LOADED_ID
    if(SQ_FAILED(sq_rawget(v,-2))) return SQ_ERROR;     // registry, loaded_table
    sq_remove(v,-2);                                    // [registry], loaded_table
    sq_pushstring(v,package,-1);                        // loaded_table, package_name
    if(SQ_FAILED(sq_rawget(v,-2)))                      // loaded_table, package_name
    {
        sq_newclosure(v,fct,0);                             // loaded_table, fct
        sq_pushroottable(v);                                // loaded_table, fct, root
        if(SQ_SUCCEEDED(sq_call(v,1,SQTrue,SQTrue))) {      // loaded_table, fct, [root], package
            sq_remove(v,-2);                                // loaded_table, [fct], package
            sq_pushstring(v,package,-1);                    // loaded_table, package, package_name
            sq_push(v,-2);                                  // loaded_table, package, package_name, package
            sq_rawset(v,-4);                                // loaded_table, package, [package_name, package]
            sq_remove(v,-2);                                // [loaded_table], package
            return 1;                                       // package
        }
        else {
            sq_pop(v,2);                                    // [loaded_table, fct]
            return SQ_ERROR;                                //
        }
    }
    else {
        sq_remove(v,-2);                                    // [loaded_table], package
        return 1;                                           // package
    }
}

SQRESULT sqstd_package_require(HSQUIRRELVM v, const SQChar *package)
{
    SQInteger res;
    sq_pushstring(v,package,-1);
    res = require(v, -1);
    if( res == 1)
        return SQ_OK;
    else if( res == 0)
        return sq_throwerror(v,_SC("required package not found"));
    else
        return SQ_ERROR;
}

/* ----------------
---------------- */

static SQInteger _g_package_require( HSQUIRRELVM v)
{
    SQInteger res;
    res = require(v, 2);
    if( res == 1)
        return 1;
    else if( res == 0)
        return sq_throwerror(v,_SC("required package not found"));
    else
        return SQ_ERROR;
}

static SQInteger _package_replacechar(HSQUIRRELVM v)
{
    SQInteger the_char;
    sq_getinteger(v,4,&the_char);
    sq_poptop(v);
    replace_char(v,(SQChar)the_char);
    return 1;
}

static SQInteger _package_pathsplit(HSQUIRRELVM v)
{
    SQInteger delimer;
    sq_getinteger(v,3,&delimer);
    sq_poptop(v);
    GetStrElems els(v,2,delimer);
    sq_newarray(v,0);               // array
    while( els.push_next(v))        // array, elem
    {
        sq_arrayappend(v,-2);       // array
    }
    return 1;
}

static SQInteger _package_pathsearch(HSQUIRRELVM v)
{
    SQFILE file = search_path(v);
    if( file) {
        sqstd_fclose( file);
    }
    else {
        sq_pushnull(v);
    }
    return 1;
}

#define _DECL_GLOBALPACKAGE_FUNC(name,nparams,typecheck) {_SC(#name),_g_package_##name,nparams,typecheck}
static const SQRegFunction g_package_funcs[]={
    _DECL_GLOBALPACKAGE_FUNC(require,2,_SC(".s")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

#define _DECL_PACKAGE_FUNC(name,nparams,typecheck) {_SC(#name),_package_##name,nparams,typecheck}
static const SQRegFunction package_funcs[]={
    _DECL_PACKAGE_FUNC(pathsplit,3,_SC(".si")),
    _DECL_PACKAGE_FUNC(pathsearch,2,_SC(".s")),
    _DECL_PACKAGE_FUNC(replacechar,4,_SC(".ssi")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

static void fill_searchers(HSQUIRRELVM v)
{// array
    sq_newclosure(v,package_search_builtin,0);  // array, closure
    sq_setnativeclosurename(v,-1,_SC("package_search_builtin"));
    sq_arrayappend(v,-2);                   // array, [closure]
    sq_newclosure(v,package_search_nut,0);  // array, closure
    sq_setnativeclosurename(v,-1,_SC("search_package_nut"));
    sq_arrayappend(v,-2);                   // array, [closure]
#ifdef SQ_ENABLE_DYNLIB
    sq_newclosure(v,package_search_c,0);    // array, closure
    sq_setnativeclosurename(v,-1,_SC("search_package_c"));
    sq_arrayappend(v,-2);                   // array, [closure]
#endif // SQ_ENABLE_DYNLIB
}

SQRESULT sqstd_register_packagelib(HSQUIRRELVM v)
{
    sq_pushregistrytable(v);                    // registry
    // package = {...}
    sq_newtable(v);                             // registry, package
    sqstd_registerfunctions(v, package_funcs);  // registry, package

    // registry.PACKAGE_ID = package
    sq_pushuserpointer(v,&PACKAGE_ID);           // registry, package, PACKAGE_ID
    sq_push(v,-2);                              // registry, package, PACKAGE_ID, package
    sq_rawset(v,-4);                            // registry, package, [PACKAGE_ID, package]

    sq_pushuserpointer(v,&LOADED_TABLE_ID);      // registry, package, LOADED_ID
    // LOADED = {}
    sq_newtable(v);                             // registry, package, LOADED_ID, loaded_table
    // LOADED["package"] <- package
    sq_pushstring(v,_SC("package"),-1);         // registry, package, LOADED_ID, loaded_table, "package"
    sq_push(v,-4);                              // registry, package, LOADED_ID, loaded_table, "package", package
    sq_rawset(v,-3);                            // registry, package, LOADED_ID, loaded_table, ["package", package]
    // package.LOADED <- LOADED
    sq_pushstring(v,_SC("LOADED"),-1);          // registry, package, LOADED_ID, loaded_table, "LOADED"
    sq_push(v,-2);                              // registry, package, LOADED_ID, loaded_table, "LOADED", loaded_table
    sq_rawset(v,-5);                            // registry, package, LOADED_ID, loaded_table, ["LOADED", loaded_table]
    // registry._LOADED_ID <- _LOADED
    sq_rawset(v,-4);                            // registry, package, [LOADED_ID, loaded_table]

    sq_pushuserpointer(v,&SEARCHERS_ARRAY_ID);  // registry, package, SEARCHERS_ID
    sq_newarray(v,0);                           // registry, package, SEARCHERS_ID, searchers_array
    fill_searchers(v);
    sq_pushstring(v,_SC("SEARCHERS"),-1);       // registry, package, SEARCHERS_ID, searchers_array, "SEARCHERS"
    sq_push(v,-2);                              // registry, package, SEARCHERS_ID, searchers_array, "SEARCHERS", searchers_array
    // package.SEARCHERS = searchers_array
    sq_rawset(v,-5);                            // registry, package, SEARCHERS_ID, searchers_array, ["SEARCHERS", searchers_array]
    // registry.SEARCHERS_ID = searchers_array
    sq_rawset(v,-4);                            // registry, package, [SEARCHERS_ID, searchers_array]

    sq_remove(v,-2);                            // [registry], package

    sq_pushstring(v,_SC("PATH"),-1);            // package, "PATH"
    sq_pushstring(v,SQ_PACKAGE_PATH_ENV,-1);    // package, "PATH", "SQ_PATH"
    sqstd_system_getenv(v);                     // package, "PATH", ["SQ_PATH"]
    if( sq_gettype(v,-1) == OT_NULL) {          // package, "PATH", path_value
        sq_poptop(v);                           // package, "PATH", [path_value]
        sq_pushstring(v,SQ_PACKAGE_DEFAULT_PATH,-1); // package, "PATH", def_path_value
    }
    // package.PATH <- path_value
    sq_rawset(v,-3);                            // package, ["PATH", path_value]

#ifdef SQ_ENABLE_DYNLIB
    sq_pushstring(v,_SC("CPATH"),-1);           // package, "CPATH"
    sq_pushstring(v,SQ_PACKAGE_CPATH_ENV,-1);   // package, "CPATH", "SQ_CPATH"
    sqstd_system_getenv(v);                     // package, "CPATH", "SQ_PATH"
    if( sq_gettype(v,-1) == OT_NULL) {          // package, "CPATH", cpath_value
        sq_poptop(v);                           // package, "CPATH", [cpath_value]
        sq_pushstring(v,SQ_PACKAGE_DEFAULT_CPATH,-1); // package, "CPATH", def_cpath_value
    }
    // package.CPATH <- cpath_value
    sq_rawset(v,-3);                            // package, ["CPATH", cpath_value]
#endif // SQ_ENABLE_DYNLIB

    sq_poptop(v);                               // [package]
	sqstd_registerfunctions(v, g_package_funcs);
    
#ifdef SQ_ENABLE_DYNLIB
    if( SQFAILED(sqstd_register_dynlib(v))) return SQ_ERROR;
#endif // SQ_ENABLE_DYNLIB
    
    return SQ_OK;
}
