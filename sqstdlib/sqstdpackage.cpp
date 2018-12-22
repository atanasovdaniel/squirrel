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

#ifdef _WIN32
#define DIR_SEP_CHAR        _SC('\\')
#else // _WIN32
#define DIR_SEP_CHAR        _SC('/')
#endif // _WIN32
#define LIST_SEP_CHAR          _SC(';')
#define REPLACE_CHAR      _SC('?')
#define DIR_SEP_REPLACE_CHAR   _SC('/')


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

static SQInteger pkg_getenv(HSQUIRRELVM v,const SQChar *name)
{
    sq_pushroottable(v);                            // root
    sq_pushstring(v,_SC("getenv"),-1);              // root, "getenv"
    if(SQ_SUCCEEDED(sq_get(v,-2))) {                // root, ["getenv"], getenv
        sq_remove(v,-2);                            // [root], getenv
        sq_pushroottable(v);                        // getenv, root
        sq_pushstring(v,name,-1);                   // getenv, root, name
        if(SQ_SUCCEEDED(sq_call(v,2,SQTrue,SQFalse))) { // getenv, [root, name], value
            sq_remove(v,-2);                        // [getenv], value
            if( sq_gettype(v,-1) == OT_STRING)      // value
                return 1;                           // value
        }
    }
    sq_reseterror(v);
    sq_poptop(v);                                   // [root] / [getenv] / [value]
    return 0;
}

/* ====================================
    package.nut
==================================== */

#define SQ_PACKAGE_NUT_PATH_ENV     _SC("SQPATH")
#define SQ_PACKAGE_NUT_SFX          _SC("?.nut;?/init.nut")

static char PACKAGE_NUT_ID;

static SQInteger _package_nut_SEARCH(HSQUIRRELVM v)
{
    // package, pkg_name, hint...
    if(sq_gettop(v) > 2) {
        sq_settop(v,2); // package, pkg_name, [hint...]
    }
    const SQChar *path;
    const SQChar *sfx;
    const SQChar *what;
    sq_pushstring(v,_SC("PATH"),-1);            // package, pkg_name, "PATH"
    sq_rawget(v,1);                             // package, pkg_name, ["PATH"], path
    sq_getstring(v,-1,&path);                   // package, pkg_name, path
    sq_poptop(v);                               // package, pkg_name, [path]
    sq_pushstring(v,_SC("SUFFIXES"),-1);        // package, pkg_name, "SUFFIXES"
    sq_rawget(v,1);                             // package, pkg_name, ["SUFFIXES"], sfx
    sq_getstring(v,-1,&sfx);                    // package, pkg_name, sfx
    sq_poptop(v);                               // package, pkg_name, [sfx]
    {
        const SQChar *pkg_name;
        SQInteger pkg_name_size;
        sq_getstringandsize(v,2,&pkg_name,&pkg_name_size);  // package, pkg_name
        SQChar *tmp = sq_getscratchpad(v,pkg_name_size);
        for(SQInteger i=0; i < pkg_name_size; i++)
            if( pkg_name[i] == _SC('.'))
                tmp[i] = DIR_SEP_CHAR;
            else
                tmp[i] = pkg_name[i];
        sq_poptop(v);                                       // package, [pkg_name]
        sq_pushstring(v,tmp,pkg_name_size);                 // package, what
        sq_getstring(v,-1,&what);
    }
    if( sqstd_package_searchfile(v,what,path,sfx) == 1) { // package, pkg_name, file_name
        const SQChar *filename;
        SQRESULT r;
        sq_getstring(v,-1,&filename);
        r = sqstd_loadfile(v,filename,SQTrue);
        if(SQ_FAILED(r)) {
            return sq_throwerror(v,_SC("failed to load package"));
        }
        return 1;
    }
    return 0;
}

static SQInteger register_nut_package( HSQUIRRELVM v)
{
    sq_newtable(v);                                 // package
    sq_pushstring(v,_SC("PATH"),-1);                // package, "PATH"
    if( pkg_getenv(v,SQ_PACKAGE_NUT_PATH_ENV) == 0) // package, "PATH", env
        sq_pushstring(v,_SC(""),-1);                // package, "PATH", ""
    sq_rawset(v,-3);                                // package, ["PATH", val]
    sq_pushstring(v,_SC("SUFFIXES"),-1);            // package, "SUFFIXES"
    sq_pushstring(v,SQ_PACKAGE_NUT_SFX,-1);         // package, "SUFFIXES", val
    sq_rawset(v,-3);                                // package, ["SUFFIXES", val]
    sq_pushstring(v,_SC("SEARCH"),-1);              // package, "SEARCH"
    sq_newclosure(v,_package_nut_SEARCH,0);         // package, "SEARCH", closure
    sq_setparamscheck(v,-2,_SC(".ss"));
    sq_push(v,-3);                                  // package, "SEARCH", closure, package
    sq_bindenv(v,-2);                               // package, "SEARCH", closure, [package], bclosure
    sq_remove(v,-2);                                // package, "SEARCH", [closure], bclosure
    // register searcher
    sqstd_package_getsearchers(v);                  // package, "SEARCH", bclosure, searchers
    sq_push(v,-2);                                  // package, "SEARCH", bclosure, searchers, bclosure
    sq_arrayinsert(v,-2,0);                         // package, "SEARCH", bclosure, searchers, [bclosure]
    sq_poptop(v);                                   // package, "SEARCH", bclosure, [searchers]
    sq_rawset(v,-3);                                // package, ["SEARCH", bclosure]
    sq_pushregistrytable(v);                        // package, registry
    sq_pushuserpointer(v,&PACKAGE_NUT_ID);          // package, registry, NUT_ID
    sq_push(v,-3);                                  // package, registry, NUT_ID, package
    sq_rawset(v,-3);                                // package, registry, [NUT_ID, package]
    sq_poptop(v);                                   // package, [registry]
    return 1;
}

SQRESULT sqstd_package_nutaddpath(HSQUIRRELVM v, const SQChar *path)
{
    const SQChar *cur_path;
    SQInteger cur_path_len;
    sq_pushregistrytable(v);                        // registry
    sq_pushuserpointer(v,&PACKAGE_NUT_ID);          // registry, NUT_ID
    if(SQ_FAILED(sq_rawget(v,-2))) {                // registry, [NUT_ID], package_nut
        sq_poptop(v);                               // [registry]
        return SQ_ERROR;
    }
    sq_remove(v,-2);                                // [registry], package_nut
    sq_pushstring(v,_SC("PATH"),-1);                // package_nut, "PATH"
    sq_push(v,-1);                                  // package_nut, "PATH", "PATH"
    if(SQ_FAILED(sq_rawget(v,-3))) {                // package_nut, "PATH", ["PATH"], cur_path
        sq_pop(v,2);                                // [package_nut, "PATH"]
        return SQ_ERROR;
    }
    if(SQ_FAILED(sq_getstringandsize(v,-1,&cur_path,&cur_path_len))) {
        cur_path = _SC("");
        cur_path_len = 0;
    }
    SQInteger path_len = scstrlen(path);
    SQChar *new_path = sq_getscratchpad(v,(path_len+1+cur_path_len)*sizeof(SQChar));
    memcpy(new_path,path,path_len*sizeof(SQChar));
    new_path[path_len] = LIST_SEP_CHAR;
    memcpy(new_path+path_len+1,cur_path,cur_path_len*sizeof(SQChar));
    sq_poptop(v);                                   // package_nut, "PATH", [cur_path]
    sq_pushstring(v,new_path,path_len+1+cur_path_len); // package_nut, "PATH", new_path
    sq_rawset(v,-3);                                // package_nut, ["PATH", new_path]
    sq_poptop(v);                                   // [package_nut]
    return SQ_OK;
}

/* ====================================
		package
==================================== */

static char PACKAGE_ID;
static char SEARCHERS_ARRAY_ID;
static char LOADED_TABLE_ID;

/* ----------------
    paths
---------------- */

typedef struct
{
    const SQChar *from;
    const SQChar *to;
} elems_t;

static int elems_next( elems_t *elems, SQChar delimer)
{
    while(1) {
        if( *elems->to == _SC('\0')) return 0;
        elems->from = elems->to;
        while( *elems->from == delimer) elems->from++;
        elems->to = elems->from;
        while( (*elems->to != _SC('\0')) && (*elems->to != delimer)) elems->to++;
        if( elems->from != elems->to) return 1;
    }
}

static SQInteger count_char( const elems_t *elems, SQChar c, SQInteger len)
{
    const SQChar *from = elems->from;
    const SQChar *to = elems->to;
    SQInteger cnt = 0;
    while( from != to) {
        if( *from == c)
            cnt+=len;
        else
            cnt++;
        from++;
    }
    return cnt;
}

SQInteger sqstd_package_searchcbk(HSQUIRRELVM v, const SQChar *what, const SQChar *path, const SQChar *suffixes, sqstd_package_search_fct fct, void *user)
{
    elems_t epath;
    SQInteger what_len = scstrlen(what);
    epath.to = epath.from = path;
    while( elems_next(&epath,LIST_SEP_CHAR)) {
        elems_t esuff;
        esuff.to = esuff.from = suffixes;
        while( elems_next(&esuff,LIST_SEP_CHAR)) {
            SQInteger r;
            SQChar *res = sq_getscratchpad(v, sizeof(SQChar)*((epath.to-epath.from)+1+count_char(&esuff,REPLACE_CHAR,what_len)));
            SQChar *p;
            memcpy(res,epath.from,epath.to-epath.from);
            p = res+(epath.to-epath.from);
            while( ((p-res)>0) && (
                    (p[-1] == DIR_SEP_CHAR) ||
                    ((DIR_SEP_CHAR != _SC('/')) && (p[-1] == _SC('/')))
                )) p--;
            *p++ = DIR_SEP_CHAR;
            while( esuff.from < esuff.to) {
                if( *esuff.from == REPLACE_CHAR) {
                    memcpy(p,what,what_len);
                    p+=what_len;
                }
                else if( (DIR_SEP_CHAR != _SC('/')) && (*esuff.from == _SC('/'))) {
                    *p++ = DIR_SEP_CHAR;
                }
                else
                    *p++ = *esuff.from;
                esuff.from++;
            }
            sq_pushstring(v,res,p-res);
#ifdef _DEBUG
            {
                const SQChar *name;
                sq_getstring(v,-1,&name);
                printf( "Try:%s\n", name);
            }
#endif // _DEBUG
            r = fct(v,user);
            if( r != 0)
                return r;
            sq_poptop(v);
        }
    }
    return 0;
}

static SQInteger search_file_fct(HSQUIRRELVM v, void *SQ_UNUSED_ARG(user))
{
    const SQChar *name;
    SQFILE fh;
    sq_getstring(v,-1,&name);
    fh = sqstd_fopen(name,_SC("rb"));
    if( fh) {
        sqstd_fclose(fh);
        return 1;
    }
    return 0;
}

SQInteger sqstd_package_searchfile(HSQUIRRELVM v, const SQChar *what, const SQChar *path, const SQChar *suffixes)
{
    return sqstd_package_searchcbk(v,what,path,suffixes,search_file_fct,0);
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
        sq_push(v,idx+1);                               // searchers_array, iterator, search_fct, root, package_name, hint
        if(SQ_FAILED(sq_call(v,3,SQTrue,SQFalse))) {    // searchers_array, iterator, search_fct, [root, package_name]
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
    sq_push(v,idx+1);                               // load_fct, root, package_name, hint
    if(SQ_SUCCEEDED(sq_call(v,3,SQTrue,SQFalse))) {  // load_fct, [root, package_name]
        sq_remove(v,-2);                            // [load_fct], package
        return SQ_OK;                               // package
    }
    else {                                          // load_fct
        sq_pop(v,1);                                // [load_fct]
        return SQ_ERROR;
    }
}

static const SQChar _recursive_require[] = _SC("recursive require of ");

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
        sq_pushuserpointer(v,(SQUserPointer)(SQHash)_recursive_require);       // loaded_table, name, val
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
            if( p == (SQUserPointer)(SQHash)_recursive_require) {
                // recursion detected, remove recursion mark
                sq_poptop(v);                           // loaded_table, [package]
                sq_push(v,idx);                         // loaded_table, pkg_name
                sq_rawdeleteslot(v,-2,SQFalse);         // loaded_table, [pkg_name]
                sq_poptop(v);                           // [loaded_table]
                {
                    const SQChar *pkg_name;
                    SQInteger pkg_name_size;
                    SQInteger err_size = scstrlen(_recursive_require);
                    sq_getstringandsize(v,idx,&pkg_name,&pkg_name_size);
                    SQChar *err = sq_getscratchpad(v,(err_size + pkg_name_size + 1)*sizeof(SQChar));
                    memcpy(err,_recursive_require,err_size*sizeof(SQChar));
                    memcpy(err + err_size,pkg_name, (pkg_name_size+1)*sizeof(SQChar));
                    return sq_throwerror(v,err);
                }
            }
        }
        sq_remove(v,-2);                                // [loaded_table], package
        return SQ_OK;                                   // package
    }
}

SQRESULT sqstd_package_require(HSQUIRRELVM v, const SQChar *package, const SQChar *hint)
{
    sq_pushstring(v,package,-1);        // pkg_name
    sq_pushstring(v,hint ? hint : _SC(""),-1);           // pkg_name, hint
    if(SQ_SUCCEEDED(require(v, -1))) {  // pkg_name, hint, package
        sq_remove(v,-2);                // pkg_name, [hint], package
        sq_remove(v,-2);                // [pkg_name], package
        return SQ_OK;                   // package
    }
    else {
        sq_pop(v,2);                    // [pkg_name, hint]
        return SQ_ERROR;
    }
}

SQRESULT sqstd_package_registerfct( HSQUIRRELVM v, const SQChar *package, SQFUNCTION loader_fct)
{
    if(SQ_FAILED(get_loaded_table(v))) return SQ_ERROR; // loaded_table
    sq_pushstring(v,package,-1);                        // loaded_table, package_name
    if(SQ_FAILED(sq_rawget(v,-2)))                      // loaded_table, package_name
    {
        sq_reseterror(v);
        sq_newclosure(v,loader_fct,0);                      // loaded_table, loader_fct
        sq_pushroottable(v);                                // loaded_table, loader_fct, root
        if(SQ_SUCCEEDED(sq_call(v,1,SQTrue,SQFalse))) {     // loaded_table, loader_fct, [root], package
            sq_remove(v,-2);                                // loaded_table, [loader_fct], package
            sq_pushstring(v,package,-1);                    // loaded_table, package, package_name
            sq_push(v,-2);                                  // loaded_table, package, package_name, package
            sq_rawset(v,-4);                                // loaded_table, package, [package_name, package]
            sq_remove(v,-2);                                // [loaded_table], package
            return SQ_OK;                                   // package
        }
        else {
            sq_pop(v,2);                                    // [loaded_table, fct]
            return SQ_ERROR;                                //
        }
    }
    else {
        sq_remove(v,-2);                                    // [loaded_table], package
        return SQ_OK;                                       // package
    }
}

/* ----------------
---------------- */

static SQInteger _g_package_require( HSQUIRRELVM v)
{
    if(sq_gettop(v) > 2) {
        sq_settop(v,3);
    }
    else {
        sq_pushstring(v,_SC(""),-1);
    }
    if(SQ_FAILED(require(v, 2)))
        return SQ_ERROR;
    return 1;
}

static SQInteger _package_searchcbk_fct(HSQUIRRELVM v, void *user)
{
    // name
    SQInteger top = (SQInteger)user;
    SQInteger r;
    sq_push(v,5);           // name, fct
    sq_pushroottable(v);    // name, fct, root
    sq_push(v,-3);          // name, fct, root, name
    for(SQInteger i=6; i <= top; i++)
        sq_push(v,i);       // name, fct, root, name, argX
    if(SQ_FAILED(sq_call(v,2+top-5,SQTrue,SQFalse))) { // name, fct, [root, name], res
        sq_poptop(v);       // name, [fct]
        return SQ_ERROR;
    }
    sq_remove(v,-2);        // name, [fct], res
    if(SQ_FAILED(sq_getinteger(v,-1,&r))) // name, res
        r = sq_throwerror(v,_SC("Bad return value type"));
    sq_poptop(v);           // name, [res]
    return r;
}

static SQInteger _package_searchcbk(HSQUIRRELVM v)
{
    const SQChar *what;
    const SQChar *path;
    const SQChar *sfx;
    SQInteger r;
    sq_getstring(v,2,&what);
    sq_getstring(v,3,&path);
    sq_getstring(v,4,&sfx);
    r = sqstd_package_searchcbk(v,what,path,sfx,_package_searchcbk_fct,(void*)sq_gettop(v));
    return r;
}

static SQInteger _package_searchfile(HSQUIRRELVM v)
{
    const SQChar *what;
    const SQChar *path;
    const SQChar *sfx;
    sq_getstring(v,2,&what);
    sq_getstring(v,3,&path);
    sq_getstring(v,4,&sfx);
    return sqstd_package_searchfile(v,what,path,sfx);
}

#define _DECL_GLOBALPACKAGE_FUNC(name,nparams,typecheck) {_SC(#name),_g_package_##name,nparams,typecheck}
static const SQRegFunction g_package_funcs[]={
    _DECL_GLOBALPACKAGE_FUNC(require,-2,_SC(".ss")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

#define _DECL_PACKAGE_FUNC(name,nparams,typecheck) {_SC(#name),_package_##name,nparams,typecheck}
static const SQRegFunction package_funcs[]={
    _DECL_PACKAGE_FUNC(searchcbk,-5,_SC(".sssc")),
    _DECL_PACKAGE_FUNC(searchfile,4,_SC(".sss")),
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
