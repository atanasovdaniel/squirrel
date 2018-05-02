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
#include <stdlib.h>
#include <squirrel.h>
#include <sqstdaux.h>
#include <sqstdpackage.h>

#ifdef SQSTD_ENABLE_DYNLIB

#include <sqstddynlib.h>

static char LOADEDLIBS_TABLE_ID;
static char CCHAIN_ID;

static HSQMEMBERHANDLE dynlib__path_handle;
static HSQMEMBERHANDLE dynlib__LOADEDLIBS_handle;

/* ====================================
		dynamic library
==================================== */

#ifdef _WIN32
/* ----------------
    WIN32
---------------- */

#include <windows.h>

SQDYNLIB sqstd_dynlib_rawload( const SQChar *path)
{
    SQDYNLIB lib = (SQDYNLIB)LoadLibrary( path);
#ifdef _DEBUG
    scprintf("LoadLibrary(%s) %p\n", path, lib);
#endif // _DEBUG
    return lib;
}

SQUserPointer sqstd_dynlib_rawsym( SQDYNLIB lib, const SQChar *name)
{
	SQUserPointer prc = (SQUserPointer)GetProcAddress( (HMODULE)lib, name);
#ifdef _DEBUG
    scprintf("GetProcAddress(%s) %p\n", name, prc);
#endif // _DEBUG
    return prc;
}

SQBool sqstd_dynlib_rawclose( SQDYNLIB lib)
{
#ifdef _DEBUG
    scprintf("FreeLibrary(%p)\n", lib);
#endif // _DEBUG
    return FreeLibrary( (HMODULE)lib) ? SQTrue : SQFalse;
}

#define _LDLIB_ERR_LEN  256

SQRESULT sqstd_dynlib_error( HSQUIRRELVM v)
{
    DWORD err = GetLastError();
    DWORD r;
    SQChar *buf;
    buf = sq_getscratchpad(v, _LDLIB_ERR_LEN * sizeof(SQChar));
    r = FormatMessage(
            FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, err, 0,
            buf, _LDLIB_ERR_LEN,
            NULL);
    if( r) {
        sq_pushstring(v,buf,r);
    }
    else {
        scsprintf(buf,_LDLIB_ERR_LEN, "system error: " _PRINT_INT_FMT, (SQInteger)err);
        sq_pushstring(v,buf,-1);
    }
    return SQ_OK;
}

#else // _WIN32

/* ----------------
    UNIX / POSIX
---------------- */

#include <dlfcn.h>

SQDYNLIB sqstd_dynlib_rawload( const SQChar *path)
{
    SQDYNLIB lib = dlopen( path, RTLD_NOW | RTLD_GLOBAL);
#ifdef _DEBUG
    scprintf("dlopen(%s) %p\n", path, lib);
#endif // _DEBUG
    return lib;
}

SQUserPointer sqstd_dynlib_rawsym( SQDYNLIB lib, const SQChar *name)
{
    return (SQUserPointer)dlsym( lib, name);
}

SQBool sqstd_dynlib_rawclose( SQDYNLIB lib)
{
#ifdef _DEBUG
    scprintf("dlclose(%p)\n", lib);
#endif // _DEBUG
    return (SQBool)dlclose( lib);
}

SQRESULT sqstd_dynlib_error( HSQUIRRELVM v)
{
    sq_pushstring(v, dlerror(), -1);
    return SQ_OK;
}

#endif // _WIN32

/* ----------------
---------------- */

struct dllib_chain_t {
    struct dllib_chain_t *prev;
    SQDYNLIB lib;
};

static SQInteger _dllib_chain_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    dllib_chain_t *chain = (dllib_chain_t*)*(SQUserPointer*)p;
    while( chain) {
        dllib_chain_t *prev = chain->prev;
        sqstd_dynlib_rawclose( chain->lib);
        sq_free( chain, sizeof(dllib_chain_t));
        chain = prev;
    }
    return 0;
}

SQRESULT sqstd_dynlib_register(HSQUIRRELVM v, SQDYNLIB lib, const SQChar *path)
{
    SQUserPointer udata;
    SQInteger r;
    sq_pushregistrytable(v);                            // registry
    sq_pushuserpointer(v,&CCHAIN_ID);                   // registry, CCHAIN_ID
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // registry, [CCHAIN_ID]
        sq_poptop(v);                                   // [registry]
        return SQ_ERROR;
    }
    sq_getuserdata(v,-1,&udata,0);                      // registry, udata
    sq_poptop(v);                                       // registry, [udata]
    sq_pushuserpointer(v,&LOADEDLIBS_TABLE_ID);         // registry, LOADEDLIBS_ID
    if(SQ_FAILED(sq_rawget(v,-2))) {                    // registry, [LOADEDLIBS_ID]
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

SQRESULT sqstd_dynlib_load(HSQUIRRELVM v, const SQChar *path, SQBool is_private, SQDYNLIB *plib)
{
    SQDYNLIB lib = 0;
    if(!is_private) {
        sq_pushregistrytable(v);                        // registry
        sq_pushuserpointer(v,&LOADEDLIBS_TABLE_ID);     // registry, LOADEDLIBS_ID
        if(SQ_FAILED(sq_rawget(v,-2))) {                // registry, [LOADEDLIBS_ID]
            sq_poptop(v);                               // [registry]
            return SQ_ERROR;
        }
        sq_remove(v,-2);                                // [registry], cloaded_table
        sq_pushstring(v,path,-1);                       // cloaded_table, path
        if(SQ_SUCCEEDED(sq_rawget(v,-2))) {             // cloaded_table, [path]
            sq_getuserpointer(v,-1,(SQUserPointer*)plib);
            sq_pop(v,2);                                // [cloaded_table, up_library]
            return SQ_OK;
        }
        else {
            sq_poptop(v);                               // [cloaded_table]
        }
    }
    lib = sqstd_dynlib_rawload( path);
    if( lib == 0) {
        sqstd_dynlib_error(v);                          // error
        return sq_throwobject(v);
    }
    if(!is_private) {
        if(SQ_FAILED(sqstd_dynlib_register(v,lib,path))) return SQ_ERROR;
    }
    *plib = lib;
    return SQ_OK;
}

SQRESULT sqstd_dynlib_sym(HSQUIRRELVM v,SQDYNLIB lib, const SQChar *sym_name, SQUserPointer *psym)
{
    SQUserPointer sym;
    sym = sqstd_dynlib_rawsym( lib, sym_name);
    if( sym) {
        *psym = sym;
        return SQ_OK;
    }
    else {
        sqstd_dynlib_error(v);          // error
        return sq_throwobject(v);       //
    }
}

/* ----------------
---------------- */

#define SETUP_DYNLIB(v) \
    SQUserPointer self = NULL; \
    if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,SQSTD_DYNLIB_TYPE_TAG))) \
        return sq_throwerror(v,_SC("invalid type tag"));

static SQInteger __dynlib_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    sqstd_dynlib_rawclose( (SQDYNLIB)p);
    return 1;
}

static SQInteger _dynlib_constructor(HSQUIRRELVM v)
{
    SQDYNLIB lib;
    const SQChar *path;
    SQBool is_private = SQFalse;
    if( sq_gettop(v) > 2) {
        sq_getbool(v,3,&is_private);
        sq_settop(v,2);
    }
    sq_getstring(v,2,&path);
    if(SQ_FAILED(sqstd_dynlib_load(v,path,is_private,&lib))) return SQ_ERROR;
    if(SQ_FAILED(sq_setinstanceup(v,1,(SQUserPointer)lib))) {
        return sq_throwerror(v, _SC("cannot create dynlib instance"));
    }
    sq_pushstring(v,path,-1);
	sq_setbyhandle(v,1,&dynlib__path_handle);

    if( is_private) {
        sq_setreleasehook(v,1,__dynlib_releasehook);
    }

    return 0;
}

static SQInteger _dynlib_register(HSQUIRRELVM v)
{
    SETUP_DYNLIB(v);
    if(sq_getreleasehook(v,1) != NULL) {
        const SQChar *path;
        sq_getbyhandle(v,1,&dynlib__path_handle);
        sq_getstring(v,-1,&path);
        if(SQ_FAILED(sqstd_dynlib_register(v,self,path))) return SQ_ERROR;
        sq_setreleasehook(v,1,NULL);
    }
    return 0;
}

static SQInteger _dynlib_symbol(HSQUIRRELVM v)
{
    SETUP_DYNLIB(v);
    const SQChar *sym_name;
    SQUserPointer sym = 0;
    sq_getstring(v,2,&sym_name);
    if(SQ_FAILED(sqstd_dynlib_sym(v,self,sym_name,&sym))) return SQ_ERROR;
    sq_pushuserpointer(v,sym);
    return 1;
}

static SQInteger _dynlib_cfunction(HSQUIRRELVM v)
{
    SQInteger top = sq_gettop(v);
    const SQChar *sym_name;
    SQUserPointer fct = 0;
    SETUP_DYNLIB(v);
    if( top > 3) {
        sq_settop(v,4);         // lib, name, nparams, typecheck
        top = 4;
    }
    sq_getstring(v,2,&sym_name);
    if(SQ_FAILED(sqstd_dynlib_sym(v,self,sym_name,&fct))) return SQ_ERROR;
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
    sq_setnativeclosurename(v,-1,sym_name);
    return 1;
}

static SQInteger _dynlib__typeof(HSQUIRRELVM v)
{
    sq_pushstring(v,_SC("dynlib"),-1);
    return 1;
}


//bindings
#define _DECL_DYNLIB_FUNC(name,nparams,typecheck) {_SC(#name),_dynlib_##name,nparams,typecheck}
static const SQRegFunction _dynlib_methods[] = {
    _DECL_DYNLIB_FUNC(constructor,-2,_SC("xsb")),
    _DECL_DYNLIB_FUNC(register,1,_SC("x")),
    _DECL_DYNLIB_FUNC(symbol,2,_SC("xs")),
    _DECL_DYNLIB_FUNC(cfunction,-2,_SC("xsis")),
    _DECL_DYNLIB_FUNC(_typeof,1,_SC("x")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

static const SQRegMember _dynlib_members[] = {
	{_SC("path"), &dynlib__path_handle },
    {_SC("-LOADEDLIBS"), &dynlib__LOADEDLIBS_handle },
	{NULL,NULL}
};

SQUserPointer _sqstd_dynlib_type_tag(void)
{
    return (SQUserPointer)_sqstd_dynlib_type_tag;
}

static const SQRegClass _sqstd_dynlib_decl = {
    0,                      // name
	_dynlib_members,        // members
	_dynlib_methods,        // methods
};

SQInteger sqstd_load_dynlib(HSQUIRRELVM v)
{
	if(SQ_FAILED(sqstd_registerclass(v,SQSTD_DYNLIB_TYPE_TAG,&_sqstd_dynlib_decl,0)))
	{
		return SQ_ERROR;
	}
    
    sq_pushregistrytable(v);                    // dynlib, registry

    // registry.CCHAIN_ID = udata
    sq_pushuserpointer(v,&CCHAIN_ID);           // dynlib, CCHAIN_ID
    {
        SQUserPointer *chain = (SQUserPointer*)sq_newuserdata(v, sizeof(SQUserPointer));
        sq_setreleasehook(v,-1,_dllib_chain_releasehook);
        *chain = 0;
    }
    sq_rawset(v,-3);                            // dynlib, registry, [CCHAIN_ID, udata]
    
    sq_pushuserpointer(v,&LOADEDLIBS_TABLE_ID); // dynlib, registry, LOADEDLIBS_ID
    // LOADEDLIBS = {}
    sq_newtable(v);                             // dynlib, registry, LOADEDLIBS_ID, cloaded_table
    sq_push(v,-1);                              // dynlib, registry, LOADEDLIBS_ID, cloaded_table, cloaded_table
    // dynlib.LOADEDLIBS = LOADEDLIBS
	sq_setbyhandle(v,-5,&dynlib__LOADEDLIBS_handle); // dynlib, registry, LOADEDLIBS_ID, cloaded_table, [cloaded_table]
    // registry.LOADEDLIBS_TABLE_ID = CLOADED
    sq_rawset(v,-3);                            // dynlib, registry, [CLOADED_ID, cloaded_table]
    
    sq_poptop(v);                               // dynlib, [registry]
    
    return SQ_OK;
}

SQRESULT sqstd_register_dynlib(HSQUIRRELVM v)
{
    if(SQ_SUCCEEDED(sqstd_package_registerfct(v,_SC("dynlib"),sqstd_load_dynlib))) {
        sq_poptop(v);
        return SQ_OK;
    }
    return SQ_ERROR;
}

#endif // SQSTD_ENABLE_DYNLIB
