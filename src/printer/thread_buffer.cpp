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

#include <string.h>
#include <sys/types.h>

#include "thread_buffer.h"

ThreadBuffer::ThreadBuffer( size_t buffer_size, bool is_line_buffered, const ntime_t &nsleep_time, string overflow_indicator, bool use_read_mutex, bool use_write_mutex, unsigned long min_line_len ) :
  size( buffer_size + 10 + overflow_indicator.length() ),
  use_write_mutex( use_write_mutex ),
  overflow( overflow_indicator ),
  line_buffered( is_line_buffered ),
  min_line_len( min_line_len ) {
  
  read_ptr = write_ptr = buff = new char[ size + 10 ];
  mutex_init( &mutex );
  if ( use_write_mutex )
    mutex_init( &write_mutex );
  
  sleep_time = nsleep_time;
  
  last_write_overflowed = false;
}

ThreadBuffer::~ThreadBuffer() {
  delete [] buff;
  mutex_destroy( &mutex );
  if ( use_write_mutex )
    mutex_destroy( &write_mutex );
}

ssize_t ThreadBuffer::SpaceAvailable( void ) {
  // Determine space remaining in buffer, being sure to always leave room
  // for the overflow string
  // 10 is a padding factor to ensure than a simple off by one errors
  // never cause the write pointer to advance pass the read pointer
  return size - ( ( write_ptr - read_ptr ) % size ) - 10 - overflow.length();
}

bool ThreadBuffer::Write( const char *data, bool wait, ssize_t datalen ) {
  // Length check
  if ( datalen < 0 )
    datalen = strlen( data );
  if ( (size_t)datalen > size - 10 - overflow.length() - 5 ) {
    return false;
  }
  
  // Lock mutex(es)
  if ( use_write_mutex )
    if ( mutex_lock( &write_mutex ) != 0 )
      return false;
  if ( mutex_lock( &mutex ) != 0 ) {
    if ( use_write_mutex )
      mutex_unlock( &write_mutex );
    return false;
  }

  // Determine if enough space is available
  // If the buffer is line buffered and the data to write does not
  // end in a newline, one will be added.
  // datalen is the length of provided data
  // fulldatalen is the length including the appended newline
  ssize_t fulldatalen = datalen;
  if ( line_buffered && data[ datalen - 1 ] != '\n' )
    fulldatalen++;
  
  if ( fulldatalen > SpaceAvailable() ) {
    if ( wait ) {
      // Wait until enough space is available
      while ( fulldatalen > SpaceAvailable() ) {
	mutex_unlock( &mutex );
	nsleep( &sleep_time );
	mutex_lock( &mutex );
      }
    } else if ( last_write_overflowed || overflow.length() == 0 ) {
      // Wrote overflow string last time, don't write it again, just give up
      mutex_unlock( &mutex );
      if ( use_write_mutex )
	mutex_unlock( &write_mutex );
      return false;
    } else {
      // Write the overflow string
      fulldatalen = datalen = overflow.length();
      data = overflow.c_str();
      if ( line_buffered && data[ datalen - 1 ] != '\n' )
	fulldatalen++;
      last_write_overflowed = true;
    }
  }
  
  // Determine if the new data "wraps around" at the end of the buffer memory
  // space.  The variable split contains the number of bytes to write before
  // the end of the memory space.
  size_t split = datalen;
  char *new_write_ptr = write_ptr + fulldatalen;
  char *old_write_ptr = write_ptr;
  
  if ( new_write_ptr >= buff + size ) {
    new_write_ptr -= size;
  }
  
  if ( write_ptr + datalen >= buff + size ) {
    split = buff - write_ptr + size;
  }
  
  // First change the data
  memcpy( write_ptr, data, split );
  memcpy( buff, &data[split], datalen - split );
  
  // If a newline needs appended, add it now
  if ( fulldatalen > datalen ) {
    if ( new_write_ptr == buff )
      buff[ size - 1 ] = '\n';
    else
      new_write_ptr[ -1 ] = '\n';
  }
  
  // Next atomically change the write pointer
  write_ptr = new_write_ptr;
  
  // Determine if the buffer was possibly empty somewhere in the process
  // Since reading happens async to writing, data could have been read out
  // immediately before the write pointer update.
  char *final_read_ptr = read_ptr;
  bool was_empty = false;
  if ( write_ptr < old_write_ptr )
    was_empty = final_read_ptr >= old_write_ptr || final_read_ptr <= write_ptr;
  else
    was_empty = final_read_ptr >= old_write_ptr && final_read_ptr <= write_ptr;
  
  if ( was_empty )
    WroteToEmpty();
  
  // Unlock mutex(es)
  mutex_unlock( &mutex );
  if ( use_write_mutex )
    mutex_unlock( &write_mutex );
  
  return true;
}

bool ThreadBuffer::DataAvailable( void ) {
  if ( min_line_len == 0 )
    return read_ptr != write_ptr;
  
  ptrdiff_t avail = ( write_ptr - read_ptr ) % size;
  
  return avail >= (ptrdiff_t) min_line_len;
}

char *ThreadBuffer::ReadRawData( string *str, char *data, char *read_start, unsigned long length, bool null_terminate ) {
  // Determine if the new data "wraps around" at the end of the buffer memory
  // space.  The variable split contains the number of bytes to write before
  // the end of the memory space.
  size_t split = length;
  
  char *read_end = read_start + length;
  if ( read_end >= buff + size ) {
    read_end -= size;
    split = buff + size - read_start;
  }
  
  // Read the data
  if ( str == NULL ) {
    memcpy( data, read_start, split );
    memcpy( &data[split], buff, length - split );
    if ( null_terminate )
      data[ length ] = '\0';
  } else {
    try {
      str->assign( read_start, split );
      str->append( buff, length - split );
    }
    catch (...) {
      mutex_unlock( &mutex );
      throw;
    }
  }
  
  return read_end;
}

size_t ThreadBuffer::Read( string *str, char *data, size_t max_len, bool wait, char *line_start ) {
  // Lock mutex
  if ( mutex_lock( &mutex ) != 0 )
    return false;
  
  // Determine if data is available
  if ( ! DataAvailable() ) {
    if ( wait ) {
      WaitOnRead();
    } else {
      mutex_unlock( &mutex );
      return 0;
    }
  }
  
  // Atomically grab the write pointer for use in the rest of the function
  // Any data written after this grab will not be read during this call
  char *init_write_ptr = write_ptr;
  
  // Determine the number of bytes to read
  ptrdiff_t bytes_to_read;
  char *new_read_ptr;
  char *read_start = read_ptr;
  
  if ( line_buffered ) {
    // Find first newline, accounting for wrap around
    bytes_to_read = min_line_len;
    char *loc = read_ptr + min_line_len;
    if ( loc >= buff + size )
      loc -= size;
    while ( loc != init_write_ptr ) {
      bytes_to_read++;
      
      if ( *loc == '\n' )
	break;
      
      if ( ++loc >= buff + size )
	loc = buff;
    }
  } else {
    bytes_to_read = init_write_ptr - read_ptr;
    while ( bytes_to_read < 0 )
      bytes_to_read += size;
  }
  
  new_read_ptr = read_ptr + bytes_to_read;
  if ( new_read_ptr >= buff + size )
    new_read_ptr -= size;
  
  // Read min_line_len bytes to line_start, if requested
  if ( min_line_len > 0 && line_start != NULL ) {
    read_start = ReadRawData( NULL, line_start, read_start, min_line_len, false );
    bytes_to_read -= min_line_len;
  }
  
  // Truncate read to max length
  if ( str == NULL && (size_t) bytes_to_read > max_len ) {
    bytes_to_read = max_len;
    if ( ! line_buffered ) {
      new_read_ptr = read_start + bytes_to_read;
      if ( new_read_ptr >= buff + size )
	new_read_ptr -= size;
    }
  }
  
  // Read the data
  ReadRawData( str, data, read_start, bytes_to_read );
  
  // Atomically update the read pointer
  read_ptr = new_read_ptr;
  
  if ( last_write_overflowed && SpaceAvailable() > 0 ) {
    // Turn overflow message back on
    last_write_overflowed = false;
  }
  
  // Unlock mutex
  mutex_unlock( &mutex );
  
  return bytes_to_read;
}

size_t ThreadBuffer::Read( char *data, size_t max_len, bool wait ) {
  return Read( NULL, data, max_len, wait );
}

string ThreadBuffer::Read( bool wait ) {
  string str;
  
  Read( &str, NULL, 0, wait );
  
  return str;
}

void ThreadBuffer::WaitOnRead( void ) {
  while ( read_ptr == write_ptr ) {
    mutex_unlock( &mutex );
    nsleep( &sleep_time );
    mutex_lock( &mutex );
  }
}

void ThreadBuffer::WroteToEmpty( void ) {
}

void ThreadBuffer::Flush( void ) {
  mutex_lock( &mutex );
  
  read_ptr = write_ptr;
  
  mutex_unlock( &mutex );
}

SignalingThreadBuffer::SignalingThreadBuffer( size_t buffer_size, bool is_line_buffered, const ntime_t &nsleep_time, string overflow_indicator, bool use_read_mutex, bool use_write_mutex, unsigned long min_line_length ) :
  ThreadBuffer( buffer_size, is_line_buffered, nsleep_time, overflow_indicator, use_read_mutex, use_write_mutex, min_line_length ) {
  cond_init( &signal_cond );
}

SignalingThreadBuffer::~SignalingThreadBuffer() {
  cond_destroy( &signal_cond );
}

void SignalingThreadBuffer::WaitOnRead( void ) {
  while ( read_ptr == write_ptr ) {
    cond_wait( &signal_cond, &mutex );
  }
}

void SignalingThreadBuffer::WroteToEmpty( void ) {
  cond_broadcast( &signal_cond );
}

ThreadBufferReturnData::ThreadBufferReturnData( size_t buffer_size, const ntime_t &nsleep_time, string overflow_indicator, bool use_read_mutex, bool use_write_mutex ) :
  ThreadBuffer( buffer_size, true, nsleep_time, overflow_indicator, use_read_mutex, use_write_mutex, sizeof( ReturnData * ) ) {
  mutex_init( &return_mutex );
  cond_init( &return_cond );
}

ThreadBufferReturnData::~ThreadBufferReturnData() {
  mutex_destroy( &return_mutex );
  cond_destroy( &return_cond );
}

bool ThreadBufferReturnData::Write( const char *data, bool wait, ssize_t datalen, ReturnData **return_data ) {
  unsigned long num_lines = 0;
  const char *loc, *line_start;
  string extended_data;
  ReturnData *ret_data = NULL;
  
  if ( return_data != NULL ) {
    ret_data = new ReturnData( &return_cond, &return_mutex );
  }
  
  if ( datalen < 0 )
    datalen = strlen( data );
  
  for ( line_start = loc = data; loc - data < datalen; loc++ ) {
    if ( *loc == '\n' ) {
      extended_data.append( ( char * ) &ret_data, min_line_len );
      extended_data.append( line_start, loc - line_start + 1 );
      line_start = loc + 1;
      num_lines++;
    }
  }
  if ( loc == data || loc[ -1 ] != '\n' ) {
    extended_data.append( ( char * ) &ret_data, min_line_len );
    extended_data.append( line_start, loc - line_start );
    extended_data.append( 1, '\n' );
    num_lines++;
  }
  
  if ( ret_data != NULL ) {
    ret_data->SetLineCount( num_lines );
    *return_data = ret_data;
  }
  
  bool ret = ThreadBuffer::Write( extended_data.c_str(), wait, extended_data.length() );
  if ( ! ret && ret_data != NULL ) {
    *return_data = NULL;
  }
  
  return ret;
}

size_t ThreadBufferReturnData::Read( char *data, size_t max_len, bool wait, ReturnData **return_data ) {
  ReturnData *ret_data = NULL;
  
  ssize_t ret = ThreadBuffer::Read( NULL, data, max_len, wait, (char *) &ret_data );
  
  if ( return_data != NULL )
    *return_data = ret_data;

  return ret;
}

string ThreadBufferReturnData::Read( bool wait, ReturnData **return_data ) {
  ReturnData *ret_data;
  string str;
  
  ThreadBuffer::Read( &str, NULL, 0, wait, (char *) &ret_data );
  
  if ( return_data != NULL )
    *return_data = ret_data;
  
  return str;
}

void ThreadBufferReturnData::Flush( void ) {
  // Need to all the ReturnData elements in the buffer by returning blank lines
  mutex_lock( &mutex );
  
  char *init_write_ptr = write_ptr;
  char *read_start = read_ptr;
  ReturnData *ret_data;
  bool at_line_start = true;
  
  if ( read_start > init_write_ptr ) {
    while ( read_start < buff + size ) {
      if ( at_line_start ) {
	ReadRawData( NULL, (char*) &ret_data, read_start, min_line_len, false );
	if ( ret_data != NULL ) 
	  ret_data->AddLine( "\n" );
	read_start += min_line_len;
	at_line_start = false;
      } else {
	at_line_start = *read_start == '\n';
	read_start++;
      }
    }
    read_start -= size;
  }
  
  while ( read_start < init_write_ptr ) {
    if ( at_line_start ) {
      ReadRawData( NULL, (char*) &ret_data, read_start, min_line_len, false );
      if ( ret_data != NULL ) 
	ret_data->AddLine( "\n" );
      read_start += min_line_len;
      at_line_start = false;
    } else {
      at_line_start = *read_start == '\n';
      read_start++;
    }
  }
  
  read_ptr = init_write_ptr;
  
  mutex_unlock( &mutex );
}

bool ThreadBufferReturnData::WaitForReturnData( ReturnData &return_data ) {
  if ( mutex_lock( &return_mutex ) != 0 )
    return false;
  
  while ( return_data.LinesRemaining() > 0 ) {
    cond_wait( &return_cond, &return_mutex );
  }
  
  mutex_unlock( &return_mutex );
  return true;
}

ThreadBufferReturnData::ReturnData::ReturnData( cond_t *return_cond, mutex_t *return_mutex, bool use_write_mutex ) : return_cond( return_cond ), return_mutex( return_mutex ) {
  mutex_init( &write_mutex );
}

ThreadBufferReturnData::ReturnData::~ReturnData() {
  mutex_destroy( &write_mutex );
}

void ThreadBufferReturnData::ReturnData::SetLineCount( unsigned long line_count ) {
  lines_remaining = line_count;
}

unsigned long ThreadBufferReturnData::ReturnData::LinesRemaining( void ) {
  return lines_remaining;
}

void ThreadBufferReturnData::ReturnData::AddLine( const char *line ) {
  mutex_lock( &write_mutex );
  
  try {
    data.append( line );
  }
  catch (...) {
    mutex_unlock( &write_mutex );
    throw;
  }
  
  mutex_lock( return_mutex );
  lines_remaining--;
  if ( lines_remaining == 0 )
    cond_broadcast( return_cond );
  mutex_unlock( return_mutex );
  
  mutex_unlock( &write_mutex );
}

string ThreadBufferReturnData::ReturnData::GetData( void ) {
  return data;
}
