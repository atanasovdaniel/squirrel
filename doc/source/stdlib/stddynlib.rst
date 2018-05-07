.. _stdlib_stddynlib:

=============================
The dynlib library
=============================

The dynlib library provides functionality to dynamicaly load libraryed (.so or .dll).

--------------
Squirrel API
--------------

++++++++++++++++
The dynlib class
++++++++++++++++

The dynlib class uses native API to load dynamic libraryes.
On WINDOWS platform it uses `LoadLibrary` to load Dynamic-link library (or .DLL).
On other platforms it uses `dlopen` from libdl to load shared object (or .so).

Loaded libraryes are stored in two places (if they are not loaded as private):

1.  In static member `dynlib.LOADEDLIBS` table where libraryes are stored by library file path name. So already loaded libraryes can be reused.

2.  In internal (registry table key `CCHAIN`) linked list. So first loaded library is unloaded last.

The libraryes loaded by `is_private` == true, are always loaded (by call to dlopen/LoadLibrary) and are not stored in any internal list.
So they are unloaded when `dynlib` instance is released. A call to `dynlib.register` explicitly adds private library to `CCHAIN` and `dynlib.LOADEDLIBS`.

.. js:class:: dynlib(path, [is_private])

    :param string path: Path to library.
    :param bool is_private: Optional. States if loaded library will be private to this instance. Default if false.

    Loads a dynamic library `path`.
    
    If `is_private` == false (default), `dynlib` first checks in `dynlib.LOADEDLIBS` for already loaded library.
    Then new library is loaded.
    
    If `is_private` == true, library is loaded always.

.. js:function:: dynlib.cfunction( sym_name, [nparams, type_check])

    :param string sym_name: Name of symbol in library.
    :param integer nparams: Optional. Number of parameters.
    :param integer type_check: Optional. Type checks for parameters.
    
    Create new squirrel function from symbol `sym_name` with optional `nparams` and `type_check` (see `sq_setparamscheck`).

.. js:function:: dynlib.symbol( sym_name)

    :param string sym_name: Name of symbol in library.

    Returns UserPointer to a `sym_name` symbol from library.

.. js:function:: dynlib.register()

    Adds private library to `dynlib.LOADEDLIBS`. This call will do the work only once and only for private library.

--------------
C API
--------------

+++++++++++++++++++
The dynlib library
+++++++++++++++++++

.. c:function:: SQRESULT sqstd_register_dynlib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    Initialize and register the dynlib library in the given VM.

.. c:function:: SQInteger sqstd_load_dynlib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT

    Dynlib loader function.

++++++++++++++++++++++++
The dynlib class
++++++++++++++++++++++++

The dynamic library is represented by opaque handle `SQDYNLIB`.

.. c:function:: SQDYNLIB sqstd_dynlib_rawload( const SQChar *path)

    :param SQChar* path: Library path name
    :returns: Library handle or NULL on error.
    
    Calls native function to open dynamic library.

.. c:function:: SQUserPointer sqstd_dynlib_rawsym( SQDYNLIB lib, const SQChar *name)

    :param SQDYNLIB lib: Library handle
    :param SQChar* name: Symbol name
    :returns: Address of symbol or NULL if symbol is not found.
    
    Calls native function to search for symbol with `name` in dynamic library `lib`.

.. c:function:: SQBool sqstd_dynlib_rawclose( SQDYNLIB lib)

    :param SQDYNLIB lib: Library handle
    :returns: false if OK, true on error.
    
    Calls native function to close dynamic library.


.. c:function:: SQRESULT sqstd_dynlib_error( HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT

    Pushes to stack string representing last error occured in calls to native dynamic library functions.


.. c:function:: SQRESULT sqstd_dynlib_load(HSQUIRRELVM v, const SQChar *path, SQBool is_private, SQDYNLIB *plib)

    :param HSQUIRRELVM v: the target VM
    :param SQChar* path: Library path name
    :param SQBool is_private: If library will be loaded private
    :param SQDYNLIB* plib: Output, handle to library
    :returns: an SQRESULT
    
    Loads dynamic library `path` using mechanisms of `dynlib.LOADEDLIBS` and `CCHAIN` to load and register library.

.. c:function:: SQRESULT sqstd_dynlib_sym(HSQUIRRELVM v,SQDYNLIB lib, const SQChar *sym_name, SQUserPointer *psym)

    :param HSQUIRRELVM v: the target VM
    :param SQDYNLIB lib: Library handle
    :param SQChar* name: Symbol name
    :param SQUserPointer* psym: Output, address of symbol
    :returns: an SQRESULT

    Search for symbol `name` in library `lib`.

.. c:function:: SQRESULT sqstd_dynlib_register(HSQUIRRELVM v, SQDYNLIB lib, const SQChar *path)

    :param HSQUIRRELVM v: the target VM
    :param SQDYNLIB lib: Library handle
    :param SQChar* path: Library path name
    :returns: an SQRESULT

    Register a private libraray `lib` with path name `path`.

