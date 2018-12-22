.. _stdlib_stdpackagelib:

=============================
The Package library
=============================
The package library provides functionality to search and load packages (modules, libraryes).

It has one global function `require` used to find package content by it's name.

The libaray itself provides two packages:

    "package" - core package functionality

    "package.nut" - searcher for nut packages

--------
Overview
--------

The "packages" are named value of Squirrel’s basic types (usually table, class, function).

The package name is string with assumption that '.' (dot) char is used to represent some kind of hierarchy between packages.
A good package names will not start, end nor contain sequences of dots.

A "package" content can be requested by calling global function `require` ::

    local mylib = require("my.lib");

this will search and load package "my.lib". Further it's content can be accessed as ::

    mylib.myfct("hello");



Package library can be accessed via package "package". It contains data used by function `require` to search and load a package.
There are two variables in package `LOADED` and `SEARCHERS`:

++++++++++++++
package.LOADED
++++++++++++++
LOADED is a table containing already loaded packages. `require` first checks there if a package is loaded.
Keys in this table are package names, values are package content.

Assigning to this table can be used to set content of package. ::

    require("package").LOADED["my.lib"] <- { a = "one", b = "two" };

subsequent require of package "my.lib" will return assigned table ::

    ::print( require("my.lib").b + "\n");

will print "two".

+++++++++++++++++
package.SEARCHERS
+++++++++++++++++
SEARCHERS is an array of searcher functions. `require` executes sequentially functions from this array while searching new package.
Initially array contains function SEARCH from "package.nut" package.

Searcher function takes at least one argument - package name. If searcher finds requested package it returns a package loader function.

If searcher failed to find package, it will return `null`, `require` then will call next searcher from SEARCHERS array.

Package loader is a function called to return the content of package.

+++++++++++++++++
Example
+++++++++++++++++
A minimalistic example of searcher for ".nut" files function is ::

    function my_searcher( pkg_name, ...)
    {
        try
        {
            return ::loadfile( "./" + pkg_name + ".nut);
        }
        catch( e)
        {
            return null;
        }
    }

if `loadfile` succeeds loading file the result will be package loader function, else result is `null`.

The `my_searcher` function then can be added to package.SEARCHERS. ::

    require("package").SEARCHERS.insert(0,my_searcher);

Then requiring some package ::

    local mypkg = require("mypkg");

Now if file with name "./mypkg.nut" exists in current folder, local variable `mypkg` will contain the result of execution of this file.

This example "nut" package file contains one function called `myfunct` ::

    local PKG = {};
    
    function PKG::myfunct( str)
    {
        ::print( "my:" + str + "\n");
    }
    
    return PKG;

-----------------
Package "package"
-----------------

++++++++++++
Squirrel API
++++++++++++

^^^^^^^^^^^^^^
Global Symbols
^^^^^^^^^^^^^^

.. js:function:: require(package,[hint])

    :param string package: virtual name of package.
    :param string hint: Optional. Default is "".

    Search for 'package', loads it and returns it's content.
    
    First `require` looks in `package.LOADED` to check if the `package` is already loaded. If it is, value stored in `package.LOADED[package]` is returned.
    
    Second `require` calls sequentially package searchers from `package.SEARCHERS` to find a `package`.

^^^^^^^^^^^^
Package data
^^^^^^^^^^^^

.. js:data:: package.LOADED

    Table, containing loaded packages, keys are package names, values are package content. `require` first searches in this table for already loaded packages.
    
.. js:data:: package.SEARCHERS

    Array, containing searcher functions. Functions in this array are called from `require` while it searches a package. See `Package searchers`

.. note:: `LOADED` and `SEARCHERS` are references to the table/array.
    Assign like ``packae.LOADED = {};`` will only replace table referenced by packae.LOADED not table used by `require`.

^^^^^^^^^^^^^^^^^
Package functions
^^^^^^^^^^^^^^^^^

.. js:function:: package.searchfile(name,path,suffixes)

    :param string name: name to search.
    :param string path: list of paths separated by ';'.
    :param string suffixes: list of suffixes separated by ';'.
    :returns: found file name or null

    Search for readable file using combinations of `path`, `suffixes` and `name`. `path` and `suffixes` are lists separated by ';' char (only non-empty elements are used).
    The "?" chars in `suffixes` are replaced by `name`, then combination path/suffix is used to check if file with this name can be opened for reading.

    Calling ``package.searchfile( "X", "P1;P2", "/?.S1;/?/S2")`` will search for theese files in this order:

    * P1/X.S1
    * P1/X/S2
    * P2/X.S1
    * P2/X/S2

.. js:function:: package.searchcbk(replace,paths,suffixes,callback,...)

    :param string replace: string to replace '?' char in suffixes
    :param string paths: list of paths separated by ';'.
    :param string suffixes: list of suffixes separated by ';'.
    :param string callback: function called to check combination
    :returns: accepted combination or null

    Same as `searchfile` but for every combination calls `callback`. `callback` is called with first argument combination to check. Variadic arguments (if any) are passed after first argument.
    If `callback` terurn true, search is stopped and combination is returned as result. If `callback` don't return true for any combination, `null` is returned.

+++++
C API
+++++

.. c:function:: SQRESULT sqstd_package_require(HSQUIRRELVM v, const SQChar *package, const SQChar *hint)

    :param HSQUIRRELVM v: the target VM
    :param SQChar* package: package name
    :param SQChar* hint: hint to be used (can be `NULL`)
    :returns: an SQRESULT

    Require a `package`, if function succeeds to load the `package` it returns `SQ_OK` and requested package is pushed on top.
    If `package` can't be found function returns `SQ_ERROR`.


.. c:function:: SQRESULT sqstd_package_registerfct( HSQUIRRELVM v, const SQChar *package, SQFUNCTION fct)

    :param HSQUIRRELVM v: the target VM
    :param SQChar* package: package name
    :param SQFUNCTION loader_fct: package loader
    :returns: an SQRESULT

    Loads a `package` fron explicit loader function `loader_fct`.
    If `package` is not loaded call a function `loader_fct` to get a package body.

    On success SQ_OK is returnes and package is pushed on top of stack, else (if `loader_fct` fail) function returns SQ_ERROR.

.. c:function:: SQRESULT sqstd_package_getsearchers(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT

    Get the `package.SEARCHERS` array.

    On success, returns SQ_OK and the `package.SEARCHERS` array is pushed on top of stack. On failure returns SQ_ERROR.

.. c:function:: SQRESULT sqstd_register_packagelib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    Initialize the package library in the given VM.
    Also loads packages "package" and "package.nut".


---------------------
Package "package.nut"
---------------------

The package provides a searcher for ".nut" files.



++++++++++++
Squirrel API
++++++++++++

^^^^^^^^^^^^
Package data
^^^^^^^^^^^^

.. js:data:: package.nut.PATH

    String, a ';' separated list of paths used to search for nut package files.

.. js:data:: package.nut.SUFFIXES

    String, a ';' separated list of suffixes used to search for nut package files. Initialy it contains "?.nut;?/init.nut".

^^^^^^^^^^^^^^^^^
Package functions
^^^^^^^^^^^^^^^^^

.. js:function:: package.nut.SEARCH(package,...)

    :param string package: virtual name of package.

    Searcher for ".nut" package files.

+++++
C API
+++++

.. c:function:: SQRESULT sqstd_package_nutaddpath(HSQUIRRELVM v, const SQChar *path);

    :param HSQUIRRELVM v: the target VM
    :param SQChar* path: path to add
    :returns: an SQRESULT

    Add `path` to `package.nut.PATH`. Newly added paths are searched first.

