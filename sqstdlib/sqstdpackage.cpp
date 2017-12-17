/*
Copyright (c) 2017 Daniel Atanasov

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

#include <string.h>
#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdio.h>
#include <sqstdsystem.h>
#include <sqstdpackage.h>
#include <stdio.h>          // !!! debug

#define SQ_PACKAGE_DEFAULT_PATH     _SC("./?.nut;./?/init.nut")
#define SQ_PACKAGE_DEFAULT_CPATH    _SC("")

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
#define CLOADED_TABLE_NAME _SC("_PACKAGE_CLOADED")
#define PATH_VAR_NAME       _SC("_PACKAGE_PATH")
#define CPATH_VAR_NAME      _SC("_PACKAGE_CPATH")
#define CCHAIN_VAR_NAME     _SC("_PACKAGE_CCHAIN")

/* ====================================
		dynamic library
==================================== */

static HSQMEMBERHANDLE dynlib__path_handle;

#ifdef _WIN32
/* ----------------
    WIN32
---------------- */

#include <windows.h>

typedef void *DL_LIB;

static DL_LIB _dllib_load( const SQChar *path)
{
    return LoadLibrary( path);
}

static SQUserPointer _dllib_sym( DL_LIB lib, const SQChar *name)
{
    return (SQUserPointer)GetProcAddress( lib, name);
}

static SQInteger _dllib_close( DL_LIB lib)
{
    return FreeLibrary( lib) ? SQTrue : SQFlase;
}

#define _LDLIB_ERR_LEN  256

static SQRESULT _dllib_error( HSQUIRRELVM v)
{
    DWORD err = GetLastError();
    DWORD r;
    SQChar *buf;
    buf = sq_getscratchpad( _LDLIB_ERR_LEN * sizeof(SQChar));
    r = FormatMessage(
            FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, err, 0,
            buff, _LDLIB_ERR_LEN,
            NULL);
    if( r) {
        sq_pushstring(v,buf,r);
    }
    else {
        scsprintf(buf,_LDLIB_ERR_LEN, "system error: %d" _PRINT_INT_FMT, (SQInteger)err);
        sq_pushstring(v,buf,-1);
    }
    return SQ_OK;
}

#else // _WIN32

/* ----------------
    UNIX / POSIX
---------------- */

#include <dlfcn.h>

typedef void *DL_LIB;

static DL_LIB _dllib_load( const SQChar *path)
{
    DL_LIB lib = dlopen( path, RTLD_NOW | RTLD_GLOBAL);
printf("load_dynlib(%s) %p\n", path, lib);
    return lib;
}

static SQUserPointer _dllib_sym( DL_LIB lib, const SQChar *name)
{
    return (SQUserPointer)dlsym( lib, name);
}

static SQBool _dllib_close( DL_LIB lib)
{
printf("_dllib_close(%p)\n", lib);
    return (SQBool)dlclose( lib);
}

static SQRESULT _dllib_error( HSQUIRRELVM v)
{
    sq_pushstring(v, dlerror(), -1);
    return SQ_OK;
}

#endif // _WIN32

/* ----------------
---------------- */

struct dllib_chain_t {
    struct dllib_chain_t *prev;
    DL_LIB lib;
};

static SQRESULT load_dynlib(HSQUIRRELVM v, DL_LIB *plib, const SQChar *path)
{
    DL_LIB lib;
    lib = _dllib_load( path);
    if( lib != 0) {
        *plib = lib;
        return SQ_OK;
    }
    else {
        _dllib_error(v);            // error
        return sq_throwobject(v);
    }
}

static SQInteger __dynlib_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    _dllib_close( (DL_LIB)p);
    return 1;
}

static SQInteger _dllib_chain_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    dllib_chain_t *chain = (dllib_chain_t*)*(SQUserPointer*)p;
    while( chain) {
        dllib_chain_t *prev = chain->prev;
        _dllib_close( chain->lib);
        sq_free( chain, sizeof(dllib_chain_t));
        chain = prev;
    }
    return 0;
}

static SQRESULT add_to_chain(HSQUIRRELVM v, DL_LIB lib, const SQChar *path)
{
    SQUserPointer udata;
    SQInteger r;
    sq_pushregistrytable(v);                            // registry
    sq_pushstring(v,CCHAIN_VAR_NAME,-1);                // registry, "_CCHAIN"
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // registry, ["_CCHAIN"]
        sq_poptop(v);                                   // [registry]
        return SQ_ERROR;
    }
    sq_getuserdata(v,-1,&udata,0);                      // registry, udata
    sq_poptop(v);                                       // registry, [udata]
    sq_pushstring(v, CLOADED_TABLE_NAME, -1);           // registry, "_CLOADED"
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // registry, ["_CLOADED"]
        sq_poptop(v);                                   // [registry]
        return SQ_ERROR;
    }
    sq_remove(v,-2);                                    // [registry], cloaded_table
    sq_pushstring(v,path,-1);                           // cloaded_table, "path"
    sq_pushuserpointer(v,lib);                          // cloaded_table, "path", up_lib
    r = sq_rawset(v,-3);                                // cloaded_table, ["path", up_lib]
    sq_poptop(v);                                       // [cloaded_table]
    if(SQ_SUCCEEDED(r)) {
        dllib_chain_t **chain = (dllib_chain_t**)udata;
        dllib_chain_t *elem = (dllib_chain_t*)sq_malloc(sizeof(dllib_chain_t));
        elem->prev = *chain;
        elem->lib = lib;
        *chain = elem;
    }
    return r;
}

static SQRESULT dynlib_load(HSQUIRRELVM v, const SQChar *path, SQBool is_private)
{
    DL_LIB lib = 0;
    if(!is_private) {
        sq_pushregistrytable(v);                        // registry
        sq_pushstring(v, CLOADED_TABLE_NAME, -1);       // registry, "_CLOADED"
        if(SQ_FAILED(sq_rawget(v,-2))) {                // registry, ["_CLOADED"]
            sq_poptop(v);                               // [registry]
            return SQ_ERROR;
        }
        sq_remove(v,-2);                                // [registry], cloaded_table
        sq_pushstring(v,path,-1);                       // cloaded_table, path
        if(SQ_SUCCEEDED(sq_rawget(v,-2))) {             // cloaded_table, [path]
            sq_remove(v,-2);                            // [cloaded_table], up_library
            return SQ_OK;
        }
        else {
            sq_remove(v,-2);                            // [cloaded_table]
        }
    }
    if(SQ_FAILED(load_dynlib(v,&lib,path))) return SQ_ERROR;
    if(!is_private) {
        if(SQ_FAILED(add_to_chain(v,lib,path))) return SQ_ERROR;
    }
    sq_pushuserpointer(v,lib);                          // up_library
    return SQ_OK;
}

static SQInteger find_symbol(HSQUIRRELVM v,DL_LIB lib)
{
                                        // sym_name
    const SQChar *sym_name;
    SQUserPointer sym;
    sq_getstring(v,-1,&sym_name);
    sym = _dllib_sym( lib, sym_name);
    if( sym) {
        sq_poptop(v);                   // [sym_name]
        sq_pushuserpointer(v,sym);      // sym
        return 1;
    }
    else {
        sq_poptop(v);                   // [sym_name]
        _dllib_error(v);                // error
        return sq_throwobject(v);
    }
}

#define SETUP_DYNLIB(v) \
    SQUserPointer self = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,(SQUserPointer)((SQUnsignedInteger)SQSTD_DYNLIB_TYPE_TAG)))) \
        return sq_throwerror(v,_SC("invalid type tag"));

static SQInteger _dynlib_constructor(HSQUIRRELVM v)
{
    SQUserPointer lib;
    const SQChar *path;
    SQBool is_private = SQFalse;
    if( sq_gettop(v) > 2) {
        sq_getbool(v,3,&is_private);
        sq_settop(v,2);
    }
    sq_getstring(v,2,&path);
    if(SQ_FAILED(dynlib_load(v,path,is_private))) return SQ_ERROR;
    sq_getuserpointer(v,-1,&lib);
    sq_poptop(v);
    if(SQ_FAILED(sq_setinstanceup(v,1,lib))) {
        return sq_throwerror(v, _SC("cannot create dynlib instance"));
    }
    sq_pushstring(v,path,-1);
	sq_setbyhandle(v,1,&dynlib__path_handle);

    if( is_private) {
        sq_setreleasehook(v,1,__dynlib_releasehook);
    }
    
    return 0;
}

static SQInteger _dynlib_share(HSQUIRRELVM v)
{
    SETUP_DYNLIB(v);
    if(sq_getreleasehook(v,1) != NULL) {
        const SQChar *path;
        sq_getbyhandle(v,1,&dynlib__path_handle);
        sq_getstring(v,-1,&path);
        if(SQ_FAILED(add_to_chain(v,self,path))) return SQ_ERROR;
        sq_setreleasehook(v,1,NULL);
    }
    return 0;
}

static SQInteger _dynlib_symbol(HSQUIRRELVM v)
{
    SETUP_DYNLIB(v);
    if(SQ_FAILED(find_symbol(v,self))) return SQ_ERROR;
    return 1;
}

static SQInteger _dynlib_cfunction(HSQUIRRELVM v)
{
    SQInteger top = sq_gettop(v);
    const SQChar *name;
    SQUserPointer fct;
    SETUP_DYNLIB(v);
    if( top > 3) {
        sq_settop(v,4);         // lib, name, nparams, typecheck
        top = 4;
    }
    sq_push(v,2);               // lib, name, (...), name
    if(SQ_FAILED(find_symbol(v,self))) return SQ_ERROR;
    sq_getuserpointer(v,-1,&fct);   // lib, name, (...), fct
    sq_poptop(v);                   // lib, name, (...), [fct]
    sq_newclosure(v,(SQFUNCTION)fct,0);
    if( top > 2) {
        SQInteger nparams;
        const SQChar *typemask = 0;
        sq_getinteger(v,3,&nparams);
        if( top > 3) {
            sq_getstring(v,4,&typemask);
        }
        sq_setparamscheck(v, nparams, typemask);
    }
    sq_getstring(v,2,&name);
    sq_setnativeclosurename(v,-1,name);
    return 1;
}

static SQInteger _dynlib__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,_sqstd_dynlib_decl.name,-1);
    return 1;
}

//bindings
#define _DECL_DYNLIB_FUNC(name,nparams,typecheck) {_SC(#name),_dynlib_##name,nparams,typecheck}
static const SQRegFunction _dynlib_methods[] = {
    _DECL_DYNLIB_FUNC(constructor,-2,_SC("xsb")),
    _DECL_DYNLIB_FUNC(share,1,_SC("x")),
    _DECL_DYNLIB_FUNC(symbol,2,_SC("xs")),
    _DECL_DYNLIB_FUNC(cfunction,-2,_SC("xsis")),
    _DECL_DYNLIB_FUNC(_typeof,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

static const SQRegMember _dynlib_members[] = {
	{_SC("path"), &dynlib__path_handle },
	{NULL,NULL}
};

const SQRegClass _sqstd_dynlib_decl = {
	NULL,                   // base_class
    _SC("std_dynlib"),      // reg_name
    _SC("dynlib"),          // name
	_dynlib_members,        // members
	_dynlib_methods,        // methods
};

/* ====================================
		package
==================================== */

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

static const SQChar _dots_repl[2] = { DIR_SEP_CHAR, _SC('\0') };

static SQRESULT search_package_nut( HSQUIRRELVM v, SQInteger idx)
{
    sq_pushregistrytable(v);                            // registry
    sq_pushstring(v, PATH_VAR_NAME, -1);                // registry, "_PATH"
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // registry, path_var
        sq_poptop(v);                                   // [registry]
        return SQ_ERROR;                                //
    }
    sq_remove(v,-2);                                    // [registry], path
    sq_push(v,idx);                                     // path, pkg_name
    sq_pushstring(v,_dots_repl,-1);                     // path, pkg_name, dots_repl
    replace_char(v, _SC('.'));                          // path, search_name
    replace_char(v, PATH_REPLACE_CHAR);                 // path_to_search
    GetStrElems els(v, -1, PATH_SEP_CHAR);
    while( els.push_next(v))                            // path_to_search, filename
    {
        const SQChar *filename;
        sq_getstring(v,-1,&filename);
//printf("[[%s]]",filename);
        SQFILE file = sqstd_fopen(filename,_SC("rb"));
        if( file) {
            SQRESULT r = sqstd_loadstream(v,file,filename,SQTrue);
            sqstd_fclose( file);
            if(SQ_FAILED(r)) {
                sq_pop(v,2);                            // [path_to_search, filename]
                return sq_throwerror(v,_SC("failed to load package"));  //
            }
            sq_remove(v,-2);                            // path_to_search, [filename], closure
            sq_remove(v,-2);                            // [path_to_search], closure
            return 1;                                   // closure
        }
        sq_poptop(v);                                   // path_to_search, [filename]
    }
    sq_poptop(v);                                       // [path_to_search]
    return 0;                                           //
}

static SQInteger search_package(HSQUIRRELVM v, SQInteger idx)
{
    SQRESULT is_found;
    is_found = search_package_nut(v, idx);              //
    if(SQ_FAILED(is_found)) return SQ_ERROR;
    if( is_found) {                                     // closure
        // package is found
        sq_pushroottable(v);                            // closure, root
        if(SQ_SUCCEEDED(sq_call(v,1,SQTrue,SQTrue))) {  // closure, [root]
            sq_remove(v,-2);                            // [closure], package
            return 1;                                   // package
        }
        else {                                          // closure
            sq_pop(v,1);                                // [closure]
            return sq_throwerror(v,_SC("failed to init package")); //
        }
    }
    else {
        // package is not found
        return 0;                                       //
    }
}

static SQChar _recursive_require[] = _SC("recursive require");

static SQInteger require( HSQUIRRELVM v, SQInteger idx)
{
    if( idx < 0) idx = sq_gettop(v) + idx + 1;
    sq_pushregistrytable(v);                            // registry
    sq_pushstring(v, LOADED_TABLE_NAME, -1);            // registry, "_LOADED"
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
        res = search_package(v, idx);
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

SQRESULT sqstd_require_fct( HSQUIRRELVM v, const SQChar *package, SQFUNCTION fct)
{
    sq_pushregistrytable(v);                            // registry
    sq_pushstring(v, LOADED_TABLE_NAME, -1);            // registry, "_LOADED"
    if(SQ_FAILED(sq_rawget(v,-2))) return SQ_ERROR;     // registry, loaded_table
    sq_remove(v,-2);                                    // [registry], loaded_table
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

SQRESULT sqstd_require(HSQUIRRELVM v, const SQChar *package)
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

static SQInteger _package_getpathelems(HSQUIRRELVM v)
{
    SQInteger the_char;
    sq_getinteger(v,3,&the_char);
    sq_poptop(v);
    GetStrElems els(v, 2, the_char);
    sq_newarray(v,0);                                   // array
    while( els.push_next(v))                            // array, elem
    {
        sq_arrayappend(v,-2);                           // array
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
    _DECL_PACKAGE_FUNC(replacechar,4,_SC(".ssi")),
    _DECL_PACKAGE_FUNC(getpathelems,3,_SC(".si")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

SQRESULT sqstd_register_packagelib(HSQUIRRELVM v)
{
    sq_pushregistrytable(v);                    // registry
    // package = {...}
    sq_newtable(v);                             // registry, package
    sqstd_registerfunctions(v, package_funcs);  // registry, package
    
    sq_pushstring(v,LOADED_TABLE_NAME,-1);      // registry, package, "_LOADED"
    // _LOADED = {}
    sq_newtable(v);                             // registry, package, "_LOADED", loaded_table
    // _LOADED["package"] <- package
    sq_pushstring(v,_SC("package"),-1);         // registry, package, "_LOADED", loaded_table, "package"
    sq_push(v,-4);                              // registry, package, "_LOADED", loaded_table, "package", package
    sq_rawset(v,-3);                            // registry, package, "_LOADED", loaded_table, ["package", package]
    // package.LOADED <- _LOADED
    sq_pushstring(v,_SC("LOADED"),-1);          // registry, package, "_LOADED", loaded_table, "LOADED"
    sq_push(v,-2);                              // registry, package, "_LOADED", loaded_table, "LOADED", loaded_table
    sq_rawset(v,-5);                            // registry, package, "_LOADED", loaded_table, ["LOADED", loaded_table]
    // registry._LOADED <- _LOADED
    sq_rawset(v,-4);                            // registry, package, ["_LOADED", loaded_table]

    sq_pushstring(v,CLOADED_TABLE_NAME,-1);    // registry, package, "_CLOADED"
    // _CLOADED = {}
    sq_newtable(v);                             // registry, package, "_CLOADED", cloaded_table
    sq_pushstring(v,_SC("CLOADED"),-1);        // registry, package, "_CLOADED", cloaded_table, "CLOADED"
    sq_push(v,-2);                              // registry, package, "_CLOADED", cloaded_table, "CLOADED", cloaded_table
    // package.CLOADED = _CLOADED
    sq_rawset(v,-5);                            // registry, package, "_CLOADED", cloaded_table, ["CLOADED", cloaded_table]
    // registry.CLOADED = _CLOADED
    sq_rawset(v,-4);                            // registry, package, ["_CLOADED", cloaded_table]
    
    // registry._CCHAIN = udata
    sq_pushstring(v,CCHAIN_VAR_NAME,-1);        // registry, package, "_CCHAIN"
    {
        SQUserPointer *chain = (SQUserPointer*)sq_newuserdata(v, sizeof(SQUserPointer));
        sq_setreleasehook(v,-1,_dllib_chain_releasehook);
        *chain = 0;
    }
    sq_rawset(v,-4);                            // registry, package, "_CCHAIN", udata
    
    sq_pushstring(v,PATH_VAR_NAME,-1);          // registry, package, "_PATH"
    sq_pushstring(v,SQ_PACKAGE_PATH_ENV,-1);    // registry, package, "_PATH", "SQ_PATH"
    sqstd_system_getenv(v);                     // registry, package, "_PATH", "SQ_PATH"
    if( sq_gettype(v,-1) == OT_NULL) {          // registry, package, "_PATH", path_value
        sq_poptop(v);                           // registry, package, "_PATH", [path_value]
        sq_pushstring(v,SQ_PACKAGE_DEFAULT_PATH,-1); // registry, package, "_PATH", def_path_value
    }
    sq_pushstring(v,_SC("PATH"),-1);            // registry, package, "_PATH", path_value, "PATH"
    sq_push(v,-2);                              // registry, package, "_PATH", path_value, "PATH", path_value
    // package.PATH <- path_value
    sq_rawset(v,-5);                            // registry, package, "_PATH", path_value, ["PATH", path_value]
    // registry._PATH <- path_value
    sq_rawset(v,-4);                            // registry, package, ["_PATH", path_value]
    
    sq_pushstring(v,CPATH_VAR_NAME,-1);         // registry, package, "_CPATH"
    sq_pushstring(v,SQ_PACKAGE_CPATH_ENV,-1);   // registry, package, "_CPATH", "SQ_CPATH"
    sqstd_system_getenv(v);                     // registry, package, "_CPATH", "SQ_PATH"
    if( sq_gettype(v,-1) == OT_NULL) {          // registry, package, "_CPATH", cpath_value
        sq_poptop(v);                           // registry, package, "_CPATH", [cpath_value]
        sq_pushstring(v,SQ_PACKAGE_DEFAULT_CPATH,-1); // registry, package, "_CPATH", def_cpath_value
    }
    sq_pushstring(v,_SC("CPATH"),-1);           // registry, package, "_CPATH", path_value, "CPATH"
    sq_push(v,-2);                              // registry, package, "_CPATH", cpath_value, "CPATH", cpath_value
    // package._CPATH <- path_value
    sq_rawset(v,-5);                            // registry, package, "_CPATH", cpath_value, ["CPATH", def_path_value]
    // registry._CPATH <- path_value
    sq_rawset(v,-4);                            // registry, package, ["_CPATH", cpath_value]
    
	if(SQ_FAILED(sqstd_registerclass(v,&_sqstd_dynlib_decl)))
	{
		return SQ_ERROR;
	}
 	sq_poptop(v);

    sq_pop(v,2);                                // [registry, package]
	sqstd_registerfunctions(v, g_package_funcs);
    return SQ_OK;
}
