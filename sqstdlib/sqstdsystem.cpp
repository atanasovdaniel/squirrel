/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sqstdsystem.h>
#include <sqcvoschar.h>

#ifdef _WIN32
#include <wchar.h>
#define scgetenv _wgetenv
#define scsystem _wsystem
#define scasctime _wasctime
#define scremove _wremove
#define screname _wrename
#else
#define scgetenv getenv
#define scsystem system
#define scasctime asctime
#define scremove remove
#define screname rename
#endif

static SQInteger _system_getenv(HSQUIRRELVM v)
{
    const SQChar *name_sc[1];
    sq_getstring(v,2,&name_sc[0]);
    SQUnsignedInteger allocated;
    const scoschar_t **name_oc = sccvtoos(name_sc,1,&allocated);
    if( name_oc) {
        const scoschar_t *val_oc[1];
        val_oc[0] = scgetenv(name_oc[0]);
        if( allocated) {
            sq_free( name_oc, allocated);
        }
        if( val_oc[0]) {
            const SQChar **val_sc = sccvfromos( val_oc, 1, &allocated);
            if( val_sc) {
                sq_pushstring(v,val_sc[0],-1);
                if( allocated) {
                    sq_free( val_sc, allocated);
                }
                return 1;
            }
        }
    }
    sq_pushnull(v);
    return 1;
}


static SQInteger _system_system(HSQUIRRELVM v)
{
    const SQChar *cmd_sc[1];
    sq_getstring(v,2,&cmd_sc[0]);
    SQUnsignedInteger allocated;
    const scoschar_t **cmd_oc = sccvtoos(cmd_sc,1,&allocated);
    if( cmd_oc) {
        sq_pushinteger(v,scsystem(cmd_oc[0]));
        if( allocated) {
            sq_free( cmd_oc, allocated);
        }
    }
    else {
        sq_pushinteger(v,-1);
    }
    return 1;
}


static SQInteger _system_clock(HSQUIRRELVM v)
{
    sq_pushfloat(v,((SQFloat)clock())/(SQFloat)CLOCKS_PER_SEC);
    return 1;
}

static SQInteger _system_time(HSQUIRRELVM v)
{
    SQInteger t = (SQInteger)time(NULL);
    sq_pushinteger(v,t);
    return 1;
}

static SQInteger _system_remove(HSQUIRRELVM v)
{
    const SQChar *name_sc[1];
    sq_getstring(v,2,&name_sc[0]);
    SQUnsignedInteger allocated;
    const scoschar_t **name_oc = sccvtoos(name_sc,1,&allocated);
    if( name_oc) {
        int r = scremove(name_oc[0]);
        if( allocated) {
            sq_free( name_oc, allocated);
        }
        if(r==-1)
            return sq_throwerror(v,_SC("remove() failed"));
    }
    else {
        return sq_throwerror(v,_SC("argument conversion failed"));
    }
    return 0;
}

static SQInteger _system_rename(HSQUIRRELVM v)
{
    const SQChar *names_sc[2];
    sq_getstring(v,2,&names_sc[0]); // old
    sq_getstring(v,3,&names_sc[1]); // new
    SQUnsignedInteger allocated;
    const scoschar_t **names_oc = sccvtoos(names_sc,2,&allocated);
    if( names_oc) {
        int r = screname(names_oc[0],names_oc[1]);
        if( allocated) {
            sq_free( names_oc, allocated);
        }
        if(r==-1)
            return sq_throwerror(v,_SC("rename() failed"));
    }
    else {
        return sq_throwerror(v,_SC("arguments conversion failed"));
    }
    return 0;
}

static void _set_integer_slot(HSQUIRRELVM v,const SQChar *name,SQInteger val)
{
    sq_pushstring(v,name,-1);
    sq_pushinteger(v,val);
    sq_rawset(v,-3);
}

static SQInteger _system_date(HSQUIRRELVM v)
{
    time_t t;
    SQInteger it;
    SQInteger format = 'l';
    if(sq_gettop(v) > 1) {
        sq_getinteger(v,2,&it);
        t = it;
        if(sq_gettop(v) > 2) {
            sq_getinteger(v,3,(SQInteger*)&format);
        }
    }
    else {
        time(&t);
    }
    tm *date;
    if(format == 'u')
        date = gmtime(&t);
    else
        date = localtime(&t);
    if(!date)
        return sq_throwerror(v,_SC("crt api failure"));
    sq_newtable(v);
    _set_integer_slot(v, _SC("sec"), date->tm_sec);
    _set_integer_slot(v, _SC("min"), date->tm_min);
    _set_integer_slot(v, _SC("hour"), date->tm_hour);
    _set_integer_slot(v, _SC("day"), date->tm_mday);
    _set_integer_slot(v, _SC("month"), date->tm_mon);
    _set_integer_slot(v, _SC("year"), date->tm_year+1900);
    _set_integer_slot(v, _SC("wday"), date->tm_wday);
    _set_integer_slot(v, _SC("yday"), date->tm_yday);
    return 1;
}



#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_system_##name,nparams,pmask}
static const SQRegFunction systemlib_funcs[]={
    _DECL_FUNC(getenv,2,_SC(".s")),
    _DECL_FUNC(system,2,_SC(".s")),
    _DECL_FUNC(clock,0,NULL),
    _DECL_FUNC(time,1,NULL),
    _DECL_FUNC(date,-1,_SC(".nn")),
    _DECL_FUNC(remove,2,_SC(".s")),
    _DECL_FUNC(rename,3,_SC(".ss")),
    {NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC

SQInteger sqstd_register_systemlib(HSQUIRRELVM v)
{
    SQInteger i=0;
    while(systemlib_funcs[i].name!=0)
    {
        sq_pushstring(v,systemlib_funcs[i].name,-1);
        sq_newclosure(v,systemlib_funcs[i].f,0);
        sq_setparamscheck(v,systemlib_funcs[i].nparamscheck,systemlib_funcs[i].typemask);
        sq_setnativeclosurename(v,-1,systemlib_funcs[i].name);
        sq_newslot(v,-3,SQFalse);
        i++;
    }
    return 1;
}
