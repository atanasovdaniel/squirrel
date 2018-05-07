.. _stdlib_stdstreamlib:

========================
The Stream library
========================

The stream library implements generic stream abstraction.

All symols from stream library are provided in package "stream".

--------------
Squirrel API
--------------

+++++++++++++++
Package Symbols
+++++++++++++++


.. js:function:: dostream(stream, sourcename, [raiseerror])

    compiles a squirrel script or loads a precompiled one and executes it.
    returns the value returned by the script or null if no value is returned.
    The parameter 'sourcename' is a string used to identify source of stream.
    if the optional parameter 'raiseerror' is true, the compiler error handler is invoked
    in case of a syntax error. If raiseerror is omitted or set to false, the compiler
    error handler is not invoked.
    The function can handle different character encodings, UTF8 with and without prefix
    and UCS-2 prefixed(both big endian an little endian).
    If the source stream is not prefixed UTF8 encoding is used as default.

.. js:function:: loadstream(stream, sourcename, [raiseerror])

    compiles a squirrel script or loads a precompiled one an returns it as as function.
    The parameter 'sourcename' is a string used to identify source of stream.
    if the optional parameter 'raiseerror' is true, the compiler error handler is invoked
    in case of a syntax error. If raiseerror is omitted or set to false, the compiler
    error handler is not invoked.
    The function can handle different character encodings, UTF8 with and without prefix
    and UCS-2 prefixed(both big endian an little endian).
    If the source stream is not prefixed UTF8 encoding is used as default.

.. js:function:: writeclosuretostream(stream, closure)

    serializes a closure to a bytecode stream. The serialized stream can be loaded
    using loadstream() and dostream().


+++++++++++++++++++++++
The stream.stream class
+++++++++++++++++++++++

    The stream class is an abstract class generalizing sources or sinks of data.
    
    The stream class is the base class of: blob, file, streamreader, textreader and textwriter.

.. js:function:: stream.close()

    closes the stream. Returns zero on success.

.. js:function:: stream.eos()

    returns a non null value if the read/write pointer is at the end of the stream.

.. js:function:: stream.flush()

    flushes the stream. Returns zero on success.

.. js:function:: stream.len()

    returns the length of the stream. If stream is not seekable result is 0

.. js:function:: stream.print(text)

    :param string text: a string to be writen
	
    writes a string to the stream.
	
.. note:: How text is encoded depends on squirrel configuration. (See textwriter and textwriter)

.. js:function:: stream.readblob(size)

    :param int size: number of bytes to read

    read n bytes from the stream and returns them as blob

.. js:function:: stream.readline()

    read a line of text from the stream and returns it as string
	
.. note:: How text is encoded depends on squirrel configuration. (See textwriter and textwriter)

.. js:function:: stream.readn(type)

    :param int type: type of the number to read

    reads a number from the stream according to the type parameter.

    `type` can have the following values:

+--------------+--------------------------------------------------------------------------------+----------------------+
| parameter    | return description                                                             |  return type         |
+==============+================================================================================+======================+
| 'l'          | processor dependent, 32bits on 32bits processors, 64bits on 64bits processors  |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'i'          | 32bits number                                                                  |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 's'          | 16bits signed integer                                                          |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'w'          | 16bits unsigned integer                                                        |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'c'          | 8bits signed integer                                                           |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'b'          | 8bits unsigned integer                                                         |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'f'          | 32bits float                                                                   |  float               |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'd'          | 64bits float                                                                   |  float               |
+--------------+--------------------------------------------------------------------------------+----------------------+

.. js:function:: stream.seek(offset [,origin])

    :param int offset: indicates the number of bytes from `origin`.
    :param int origin: origin of the seek

                        +--------------+-------------------------------------------+
                        |  'b'         |  beginning of the stream                  |
                        +--------------+-------------------------------------------+
                        |  'c'         |  current location                         |
                        +--------------+-------------------------------------------+
                        |  'e'         |  end of the stream                        |
                        +--------------+-------------------------------------------+

    Moves the read/write pointer to a specified location.

.. note:: If origin is omitted the parameter is defaulted as 'b'(beginning of the stream).

.. js:function:: stream.tell()

    returns the read/write pointer absolute position. On error returns -1.

.. js:function:: stream.writeblob(src)

    :param blob src: the source blob containing the data to be written

    writes a blob to the stream

.. js:function:: stream.writen(n, type)

    :param number n: the value to be written
    :param int type: type of the number to write

    writes a number in the stream formatted according to the `type` pamraeter

    `type` can have the following values:

+--------------+--------------------------------------------------------------------------------+
| parameter    | return description                                                             |
+==============+================================================================================+
| 'i'          | 32bits number                                                                  |
+--------------+--------------------------------------------------------------------------------+
| 's'          | 16bits signed integer                                                          |
+--------------+--------------------------------------------------------------------------------+
| 'w'          | 16bits unsigned integer                                                        |
+--------------+--------------------------------------------------------------------------------+
| 'c'          | 8bits signed integer                                                           |
+--------------+--------------------------------------------------------------------------------+
| 'b'          | 8bits unsigned integer                                                         |
+--------------+--------------------------------------------------------------------------------+
| 'f'          | 32bits float                                                                   |
+--------------+--------------------------------------------------------------------------------+
| 'd'          | 64bits float                                                                   |
+--------------+--------------------------------------------------------------------------------+


--------------
C API
--------------

.. _sqstd_register_streamlib:

.. c:function:: SQRESULT sqstd_register_streamlib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initialize and register the stream library in the given VM.

++++++++++++++++
The stream class
++++++++++++++++

    The stream object is represented by opaque structure SQSTREAM.

.. c:function:: SQInteger sqstd_sread(void *buffer, SQInteger size, SQSTREAM stream)

    :param void* buffer: buffer to read to
    :param SQInteger size: size in bytes to read from the stream
    :param SQSTREAM stream: the stream to read from
	:returns: the number of bytes read or 0 on error
    
    Reads `size` bytes from `stream` and stores them to `buffer`.
	
.. c:function:: SQInteger sqstd_swrite(const void *buffer, SQInteger size, SQSTREAM stream)

    :param void* buffer: buffer with data to be writen
    :param SQInteger size: size in bytes to write from to stream
    :param SQSTREAM stream: the stream to write to
	:returns: the number of bytes writen or 0 on error

    Writes `size` bytes stored in `buffer` to `stream`.

.. c:function:: sqstd_sseek(SQSTREAM stream, SQInteger offset, SQInteger origin)

    :param SQSTREAM stream: the stream
    :param SQInteger offset: offset in file relative to `origin`
    :param SQInteger origin: origin of `offset`
    :returns: 0 on success or non-zeto on failure.

    Sets position in the stream.
    `origin` can be one of:

        +--------------+-------------------------------------------+
        |  SQ_SEEK_SET |  beginning of the stream                  |
        +--------------+-------------------------------------------+
        |  SQ_SEEK_CUR |  current location                         |
        +--------------+-------------------------------------------+
        |  SQ_SEEK_END |  end of the stream                        |
        +--------------+-------------------------------------------+

.. c:function:: SQInteger sqstd_stell(SQSTREAM stream)

    :param SQSTREAM stream: the stream
    :returns: the position in the stream or -1 on error.

.. c:function:: SQInteger sqstd_sflush(SQSTREAM stream)

    :param SQSTREAM stream: the stream
    :returns: 0 on success or non-zeto on failure.

    Flushes the stream

.. c:function:: SQInteger sqstd_seof(SQSTREAM stream)

    :param SQSTREAM stream: the stream
    :returns: non-zero if end of stream is reached, zero if not.
    
    Checks if end of stream was reached.
    
.. c:function:: SQInteger sqstd_sclose(SQSTREAM stream)

    :param SQSTREAM stream: the stream
    :returns: 0 on success or non-zeto on failure..
    
    Closes the stream. Returns zero on success or non-zeto on failure.

.. c:function:: void sqstd_srelease(SQSTREAM stream)

    :param SQSTREAM stream: the stream

    Releases (frees) the stream object. All stream objects must be released.

++++++++++++++
Stream Object
++++++++++++++

    TBD.

++++++++++++++++++++++++++++++++
Script loading and serialization
++++++++++++++++++++++++++++++++

.. c:function:: SQRESULT sqstd_loadstream(HSQUIRRELVM v, SQSTREAM stream, const SQChar* sourcename, SQBool printerror)

    :param HSQUIRRELVM v: the target VM
    :param SQSTREAM stream: stream to be loaded
    :param SQChar* sourcename: name used to identify the source
    :param SQBool printerror: if true the compiler error handler will be called if a error occurs
    :returns: an SQRESULT

    Compiles a squirrel script or loads a precompiled one an pushes it as closure in the stack.
    The function can handle different character encodings, UTF8 with and without prefix and UCS-2 prefixed(both big endian an little endian).
    If the source stream is not prefixed UTF8 encoding is used as default.

.. c:function:: SQRESULT sqstd_dostream(HSQUIRRELVM v, SQSTREAM stream, const SQChar* sourcename, SQBool retval, SQBool printerror)

    :param HSQUIRRELVM v: the target VM
    :param SQSTREAM stream: stream to be loaded
    :param SQChar* sourcename: name used to identify the source
    :param SQBool retval: if true the function will push the return value of the executed script in the stack.
    :param SQBool printerror: if true the compiler error handler will be called if a error occurs
    :returns: an SQRESULT
    :remarks: the function expects a table on top of the stack that will be used as 'this' for the execution of the script. The 'this' parameter is left untouched in the stack.

    Compiles a squirrel script or loads a precompiled one and executes it.
    Optionally pushes the return value of the executed script in the stack.
    The function can handle different character encodings, UTF8 with and without prefix and UCS-2 prefixed(both big endian an little endian).
    If the source stream is not prefixed, UTF8 encoding is used as default. ::

        sq_pushroottable(v); //push the root table(were the globals of the script will are stored)
        sqstd_dostream(v, test_stream, _SC("test_stream"), SQFalse, SQTrue);// also prints syntax errors if any

.. c:function:: SQRESULT sqstd_writeclosuretostream(HSQUIRRELVM v, SQSTREAM stream)

    :param HSQUIRRELVM v: the target VM
    :param SQSTREAM stream: destination stream of serialized closure
    :returns: an SQRESULT

    serializes the closure at the top position in the stack as bytecode in the stream.

