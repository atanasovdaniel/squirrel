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
#include <sqstdio.h>

#include <sqstdpackage.h>

#define _DEBUG

static SQInteger sqstd_registerfunctions(HSQUIRRELVM v,const SQRegFunction *fcts)
{
														// table
	if( fcts != 0) {
		const SQRegFunction	*f = fcts;
		while( f->name != 0) {
			SQBool bstatic = SQFalse;
			const SQChar *typemask = f->typemask;
			if( typemask && (typemask[0] == _SC('-'))) {	// if first char of typemask is '-', make static function
				typemask++;
				bstatic = SQTrue;
			}
			sq_pushstring(v,f->name,-1);				// table, method_name
			sq_newclosure(v,f->f,0);					// table, method_name, method_fct
			sq_setparamscheck(v,f->nparamscheck,typemask);
            sq_setnativeclosurename(v,-1,f->name);
			sq_newslot(v,-3,bstatic);					// table
			f++;
		}
	}
    return SQ_OK;
}

/* ====================================
    package.nut
==================================== */

#define SQ_PACKAGE_PATH_ENV     _SC("SQPATH")

static const SQChar *nut_suffixes[] = {
    _SC("?.nut"),
    _SC("?/init.nut"),
    0
};

static char NUT_PATH_ARRAY_ID;

static SQRESULT get_nut_path( HSQUIRRELVM v)
{
    sq_pushregistrytable(v);                            // registry
    sq_pushuserpointer(v,&NUT_PATH_ARRAY_ID);               // registry, NUT_PATH_ARRAY_ID
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // registry, [NUT_PATH_ARRAY_ID], path_array
        sq_poptop(v);                                   // [registry]
        return SQ_ERROR;                                //
    }
    sq_remove(v,-2);                                    // [registry], path_array
    return SQ_OK;
}

static SQInteger _package_nut_SEARCH(HSQUIRRELVM v)
{
    // root, pkg_name
    SQInteger r;
    if( SQ_FAILED(get_nut_path(v))) return SQ_ERROR;
    sq_push(v,2);                                       // path, pkg_name
    r = sqstd_package_pathsearch(v,-2);
    if(SQ_FAILED(r)) return SQ_ERROR;
    if( r) {                                            // path, filename
        const SQChar *filename;
        SQRESULT r;
        sq_getstring(v,-1,&filename);
        r = sqstd_loadfile(v,filename,SQTrue);
        if(SQ_FAILED(r)) {
            sq_pop(v,2);                                // [path, filename]
            return sq_throwerror(v,_SC("failed to load package"));  //
        }
        sq_remove(v,-2);                                // path, [filename], closure
        sq_remove(v,-2);                                // [path], closure
        return 1;
    }
    sq_poptop(v);                                       // [path]
    return 0;
}

SQRESULT sqstd_package_nutaddpath( HSQUIRRELVM v, const SQChar *path)
{
    if(SQ_FAILED(get_nut_path(v))) return SQ_ERROR; // PATH
    sq_pushstring(v,path,-1);                       // PATH, path
    SQRESULT r = sqstd_package_pathadd(v,-2,nut_suffixes); // PATH, [path]
    sq_poptop(v);                                   // [PATH]
    return r;
}

static SQInteger _package_nut_addpath( HSQUIRRELVM v)
{
    // x, arg
    if(SQ_FAILED(get_nut_path(v))) return SQ_ERROR;     // x, arg, PATH
    sq_push(v,-2);                                      // x, arg, PATH, arg
    sq_remove(v,-3);                                    // x, [arg], PATH, arg
    return sqstd_package_pathadd(v,-2,nut_suffixes);     // x, PATH, [arg]
}

#define _DECL_PACKAGE_NUT_FUNC(name,nparams,typecheck) {_SC(#name),_package_nut_##name,nparams,typecheck}
static const SQRegFunction package_nut_funcs[]={
    _DECL_PACKAGE_NUT_FUNC(addpath,2,_SC(".s")),
    _DECL_PACKAGE_NUT_FUNC(SEARCH,2,_SC(".s")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

static SQInteger register_nut_package( HSQUIRRELVM v)
{
    sq_newtable(v);                                 // package
    sqstd_registerfunctions(v, package_nut_funcs);  // package
    // .PATH
    sq_newarray(v,0);                               // package, array
    sq_pushregistrytable(v);                        // package, array, registry
    sq_pushuserpointer(v,&NUT_PATH_ARRAY_ID);       // package, array, registry, PATH_ID
    sq_push(v,-3);                                  // package, array, registry, PATH_ID, array
    sq_pushstring(v,_SC("PATH"),-1);                // package, array, registry, PATH_ID, array, "PATH"
    sq_push(v,-2);                                  // package, array, registry, PATH_ID, array, "PATH", array
    sq_rawset(v,-7);                                // package, array, registry, PATH_ID, array, ["PATH", array]
    sq_rawset(v,-3);                                // package, array, registry, [PATH_ID, array]
    sq_poptop(v);                                   // package, array, [registry]
    // fill path from environment SQPATH
    sq_pushroottable(v);                            // package, array, root
    sq_pushstring(v,_SC("getenv"),-1);              // package, array, root, "getenv"
    if(SQ_SUCCEEDED(sq_get(v,-2))) {                // package, array, root, ["getenv"], getenv
        sq_remove(v,-2);                            // package, array, [root], getenv
        sq_pushroottable(v);                        // package, array, getenv, root
        sq_pushstring(v,SQ_PACKAGE_PATH_ENV,-1);    // package, array, getenv, root, "SQPATH"
        if(SQ_SUCCEEDED(sq_call(v,2,SQTrue,SQFalse))) { // package, array, getenv, [root, "SQPATH"], sqpath
            sq_remove(v,-2);                        // package, array, [getenv], sqpath
            if( sq_gettype(v,-1) == OT_STRING) {    // package, array, sqpath
                sqstd_package_pathaddlist(v,-2,nut_suffixes); // package, array, [sqpath]
            }
            else
                sq_poptop(v);                       // package, array, [sqpath]
        }
        else {
            sq_poptop(v);                           // package, array, [getenv]
        }
    }
    sq_poptop(v);                                   // package, [array]
    // register searcher
    sqstd_package_getsearchers(v);                  // package, searchers
    sq_pushstring(v,_SC("SEARCH"),-1);              // package, searchers, "SEARCH"
    sq_rawget(v,-3);                                // package, searchers, ["SEARCH"], search_fct
    sq_arrayinsert(v,-2,0);                         // package, searchers, [search_fct]
    sq_poptop(v);                                   // package, [searchers]
    return 1;
}

/* ====================================
		package
==================================== */

#ifdef _WIN32
#define DIR_SEP_CHAR        _SC('\\')
#define DIR_SEP_STRING      _SC("\\")
#define PATH_LIST_SEP_STRING  _SC(";")
#else // _WIN32
#define DIR_SEP_CHAR        _SC('/')
#define DIR_SEP_STRING      _SC("/")
#define PATH_LIST_SEP_STRING  _SC(":")
#endif // _WIN32

#define PATH_REPLACE_CHAR      _SC('?')
#define DIR_SEP_REPLACE_CHAR   _SC('/')

static char PACKAGE_ID;
static char SEARCHERS_ARRAY_ID;
static char LOADED_TABLE_ID;

/* ----------------
    paths
---------------- */

SQRESULT sqstd_package_replacechar(HSQUIRRELVM v, SQChar the_char)
{
    // original, repl
    const SQChar *src, *repl, *p;
    SQChar *res, *r;
    SQInteger repl_len;
    SQInteger res_len;

    if(SQ_FAILED(sq_getstringandsize(v,-1,&repl,&repl_len))) return SQ_ERROR;
    if(SQ_FAILED(sq_getstring(v,-2,&src))) return SQ_ERROR;

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
    return SQ_OK;
}

static void sufix_cpy( SQChar *dst, const SQChar *sfx, SQInteger sfx_len)
{
    *dst++ = DIR_SEP_CHAR;
    while( sfx_len) {
        if( *sfx != DIR_SEP_REPLACE_CHAR)
            *dst++ = *sfx;
        else
            *dst++ = DIR_SEP_CHAR;
        sfx++;
        sfx_len--;
    }
}

SQRESULT sqstd_package_pathadd( HSQUIRRELVM v, SQInteger path_idx, const SQChar **suffixes)
{
    // element
    if( path_idx < 0) path_idx = sq_gettop(v) + path_idx + 1;
    const SQChar *elem;
    SQInteger elem_len;
    SQInteger ins_idx = 0;
    if( SQ_FAILED(sq_getstringandsize(v,-1,&elem,&elem_len)))
    {
        sq_poptop(v);           // [element]
        return SQ_ERROR;
    }
    while( (elem_len > 0) &&
           (elem[elem_len-1] == DIR_SEP_CHAR)
    ) elem_len--;
    while( *suffixes) {
        const SQChar *sfx = *suffixes;
        SQInteger sfx_len = scstrlen(sfx);
        SQChar *cat = sq_getscratchpad(v,sizeof(SQChar)*(elem_len+1+sfx_len+1));
        memcpy(cat,elem,sizeof(SQChar)*elem_len);
//        memcpy(cat+elem_len,sfx,sizeof(SQChar)*(sfx_len+1));
        sufix_cpy(cat+elem_len,sfx,sizeof(SQChar)*(sfx_len+1));
        sq_pushstring(v,cat,elem_len+1+sfx_len);                  // element, cat
#ifdef _DEBUG
        printf( "Add:%s\n", cat);
#endif // _DEBUG
        if(SQ_FAILED(sq_arrayinsert(v,path_idx,ins_idx))) {           // element, [cat]
            sq_poptop(v);   // [element]
            return SQ_ERROR;
        }
        ins_idx++;
        suffixes++;
    }
    sq_poptop(v);           // [element]
    return SQ_OK;
}

static SQRESULT path_add_array( HSQUIRRELVM v, SQInteger path_idx, const SQChar **suffixes)
{
    // array
    if( path_idx < 0) path_idx = sq_gettop(v) + path_idx + 1;
    SQInteger ar_len = sq_getsize(v,-1);
    if( ar_len == -1) {
        sq_poptop(v);                                   // [array]
        return SQ_ERROR;
    }
    while(ar_len > 0) {
        ar_len--;
        sq_pushinteger(v,ar_len);                       // array, index
        sq_rawget(v,-2);                                // array, [index], elem
        if(SQ_FAILED(sqstd_package_pathadd(v,path_idx,suffixes))) {  // array, [elem]
            sq_poptop(v);                               // [array]
            return SQ_ERROR;
        }
    }
    sq_poptop(v);                                       // [array]
    return SQ_OK;
}

SQRESULT sqstd_package_pathaddlist( HSQUIRRELVM v, SQInteger path_idx, const SQChar **suffixes)
{
    // path_elements
    if( path_idx < 0) path_idx = sq_gettop(v) + path_idx + 1;
    sq_pushroottable(v);                                // path_elements, root
    sq_pushstring(v,_SC("split"),-1);                   // path_elements, root, "split"
    if(SQ_FAILED(sq_get(v,-2))) {                       // path_elements, root, ["split"], split
        sq_pop(v,2);
        return SQ_ERROR;
    }
    sq_remove(v,-2);                                    // path_elements, [root], split
    sq_pushroottable(v);                                // path_elements, split, root
    sq_push(v,-3);                                      // path_elements, split, root, path_elements
    sq_remove(v,-4);                                    // [path_elements], split, root, path_elements
    sq_pushstring(v,PATH_LIST_SEP_STRING,-1);           // split, root, path_elements, separator
    if(SQ_FAILED(sq_call(v,3,SQTrue,SQFalse))) {        // split, array
        sq_poptop(v);                                   // [split]
        return SQ_ERROR;
    }
    sq_remove(v,-2);                                    // [split], array
    return path_add_array(v,path_idx,suffixes);         // [array]
}

SQInteger sqstd_package_pathsearch(HSQUIRRELVM v, SQInteger path_idx)
{
    // pkg_name
    if( path_idx < 0) path_idx = sq_gettop(v) + path_idx + 1;
    sq_pushstring(v,DIR_SEP_STRING,-1);         // pkg_name, dots_repl
    if(SQ_FAILED(sqstd_package_replacechar(v, _SC('.')))) { // name
        return SQ_ERROR;
    }
    sq_pushnull(v);                             // name, iter
    while(SQ_SUCCEEDED(sq_next(v,path_idx))) {  // name, iter, p_idx, p_value
        const SQChar *filename;
        sq_remove(v,-2);                        // name, iter, [p_idx], p_value
        sq_push(v,-3);                          // name, iter, p_value, name
        if(SQ_FAILED(sqstd_package_replacechar(v, PATH_REPLACE_CHAR))) {     // name, iter, [p_value, name], filename
            sq_pop(v,2);    // [name, iter]
            return SQ_ERROR;
        }
        sq_getstring(v,-1,&filename);
#ifdef _DEBUG
        scprintf("Try:%s\n",filename);
#endif // _DEBUG
        SQFILE file = sqstd_fopen(filename,_SC("rb"));
        if( file) {
            sq_remove(v,-2);                    // name, [iter], filename
            sq_remove(v,-2);                    // [name], filename
            sqstd_fclose(file);
            return 1;                           // filename
        }
        sq_poptop(v);                           // name, iter, [filename]
    }
    sq_pop(v,2);                                // [name, iter]
    return 0;
}

/* ----------------
    require
---------------- */

SQRESULT sqstd_package_getsearchers(HSQUIRRELVM v)
{
    sq_pushregistrytable(v);                            // registry
    sq_pushuserpointer(v,&SEARCHERS_ARRAY_ID);          // registry, SEARCHERS_ID
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // registry, searchers_array
        sq_poptop(v);                                   // [registry]
        return SQ_ERROR;
    }
    sq_remove(v,-2);                                    // [registry,] searchers_array
    return SQ_OK;
}

static SQRESULT run_searchers(HSQUIRRELVM v, SQInteger idx)
{
    if(SQ_FAILED(sqstd_package_getsearchers(v)))        // searchers_array
        return SQ_ERROR;
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
            return SQ_OK;
        }
        sq_pop(v,2);                                    // searchers_array, iterator, [search_fct, load_fct]
    }
    sq_pop(v,2);                                        // [searchers_array, iterator]
    return sq_throwerror(v,_SC("required package not found"));
}

static SQRESULT load_package(HSQUIRRELVM v, SQInteger idx)
{
    if(SQ_FAILED(run_searchers(v,idx))) return SQ_ERROR; // load_fct
    sq_pushroottable(v);                            // load_fct, root
    sq_push(v,idx);                                 // load_fct, root, package_name
    if(SQ_SUCCEEDED(sq_call(v,2,SQTrue,SQTrue))) {  // load_fct, [root, package_name]
        sq_remove(v,-2);                            // [load_fct], package
        return SQ_OK;                               // package
    }
    else {                                          // load_fct
        sq_pop(v,1);                                // [load_fct]
        return sq_throwerror(v,_SC("failed to load package")); //
    }
}

static SQChar _recursive_require[] = _SC("recursive require");

static SQRESULT get_loaded_table( HSQUIRRELVM v)
{
    sq_pushregistrytable(v);                            // registry
    sq_pushuserpointer(v,&LOADED_TABLE_ID);             // registry, LOADED_TABLE_ID
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // registry, [LOADED_TABLE_ID], loaded_table
        sq_poptop(v);                                   // [registry]
        return SQ_ERROR;                                //
    }
    sq_remove(v,-2);                                    // [registry], loaded_table
    return SQ_OK;
}

static SQInteger require( HSQUIRRELVM v, SQInteger idx)
{
    if( idx < 0) idx = sq_gettop(v) + idx + 1;
    if(SQ_FAILED(get_loaded_table(v))) return SQ_ERROR; // loaded_table
    sq_push(v,idx);                                     // loaded_table, pkg_name
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // loaded_table, [pkg_name]
        // mark package for recursion
        sq_reseterror(v);
        sq_push(v,idx);                                 // loaded_table, pkg_name
        sq_pushuserpointer(v,(SQUserPointer)_recursive_require);       // loaded_table, name, val
        sq_rawset(v,-3);                                // loaded_table
        sq_push(v,idx);                                 // loaded_table, pkg_name
        // then load it
        if(SQ_FAILED(load_package(v, idx))) {           // loaded_table, pkg_name, package
            // not found
            // remove recursion mark
            sq_rawdeleteslot(v,-2,SQFalse);             // loaded_table, [pkg_name]
            sq_poptop(v);                               // [loaded_table]
            return SQ_ERROR;
        }
        // package is found                             // loaded_table, pkg_name, package
        HSQOBJECT tmp;                                  // loaded_table, pkg_name, package
        sq_resetobject(&tmp);
        sq_getstackobj(v,-1,&tmp);
        sq_rawset(v,-3);                                // loaded_table, [pkg_name, package]
        sq_poptop(v);                                   // [loaded_table]
        sq_pushobject(v,tmp);                           // package
        return SQ_OK;
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
        return SQ_OK;                                       // package
    }
}

SQRESULT sqstd_package_require(HSQUIRRELVM v, const SQChar *package)
{
    sq_pushstring(v,package,-1);
    return require(v, -1);
}

SQRESULT sqstd_package_registerfct( HSQUIRRELVM v, const SQChar *package, SQFUNCTION fct)
{
    if(SQ_FAILED(get_loaded_table(v))) return SQ_ERROR; // loaded_table
    sq_pushstring(v,package,-1);                        // loaded_table, package_name
    if(SQ_FAILED(sq_rawget(v,-2)))                      // loaded_table, package_name
    {
        sq_reseterror(v);
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

/* ----------------
---------------- */

static SQInteger _g_package_require( HSQUIRRELVM v)
{
    if(SQ_SUCCEEDED(require(v, 2)))
        return 1;
    else
        return SQ_ERROR;
}

static SQRESULT get_suffixes(HSQUIRRELVM v, SQInteger idx, const SQChar ***psfx, SQUnsignedInteger *pallocated)
{
    SQInteger size = sq_getsize(v,idx);
    SQUnsignedInteger allocated = (size+1)*sizeof(SQChar*);
    const SQChar **sfx = (const SQChar **)sq_malloc(allocated);
    int i = 0;
    sq_pushnull(v);								        // iterator
    while(SQ_SUCCEEDED(sq_next(v,idx))) {   		    // iterator, key, value
        if(SQ_FAILED(sq_getstring(v,-1,sfx+i))) {
            sq_free( sfx, allocated);
            return SQ_ERROR;
        }
        i++;
        sq_pop(v,2);                                    // iterator, [key, value]
    }
    *psfx = sfx;
    *pallocated = allocated;
    return SQ_OK;
}

static SQInteger _package_pathadd( HSQUIRRELVM v)
{
    // self, patharray, arg, suffixes
    const SQChar **sfx;
    SQUnsignedInteger sfx_aloc;
    SQInteger res;
    if(SQ_FAILED(get_suffixes(v,4,&sfx,&sfx_aloc))) return SQ_ERROR;
    sq_push(v,3);                               // self, patharray, arg, suffixes, arg
    if(sq_gettype(v,-1) == OT_ARRAY)
        res = path_add_array(v,2,sfx);          // self, patharray, arg, suffixes, [arg]
    else
        res = sqstd_package_pathadd(v,2,sfx);   // self, patharray, arg, suffixes, [arg]
    sq_free(sfx,sfx_aloc);
    sq_settop(v,2);
    return SQ_FAILED(res) ? SQ_ERROR : 1;
}

static SQInteger _package_pathaddlist( HSQUIRRELVM v)
{
    // self, patharray, arg, suffixes
    const SQChar **sfx;
    SQUnsignedInteger sfx_aloc;
    SQInteger res;
    if(SQ_FAILED(get_suffixes(v,4,&sfx,&sfx_aloc))) return SQ_ERROR;
    sq_push(v,3);                               // self, patharray, arg, suffixes, arg
    res = sqstd_package_pathaddlist(v,2,sfx);   // self, patharray, arg, suffixes, [arg]
    sq_free(sfx,sfx_aloc);
    sq_settop(v,2);
    return SQ_FAILED(res) ? SQ_ERROR : 1;
}

static SQInteger _package_replacechar(HSQUIRRELVM v)
{
    SQInteger the_char;
    sq_getinteger(v,4,&the_char);
    sq_poptop(v);
    sqstd_package_replacechar(v,(SQChar)the_char);
    return 1;
}

static SQInteger _package_pathsearch(HSQUIRRELVM v)
{
    // self, path_array, name
    return sqstd_package_pathsearch(v,2);
}

#define _DECL_GLOBALPACKAGE_FUNC(name,nparams,typecheck) {_SC(#name),_g_package_##name,nparams,typecheck}
static const SQRegFunction g_package_funcs[]={
    _DECL_GLOBALPACKAGE_FUNC(require,2,_SC(".s")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

#define _DECL_PACKAGE_FUNC(name,nparams,typecheck) {_SC(#name),_package_##name,nparams,typecheck}
static const SQRegFunction package_funcs[]={
    _DECL_PACKAGE_FUNC(pathadd,4,_SC(".as|aa")),
    _DECL_PACKAGE_FUNC(pathaddlist,4,_SC(".asa")),
    _DECL_PACKAGE_FUNC(pathsearch,3,_SC(".as")),
    _DECL_PACKAGE_FUNC(replacechar,4,_SC(".ssi")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

SQRESULT sqstd_register_packagelib(HSQUIRRELVM v)
{
    sq_pushregistrytable(v);                    // registry

    // package = {...}
    sq_newtable(v);                             // registry, package
    sqstd_registerfunctions(v, package_funcs);  // registry, package

    // registry.PACKAGE_ID = package
    sq_pushuserpointer(v,&PACKAGE_ID);          // registry, package, PACKAGE_ID
    sq_push(v,-2);                              // registry, package, PACKAGE_ID, package
    sq_rawset(v,-4);                            // registry, package, [PACKAGE_ID, package]

    sq_pushuserpointer(v,&LOADED_TABLE_ID);     // registry, package, LOADED_ID
    // LOADED = {}
    sq_newtable(v);                             // registry, package, LOADED_ID, loaded_table
    // LOADED["package"] <- package.weakref()
    sq_pushstring(v,_SC("package"),-1);         // registry, package, LOADED_ID, loaded_table, "package"
    sq_weakref(v,-4);                           // registry, package, LOADED_ID, loaded_table, "package", package
    sq_rawset(v,-3);                            // registry, package, LOADED_ID, loaded_table, ["package", package]
    // package.LOADED <- LOADED
    sq_pushstring(v,_SC("LOADED"),-1);          // registry, package, LOADED_ID, loaded_table, "LOADED"
    sq_push(v,-2);                              // registry, package, LOADED_ID, loaded_table, "LOADED", loaded_table
    sq_rawset(v,-5);                            // registry, package, LOADED_ID, loaded_table, ["LOADED", loaded_table]
    // registry._LOADED_ID <- _LOADED
    sq_rawset(v,-4);                            // registry, package, [LOADED_ID, loaded_table]

    sq_pushuserpointer(v,&SEARCHERS_ARRAY_ID);  // registry, package, SEARCHERS_ID
    sq_newarray(v,0);                           // registry, package, SEARCHERS_ID, searchers_array
    sq_pushstring(v,_SC("SEARCHERS"),-1);       // registry, package, SEARCHERS_ID, searchers_array, "SEARCHERS"
    sq_push(v,-2);                              // registry, package, SEARCHERS_ID, searchers_array, "SEARCHERS", searchers_array
    // package.SEARCHERS = searchers_array
    sq_rawset(v,-5);                            // registry, package, SEARCHERS_ID, searchers_array, ["SEARCHERS", searchers_array]
    // registry.SEARCHERS_ID = searchers_array
    sq_rawset(v,-4);                            // registry, package, [SEARCHERS_ID, searchers_array]

    sq_pop(v,2);                                // [registry, package]

	sqstd_registerfunctions(v, g_package_funcs);

	sqstd_package_registerfct(v,_SC("package.nut"),register_nut_package);
	sq_poptop(v);

    return SQ_OK;
}
