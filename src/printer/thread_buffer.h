/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011-12 martin.dieringer@gmx.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <string>
#include <sys/types.h>

#include "thread.h"

using namespace std;

// These classes buffer string data between threads.  One or more threads write
// to the buffer and one or more threads can read from the buffer.

// Options exist to drop write data if buffer is full or to read the
// empty string if no data is available.

// The additional write mutex is usefull if multple threads are writting
// to the buffer continuiously because it tends to make the write operations
// more fair.  The code is fully thread-safe without this parameter.

class ThreadBuffer {
protected:
  const size_t size;
  const bool use_write_mutex;
  char *buff;
  char *read_ptr;
  char *write_ptr;
  mutex_t mutex;
  mutex_t write_mutex;

  const string overflow;
  bool last_write_overflowed;

  ntime_t sleep_time;

  const bool line_buffered;
  const unsigned long min_line_len;

  ssize_t SpaceAvailable( void );
  virtual void WaitOnRead( void );
  virtual void WroteToEmpty( void );

  char *ReadRawData( string *str, char *data, char *read_start, unsigned long length, bool null_terminate = true );
  // Copys data from circular buffer, wrapping when necessary.

  size_t Read( string *str, char *data, size_t max_len, bool wait, char *line_start = NULL );
  // Generic function, returns value in str, unless it is NULL, then returns value into data.
  // max_len applies only to data, if used.  str can return unlimited length.

public:
  ThreadBuffer( size_t buffer_size, bool is_line_buffered, const ntime_t &nsleep_time, string overflow_indicator = "", bool use_read_mutex = true, bool use_write_mutex = true, unsigned long min_line_len = 0 );
  virtual ~ThreadBuffer();
  virtual bool Write( const char *data, bool wait, ssize_t datalen = -1 );
  virtual size_t Read( char *data, size_t max_len, bool wait );
  virtual string Read( bool wait );
  virtual bool DataAvailable( void );
  virtual void Flush( void );
};

class SignalingThreadBuffer : public ThreadBuffer {
protected:
  cond_t signal_cond;

  virtual void WaitOnRead( void );
  virtual void WroteToEmpty( void );

public:
  SignalingThreadBuffer( size_t buffer_size, bool is_line_buffered, const ntime_t &nsleep_time, string overflow_indicator = "", bool use_read_mutex = true, bool use_write_mutex = true, unsigned long min_line_len = 0 );
  virtual ~SignalingThreadBuffer();
};

class ThreadBufferReturnData : public ThreadBuffer {
public:
  class ReturnData {
  private:
    mutex_t write_mutex;
    cond_t *return_cond;
    mutex_t *return_mutex;
    unsigned long lines_remaining;
    string data;

  public:
    ReturnData( cond_t *return_cond, mutex_t *return_mutex, bool use_write_mutex = true );
    ~ReturnData();
    void SetLineCount( unsigned long line_count );
    unsigned long LinesRemaining( void );
    void AddLine( const char *line );
    string GetData( void );
  };

protected:
  cond_t return_cond;
  mutex_t return_mutex;

public:
  ThreadBufferReturnData( size_t buffer_size, const ntime_t &nsleep_time, string overflow_indicator = "", bool use_read_mutex = true, bool use_write_mutex = true );
  virtual ~ThreadBufferReturnData();

  virtual bool Write( const char *data, bool wait, ssize_t datalen = -1, ReturnData **return_data = NULL );

  virtual size_t Read( char *data, size_t max_len, bool wait, ReturnData **return_data = NULL );
  virtual string Read( bool wait, ReturnData **return_data = NULL );
  virtual void Flush( void );

  virtual bool WaitForReturnData( ReturnData &return_data );
};
