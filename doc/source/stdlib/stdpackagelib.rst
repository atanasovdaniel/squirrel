.. _stdlib_stdpackagelib:

=============================
The Package library
=============================

The package library provides functionality to search and load source packages/modules/libraryes.

--------------
Squirrel API
--------------

++++++++++++++
Global Symbols
++++++++++++++

.. js:function:: require(package)

    :param string package: virtual name of package.

    Search for 'package', loads it and returns it's content.
    
    First `require` looks in `package.LOADED` if the `package` is already loaded. If it is, value stored in `package.LOADED[package]` is returned.
    
    Second `require` calls sequentially package searchers from `package.SEARCHERS` to find a `package`.

++++++++++++
Package data
++++++++++++

.. js:data:: package.LOADED

    Table, containing loaded packages, keys are package names, values are package content. `require` first searches in this table for already loaded packages.
    
.. js:data:: package.SEARCHERS

    Array, containing searcher functions. Functions in this array are called from `require` while it searches a package. See `Package searchers`

.. js:data:: package.PATH

    String, path to search for nut packages.

.. js:data:: package.CPATH

    String, path to search for C packages.

.. js:data:: package.CLOADED

    Table containing all loaded dynamic libraries. Keys are library paths, values are UserPointer handles to library.

.. note:: `LOADED`, `SEARCHERS` and `CLOADED` are references to the table/array.
    Assign like ``packae.LOADED = {};`` will only replace table referenced by packae.LOADED not table used by `require`.

+++++++++++++++++
Package functions
+++++++++++++++++

.. js:function:: package.pathsearch(path)

    :param string path: list of file names to be checked.
    :returns: found file name or null

    Search for first readable file in the `path`. `path` is ';' delimited list of file names e.g. "a.txt;../b.txt;c.txt".

.. js:function:: package.pathsplit(path)

    :param string path: list of file names to be split.
    :returns: array with filenames

    Split `path` by ';' excluding empty elements.

.. js:function:: package.replacechar(original,replace,char)

    :param string original: String to replace in.
    :param string replace: String which will replace `the_char`.
    :param integer the_char: `the_char` to replace.
    :returns: new string

    Replaces all occurrences of `the_char` in `original` with `replace`.
    
    For example ``package.replacechar("./?.nut;./?/init.nut","aa/bb",'?')`` will return "./aa/bb.nut;./aa/bb/init.nut".

+++++++++++++++++
Package searchers
+++++++++++++++++

Package searchers are functions with prototype as `package.searcher_function`
they are stored in array `package.SEARCHERS` and are executed sequentially by `require` to search for package loader.

If searcher succeeds it returns package loader. The package loader is closure witch will load the package.
For .nut packages loader is same closure as that returned by call to `loadfile`.

If searcher fails to find package loader it returns null.

.. js:function:: package.searcher_function(package)

    :param string package: virtual name of package.
    :returns: closure or null

^^^^^^^^^^^^^^^^^^^^
Nut package searcher
^^^^^^^^^^^^^^^^^^^^

Converts package name to file name. Searches for that file name in `package.PATH` and loads it as .nut file.

For example if `package.PATH` is "./?.nut;./?/init.nut" and ``require("my.test.pkg")`` is called, following files will be checked:

1.  file "./my/test/pkg.nut"
2.  file "./my/test/pkg/init.nut"

^^^^^^^^^^^^^^^^^^^^
C package searcher
^^^^^^^^^^^^^^^^^^^^

Search for dynamic library in `package.CPATH`. First search is with full package name. Second search is with all but last element of package name.

In both cases if library is found, it is checked for symbol with name "sqload\_" + last element of package name.
If such symbol is found, it is converted to squirrel function and returned as loader.

For example if `package.CPATH` is "./?.so" and ``require("my.test.pkg")`` is called, following files will be checked:

1.  file "./my/test/pkg.so" for symbol "sqload_pkg"
2.  file "./my/test.so" for symbol "sqload_pkg" (this step is executed if package name has '.')

++++++++++++++++++++++++
The package.dynlib class
++++++++++++++++++++++++

The dynlib class uses native API to load dynamic libraryes.
On WINDOWS platform it uses `LoadLibrary` to load Dynamic-link library (or .DLL).
On other platforms it uses `dlopen` from libdl to load shared object (or .so).

Loaded libraryes are stored in two places (if they are not loaded as private):

1.  In `package.CLOADED` table where libraryes are stored by library file path name. So already loaded libraryes can be reused.

2.  In internal (registry table key `CCHAIN`) linked list. So first loaded library is unloaded last.

The libraryes loaded by `is_private` == true, are always loaded (by call to dlopen/LoadLibrary) and are not stored in any internal list.
So they are unloaded when `dynlib` instance is released. A call to `dynlib.register` explicitly adds private library to `CCHAIN` and `package.CLOADED`.

.. js:class:: package.dynlib(path, [is_private])

    :param string path: Path to library.
    :param bool is_private: Optional. States if loaded library will be private to this instance. Default if false.

    Loads a dynamic library `path`.
    
    If `is_private` == false (default), `package.dynlib` first checks in `package.CLOADED` for already loaded library.
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

    Adds private library to `package.CLOADED`. This call will do the work only once and only for private library.

--------------
C API
--------------

+++++++++++++++++++
The package library
+++++++++++++++++++

.. c:function:: SQRESULT sqstd_register_packagelib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    Initialize and register the package library in the given VM.

.. c:function:: SQRESULT sqstd_package_require(HSQUIRRELVM v, const SQChar *package)

    :param HSQUIRRELVM v: the target VM
    :param SQChar* package: package name
    :returns: The package content
    
    Search for 'package', loads it and returns it's content.

.. c:function:: SQRESULT sqstd_package_registerfct( HSQUIRRELVM v, const SQChar *package, SQFUNCTION fct)

    :param HSQUIRRELVM v: the target VM
    :param SQChar* package: package name
    :param SQFUNCTION fct: package loader function
    :returns: The package content
    
    Loads `package` (if not already loaded) from loader function `fct` and returns it's content.

++++++++++++++++++++++++
The package.dynlib class
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
    
    Loads dynamic library `path` using mechanisms of `package.CLOADED` and `CCHAIN` to load and register library.

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

