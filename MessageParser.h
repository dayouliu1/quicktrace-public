// Copyright (c) 2018, Arista Networks, Inc.
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

#ifndef QUICKTRACE_MESSAGEPARSER_H
#define QUICKTRACE_MESSAGEPARSER_H

#include <cstdint>
#include <string>

struct Message {
 public:
   Message() : tsc_( 0 ), msgId_( 0 ), lineno_( 0 ) {}
   Message( uint64_t tsc, std::string filename, uint32_t lineno, std::string msg,
           std::string fmt, uint32_t msgId ) :
         tsc_( tsc ), msgId_( msgId ), lineno_( lineno ),
         filename_( std::move( filename ) ), msg_( std::move( msg ) ),
         fmt_( std::move( fmt ) ) {}
   uint64_t tsc() { return tsc_; }
   const std::string & filename() const { return filename_; }
   uint32_t lineno() { return lineno_; }
   const std::string & msg() const { return msg_; }
   const std::string & fmt() const { return fmt_; }
   uint32_t msgId() { return msgId_; }
 private:
   uint64_t tsc_;
   uint32_t msgId_;
   uint32_t lineno_;
   std::string filename_;
   std::string msg_;
   std::string fmt_;
};

class MessageParser {
public:
   MessageParser();

   void initialize( const void * fpp, int fd );
   bool more() const;
   Message parse();
   void recheck();

private:
   uint32_t index_;
   uint32_t version_;
   int fd_;
   const char * base_;
   const char * p_;
   const char * end_;
};

#endif
