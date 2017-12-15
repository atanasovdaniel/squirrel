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

#define SQ_PACKAGE_DEFAULT_PATH     _SC("./?.nut;./?/init.nut")
#define SQ_PACKAGE_DEFAULT_CPATH    _SC("")

#define SQ_PACKAGE_PATH_ENV         _SC("SQ_PATH")
#define SQ_PACKAGE_CPATH_ENV        _SC("SQ_CPATH")

#define DIR_SEP_CHAR        _SC('/')

#define PATH_SEP_CHAR       _SC(';')
#define PATH_REPLACE_CHAR   _SC('?')

#define LOADED_TABLE_NAME   _SC("_PACKAGE_LOADED")
#define PATH_VAR_NAME       _SC("_PACKAGE_PATH")
#define CPATH_VAR_NAME      _SC("_PACKAGE_CPATH")

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
    is_found = search_package_nut(v, idx);                  //
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
//    sq_push(v,-2);                              // registry, package, "_LOADED", loaded_table, "_LOADED"
    sq_pushstring(v,_SC("LOADED"),-1);          // registry, package, "_LOADED", loaded_table, "LOADED"
    sq_push(v,-2);                              // registry, package, "_LOADED", loaded_table, "LOADED", loaded_table
    sq_rawset(v,-5);                            // registry, package, "_LOADED", loaded_table, ["LOADED", loaded_table]
    // registry._LOADED <- _LOADED
    sq_rawset(v,-4);                            // registry, package, ["_LOADED", loaded_table]
    
    sq_pushstring(v,PATH_VAR_NAME,-1);          // registry, package, "_PATH"
    sq_pushstring(v,SQ_PACKAGE_PATH_ENV,-1);    // registry, package, "_PATH", "SQ_PATH"
    sqstd_system_getenv(v);                     // registry, package, "_PATH", "SQ_PATH"
    if( sq_gettype(v,-1) == OT_NULL) {          // registry, package, "_PATH", path_value
        sq_poptop(v);                           // registry, package, "_PATH", [path_value]
        sq_pushstring(v,SQ_PACKAGE_DEFAULT_PATH,-1); // registry, package, "_PATH", def_path_value
    }
//    sq_push(v,-2);                              // registry, package, "_PATH", path_value, "_PATH"
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
//    sq_push(v,-2);                              // registry, package, "_CPATH", cpath_value, "_CPATH"
    sq_pushstring(v,_SC("CPATH"),-1);           // registry, package, "_CPATH", path_value, "CPATH"
    sq_push(v,-2);                              // registry, package, "_CPATH", cpath_value, "CPATH", cpath_value
    // package._CPATH <- path_value
    sq_rawset(v,-5);                            // registry, package, "_CPATH", cpath_value, ["CPATH", def_path_value]
    // registry._CPATH <- path_value
    sq_rawset(v,-4);                            // registry, package, ["_CPATH", cpath_value]
    
    sq_pop(v,2);                                // [registry, package]
	sqstd_registerfunctions(v, g_package_funcs);
    return SQ_OK;
}
