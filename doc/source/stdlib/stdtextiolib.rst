.. _stdlib_stdtextiolib:

=============================
The Text Input/Output library
=============================

The text i/o library implements reading/writing text in different encodings.

All symols from stream library are provided in package "textio".

--------------
Squirrel API
--------------

+++++++++++++++++++++++
The textio.reader class
+++++++++++++++++++++++

	The reader class implements an abstract read only stream. It is used to read text with arbitrary encoding from a stream.

    The reader class extends class stream.

.. js:class:: textio.reader(source[,owns,encoding,guess])

    :param stream source: stream to read from
    :param bool owns: if source stream will be closed when textreader is closed. Default is false.
    :param string encoding: encoding name. Default is "UTF-8".
    :param bool guess: try to guess encoding from BOM in source stream, in this case `encoding` is used as fallback. Default is false.

    If successfully created textreader instance will reference the `source` stream.
    
    Currently supported encodings are: ASCII; UTF-8; UTF-16, UTF-16BE, UCS-2BE; UCS-2, UCS-2LE, UTF-16LE. Encoding UCS-2 is supported only as alias for UTF-16.

+++++++++++++++++++++++
The textio.writer class
+++++++++++++++++++++++

	The writer class implements an abstract write only stream. It is used to write text with arbitrary encoding to a stream.

    The writer class extends class stream.
    
.. js:class:: textio.writer(destination[,owns,encoding])

    :param stream destination: stream to write to
    :param bool owns: if destination stream will be closed when textwriter is closed. Default is false.
    :param string encoding: encoding name. Default is "UTF-8".
    
    If successfully created textwriter instance will reference the `destination` stream.

    For encodings see textreader.

--------------
C API
--------------

.. _sqstd_register_textiolib:

.. c:function:: SQRESULT sqstd_register_textiolib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initialize and register the textio library in the given VM.

+++++++++++++++++++
Textreader Object
+++++++++++++++++++

.. c:function:: SQSTREAM sqstd_textreader(SQSTREAM source,SQBool owns,const SQChar *encoding,SQBool guess)

    :param SQSTREAM source: stream object to read from
    :param SQBool owns: tells if stream `source` will be closed when textreader is closed
    :param SQChar* encoding: encoding name. If NULL default encoding "UTF-8" is used.
    :param SQBool guess: If non-zero - try to guess encoding by reading BOM from `source`.
    :returns: a stream representing textreader object
    
    Creates textreader to read from `source` stream.
    
    Textreader must be released by call to sqstd_srelease.
    
.. note:: Releasing textreader does NOT release the `source` stream.

+++++++++++++++++++
Textwriter Object
+++++++++++++++++++

.. c:function:: SQSTREAM sqstd_textwriter(SQSTREAM destination,SQBool owns,const SQChar *encoding)

    :param SQSTREAM destination: stream object to write to
    :param SQBool owns: tells if stream `destination` will be closed when textwriter is closed
    :param SQChar* encoding: encoding name. If NULL default encoding "UTF-8" is used.
    :returns: a stream representing textwriter object

    Creates textwriter to write to `destination` stream.
    
    Textwriter must be released by call to sqstd_srelease.
    
.. note:: Releasing textwriter does NOT release the `destination` stream.
