// Copyright (c) 2011, Arista Networks, Inc.
// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// 	* Redistributions of source code must retain the above copyright notice,
//  	  this list of conditions and the following disclaimer.
// 	* Redistributions in binary form must reproduce the above copyright notice,
// 	  this list of conditions and the following disclaimer in the documentation
// 	  and/or other materials provided with the distribution.
// 	* Neither the name of Arista Networks nor the names of its contributors may
// 	  be used to endorse or promote products derived from this software without
// 	  specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL ARISTA NETWORKS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#include <py3c.h>
#include <QuickTrace/QuickTrace.h>
//#include <iostream>
//using namespace std;

static PyObject * msgFormatDict;

static PyObject *
messageIs(PyObject *self, PyObject *args) {
   QuickTrace::TraceFile * qtf = QuickTrace::theTraceFile;
   if( !qtf ) Py_RETURN_NONE;
   char const * file;
   int line;
   char const * msg;
   char const * fmt;
   if( !PyArg_ParseTuple( args, "siss:messageIs",
                          &file, &line, &msg, &fmt ) ) {
      return NULL;
   }
   // cout << qtf << "," << file << "," << line << "," << msg << "," << fmt << endl;
   QuickTrace::MsgId msgId = 0;
   QuickTrace::MsgDesc qtmd( qtf, &msgId, file, line );
   qtmd.formatString().put( fmt );
   qtmd << msg;
   qtmd.finish();
   PyObject * pyMsgId = PyInt_FromLong( (long)msgId );
   PyObject * pyfmt = PyTuple_GET_ITEM( args, 3 );
   PyDict_SetItem( msgFormatDict, pyMsgId, pyfmt );
   return pyMsgId;
}

static PyObject *
trace(PyObject *self, PyObject *args) {
   QuickTrace::TraceFile * qtf = QuickTrace::theTraceFile;
   if( !qtf ) Py_RETURN_NONE;
   int len = PyTuple_GET_SIZE( args );
   // msgid
   if( len < 3 ) {
      // set error
      return NULL;
   }
   PyObject * pymid = PyTuple_GET_ITEM( args, 0 );
   long mid = PyInt_AS_LONG( pymid );
   if( mid == 0 ) {
      Py_RETURN_NONE;
   }
   // check for disabled
   PyObject * fmt = PyDict_GetItem( msgFormatDict, pymid );
   if( !fmt ) {
      Py_RETURN_NONE;
   }
   char const * fmtString = PyStr_AsString( fmt );
   long level = PyInt_AS_LONG( PyTuple_GET_ITEM( args, 1 ));
   QuickTrace::RingBuf & ringBuf = qtf->log(level);
   ringBuf.startMsg( qtf, mid );
   long result = 0;
   PyObject * targs = PyTuple_GET_ITEM( args, 2 );
   unsigned int tlen = PyList_GET_SIZE( targs );
   unsigned int fmtLen = strlen( fmtString );
   // We iterate over the format string in case we
   // get the wrong number of args in targs.  See
   // test/QtVaryingArgs.py
   for( unsigned int i = 0; i < fmtLen; i += 2 ) {
      PyObject * o;
      if ( i / 2 < tlen ) {
         o = PyList_GET_ITEM( targs, i / 2 );
      } else {
         o = Py_None;
      }
      switch( fmtString[i] ) {
       case 'i': {
         long x = PyInt_AsLong(o);
         ringBuf << x;
         result = 1;
         break;
       }
       case 'q': {
          long long ll = PyLong_AsLongLong(o);
         ringBuf << ll;
          result = 4;
          break;
       }
       case 'b': {
         bool b = ( o == Py_True );
         ringBuf << b;
         result = 3;
         break;
       }
       case 'd': {
         double x = PyFloat_AsDouble(o);
         ringBuf << x;
         result = 5;
         break;
       }
       case 'p':
       default: {
          const char * c;
          // If not a string, convert, otherwise, bump reference, so either way,
          // we just need to decrement refcount on 'o' when done.
          if( !PyStr_Check( o ) ) {
             o = PyObject_Str( o );
          } else {
            Py_INCREF( o );
          }

          if ( o != NULL ) {
             c = PyUnicode_AsUTF8( o );
             PyObject *bytes;
             if ( c == NULL ) {
                // Unhappy path - strict UTF-8 decode failed, try the hard way
                // with "surrogateescape"
                PyErr_Clear();
                bytes = PyUnicode_AsEncodedString( o, "utf-8", "surrogateescape" );

                // If we failed to convert to bytes, add some indicator to the trace.
                if ( bytes == NULL ) {
                   c = "<invalid UTF-8>";
                   PyErr_Clear();
                   result = 6;
                } else {
                   c = PyBytes_AsString( bytes );
                   // This should not happen: it would require
                   // PyUnicode_AsEncodedString to return something other than
                   // a bytes object, which it does not.
                   if ( c == NULL ) {
                      c = "<invalid UTF-8>";
                      PyErr_Clear();
                      result = 7;
                   } else {
                      result = 8;
                   }
                }
             } else {
                // happy case - got cached UTF-8 encoding.
                result = 2;
                bytes = NULL;
             }
             ringBuf << c;
             if ( o != NULL ) {
                Py_DECREF( o );
             }
             if ( bytes != NULL ) {
                Py_DECREF( bytes );
             }
          }
          break;
       }
       case ',': {
          assert( !"Format string malformed" );
       }
      }
      //std::cout << "type " << result << " ";

      if( PyErr_Occurred() && o == Py_None ) {
         // We got an error, probably because a wrong argument type is passed.
         // This can happen if we are not careful and mix integer/float numbers
         // and None. Let's just handle it instead of throwing an exception,
         // since it's just cosmetic. Note the number we get in the log is
         // -1/-1.0, which indicates an error.
         PyErr_Clear();
      }

   }
   //std::cout << std::endl;
   ringBuf.endMsg();
   if( PyErr_Occurred() ) {
      // We got an error, probably because a wrong argument type is passed.
      // See BUG51574.  Return NULL so an exception will be thrown.
      // Note that a qtrace message is still generated, with the erroneous
      // argument getting an invalid value (e.g., -1 if a string is passed
      // as an integer).
      return NULL;
   }
   return PyInt_FromLong((long)result);
}

static PyObject *
close( PyObject * self, PyObject * args ) {
   QuickTrace::close();
   return Py_BuildValue( "" );
}

static PyObject*
getMaxStringTraceLen(PyObject *self, PyObject *args) {
   return Py_BuildValue( "i", QuickTrace::qtMaxStringLen );
}

/*----------------------------------------------------------------------------*
 *                                Module Setup                                *
 *----------------------------------------------------------------------------*/
static PyMethodDef module_methods[] = {
   { ( char * )"trace", trace, METH_VARARGS, "Trace a message" },
   { ( char * )"messageIs", messageIs, METH_VARARGS, "Create a message" },
   { ( char * )"getMaxStringTraceLen",
     getMaxStringTraceLen,
     METH_NOARGS,
     "Get max string trace length" },
   { ( char * )"close", close, METH_NOARGS, "Get max string trace length" },
   { NULL } /* sentinel */
};

extern "C" {
/* declarations for DLL export */
static struct PyModuleDef QuickTrace_module =
{
   .m_base = PyModuleDef_HEAD_INIT,
   .m_name = "libQuickTracePy",
   .m_doc = "QuickTrace C++ back-end",
   .m_size = -1,
   .m_methods = module_methods
};

MODULE_INIT_FUNC( libQuickTracePy )
{
   PyObject* m = PyModule_Create( &QuickTrace_module );
   msgFormatDict = PyDict_New();
   PyModule_AddObject(m, "format", msgFormatDict);
   return m;
}
}
