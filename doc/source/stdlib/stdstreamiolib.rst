.. _stdlib_stdstreamreaderlib:

=========================
The Stream I/O library
=========================

The streamio library implements buffered access to generic stream.

All symols from streamio library are provided in package "streamio".

--------------
Squirrel API
--------------

+++++++++++++++++++++++++
The streamio.reader class
+++++++++++++++++++++++++

    The streamreader class implements an abstract read only stream with ability to mark and reset its position. Optional buffering is also available.
    The streamreader class extends class stream.

.. js:class:: streamio.reader(source[,owns,buffer_size])

    :param stream source: stream to read from
    :param bool owns: if source stream will be closed when streamreader is closed. Default is false.
    :param int buffer_size: buffer size to be used while reading source stream. Default is 0 - no buffering.

    If successfully created textreader instance will reference the `source` stream.
	
.. js:function:: reader.mark(readAheadLimit)

    :param int readAheadLimit: Limit on the number of characters that may be read while still preserving the mark. After reading more than this many characters, attempting to reset the stream may fail.

    Marks the present position in the stream. Subsequent calls to reset() will attempt to reposition the stream to this point.

.. js:function:: reader.reset()

    If the stream has been marked, then attempt to reposition it at the mark. Return value is 0.
    If the stream has not been marked or readAheadLimit is reached, nothing is done. Return value is 1.

--------------
C API
--------------

.. _sqstd_register_streamiolib:

.. c:function:: SQRESULT sqstd_register_streamiolib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initialize and register the streamreader library in the given VM.


+++++++++++++++++++
Streamreader Object
+++++++++++++++++++

    The streamreader object is represented by opaque structure SQSRDR. SQSRDR can be freely casted to SQSTREAM.
    
.. c:function:: SQSRDR sqstd_streamreader( SQSTREAM source,SQBool owns,SQInteger buffer_size)

    :param SQFILE source: a stream to read from
    :param SQBool owns: tells if stream `source` will be closed when streamreader is closed
    :param SQInteger buffer_size: buffer size to be used while reading source stream.
    :returns: a streamreader object
    
    Creates a streamreader to read from stream `source`. If `buffer_size` is 0 no buffering is used.
    
    Streamreader must be released by call to sqstd_frelease.
    
.. note:: Releasing streamreader does NOT release the `source` stream.

.. c:function:: SQInteger sqstd_srdrmark(SQSRDR srdr,SQInteger readAheadLimit)

    :param SQSRDR srdr: streamreader object
    :param SQInteger readAheadLimit: read ahead limit

    Marks the present position in the stream. Subsequent calls to reset() will attempt to reposition the stream to this point.

.. c:function:: SQInteger sqstd_srdrreset(SQSRDR srdr)

    :param SQSRDR srdr: streamreader object
    :returns: zero on success, non-zero otherwise.

    Repositions streamreader to marked position.

