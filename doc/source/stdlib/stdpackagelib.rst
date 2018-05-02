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

    Array, patterns to search for nut packages.

.. js:data:: package.CPATH

    Array, patterns to search for C packages. (if dynlib if enabled)

.. note:: `LOADED`, `SEARCHERS`, `PATH` and `CPATH` are references to the table/array.
    Assign like ``packae.LOADED = {};`` will only replace table referenced by packae.LOADED not table used by `require`.

+++++++++++++++++
Package functions
+++++++++++++++++

.. js:function:: package.addpath(path)

    :param string path: string or array of strings.

    Add path(s) to be searched for nut packages. New added paths are searched first.

.. js:function:: package.addcpath(path)

    :param string path: string or array of strings.

    Add path(s) to be searched for C packages (if dynlib if enabled). New added paths are searched first.

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

^^^^^^^^^^^^^^^^^^^^^^^^
Builtin package searcher
^^^^^^^^^^^^^^^^^^^^^^^^

Searches for exact match of package name in the list added by `sqstd_package_addbuiltins`. If match is found, coresponding function if returned as loader.

^^^^^^^^^^^^^^^^^^^^
Nut package searcher
^^^^^^^^^^^^^^^^^^^^

Searches for that file name in `package.PATH` and loads it as .nut file.

For example if `package.PATH` is ``["./?.nut","./?/init.nut"]`` and ``require("my.test.pkg")`` is called, following files will be checked:

1.  file "./my/test/pkg.nut"
2.  file "./my/test/pkg/init.nut"

^^^^^^^^^^^^^^^^^^^^
C package searcher
^^^^^^^^^^^^^^^^^^^^

Search for dynamic library in `package.CPATH` containing symbol ``"sqload\_"`` + package name with '.' replaced with '_'.
If such symbol is found, it is converted to squirrel function and returned as loader.

For example if `package.CPATH` is ["./?.so"] and ``require("my.test.pkg")`` is called, following files will be checked for symbol "sqload_my_test_pkg":

1.  file "./my/test/pkg.so"
2.  file "./my/test.so"
3.  file "./my.so"

--------------
C API
--------------

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

.. c:function:: SQRESULT sqstd_package_addpath( HSQUIRRELVM v, const SQChar *path)

    :param HSQUIRRELVM v: the target VM
    :param SQChar* path: path to add
    :returns: an SQRESULT
    
    Add path to be searched for nut packages. New added paths are searched first.

.. c:function:: SQRESULT sqstd_package_addcpath( HSQUIRRELVM v, const SQChar *path)

    :param HSQUIRRELVM v: the target VM
    :param SQChar* path: path to add
    :returns: an SQRESULT
    
    Add path to be searched for C packages (if dynlib if enabled). New added paths are searched first.

.. c:function:: void sqstd_package_addbuiltins( const SQSTDPackageList *list)

    :param SQSTDPackageList* list: list of packages
    
    Add list of builtin packages.

