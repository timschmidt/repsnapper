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

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "threaded_printer_serial.h"

const struct timespec ThreadedPrinterSerial::command_buffer_sleep = { 0, 100 * 1000 * 1000 };
const struct timespec ThreadedPrinterSerial::response_buffer_sleep = { 0, 10 * 1000 * 1000 };
const struct timespec ThreadedPrinterSerial::log_buffer_sleep = { 0, 10 * 1000 * 1000 };
const struct timespec ThreadedPrinterSerial::helper_thread_sleep = { 0, 100 * 1000 * 1000 };

ThreadedPrinterSerial::ThreadedPrinterSerial() :
  PrinterSerial( helper_thread_sleep.tv_nsec / 1000 / 1000 ),
  command_buffer( command_buffer_size, command_buffer_sleep, "", false, true ),
  response_buffer( response_buffer_size, true, response_buffer_sleep, "", true, false ),
  log_buffer( log_buffer_size, false, log_buffer_sleep, "\n*** Log overflow ***\n\n", true, false ),
  error_buffer( log_buffer_size, false, log_buffer_sleep, "\n*** Error Log overflow ***\n\n", true, false ) {
  request_print = is_printing = printing_complete = false;
  printer_commands = NULL;
  pc_lines_printed = 0;
  pc_bytes_printed = 0;
  pc_stop_line = 0;
  
  mutex_init( &pc_mutex );
  mutex_init( &pc_cond_mutex );
  cond_init( &pc_cond );
  
  helper_active = false;
  helper_cancel = false;
}

ThreadedPrinterSerial::~ThreadedPrinterSerial() {
  if ( helper_active ) {
    mutex_lock( &pc_cond_mutex );
    helper_cancel = true;
    mutex_unlock( &pc_cond_mutex );
    
    thread_join( helper_thread );
    helper_active = false;
  }
  
  mutex_destroy( &pc_mutex );
  mutex_destroy( &pc_cond_mutex );
  cond_destroy( &pc_cond );
  
  if ( printer_commands != NULL )
    delete [] printer_commands;
}

bool ThreadedPrinterSerial::Connect( string device, int baudrate ) {
  // Open Serial Port
  if ( ! PrinterSerial::Connect( device, baudrate ) )
    return false;
  
  // Clear printer_commands
  if ( printer_commands != NULL ) {
    delete [] printer_commands;
    printer_commands = NULL;
  }
  
  // Clear/Flush buffers
  command_buffer.Flush();
  response_buffer.Flush();
  
  helper_cancel = false;
  
  // Start thread
  int rc;
  if ( ( rc = thread_create( &helper_thread, HelperMainStatic, this ) ) != 0 ) {
    close( device_fd );
    device_fd = -1;
    ostringstream os;
    os << "Could not create serial helper thread: " << strerror( rc ) << endl;
    LogError( os.str().c_str() );
    return false;
  }
  helper_active = true;
  
  // Sleep for 10 ms
  struct timespec nts = { 0, 10 * 1000 * 1000 };
  nanosleep( &nts, NULL );
  
  // Send version request command
  SendAndWaitResponse( "M115" );
  
  return true;
}

void ThreadedPrinterSerial::Disconnect( void ) {
  StopPrinting( true );
  
  if ( helper_active ) {
    mutex_lock( &pc_cond_mutex );
    helper_cancel = true;
    mutex_unlock( &pc_cond_mutex );
    
    thread_join( helper_thread );
    helper_active = false;
  }
  
  command_buffer.Flush();
  
  PrinterSerial::Disconnect();
}

bool ThreadedPrinterSerial::IsConnected( void ) {
  return helper_active && PrinterSerial::IsConnected();
}

bool ThreadedPrinterSerial::StartPrinting( string commands, unsigned long start_line, unsigned long stop_line ) {
  char *commands_copy;
  int rc;
  size_t pos;
  unsigned long count;
  unsigned long lines_printed;
  unsigned long bytes_printed;
  
  for ( pos = -1, count = 1; count < start_line; count++ ) {
    if ( ( pos = commands.find( '\n', pos + 1) ) == string::npos ) {
      ostringstream os;
      os << "Reached end of printer commands at line " << count << " before finding line " << start_line << endl;
      LogError( os.str().c_str() );
      return false;
    }
  }
  
  bytes_printed = pos + 1;
  lines_printed = start_line > 0 ? start_line - 1 : 0;
  
  // Allocate memory for commands
  size_t len;
  len = commands.length();
  commands_copy = new char[ len + 10 ];
  memcpy( commands_copy, commands.c_str(), len + 1 );
  
  // Make sure we are connected to a printer
  if ( ! IsConnected() ) {
    delete [] commands_copy;
    return false;
  }
  
  // Lock pc_mutex
  if ( ( rc = mutex_lock( &pc_mutex ) ) != 0 ) {
    delete [] commands_copy;
    ostringstream os;
    os << "Could not lock mutex to start printing: " << strerror( rc ) << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  // Lock the cond mutex
  if ( ( rc = mutex_lock( &pc_cond_mutex ) ) != 0 ) {
    delete [] commands_copy;
    mutex_unlock( &pc_mutex );
    ostringstream os;
    os << "Could not lock condition mutex to start printing: " << strerror( rc ) << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  // Make sure we are not already printing
  if ( is_printing ) {
    request_print = false;
    
    if ( ( rc = cond_wait( &pc_cond, &pc_cond_mutex ) ) !=0 ) {
      delete [] commands_copy;
      mutex_unlock( &pc_cond_mutex );
      mutex_unlock( &pc_mutex );
      ostringstream os;
      os << "Could not wait on signal to stop previous printing: " << strerror( rc ) << endl;
      LogError( os.str().c_str() );
      return false;
    }
  }
  
  // Ready to start printing, set the variables
  if ( printer_commands != NULL )
    delete [] printer_commands;
  
  printer_commands = commands_copy;
  pc_lines_printed = lines_printed;
  pc_bytes_printed = bytes_printed;
  pc_stop_line = stop_line;
  
  // Request printing
  request_print = true;
  
  if ( ( rc = cond_wait( &pc_cond, &pc_cond_mutex ) ) !=0 ) {
    delete [] commands_copy;
    mutex_unlock( &pc_cond_mutex );
    mutex_unlock( &pc_mutex );
    ostringstream os;
    os << "Could not wait on signal to start printing: " << strerror( rc ) << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  // Unlock mutexes
  mutex_unlock( &pc_cond_mutex );
  mutex_unlock( &pc_mutex );
  
  return true;
}

bool ThreadedPrinterSerial::IsPrinting() {
  return is_printing && ! printing_complete;
}

bool ThreadedPrinterSerial::StopPrinting( bool wait ) {
  int rc;
  
  if ( ! IsConnected() ) {
    return true;
  }
  
  if ( ( rc = mutex_lock( &pc_mutex ) ) != 0 ) {
    ostringstream os;
    os << "Could not lock mutex to stop printing: " << strerror( rc ) << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  if ( ( rc = mutex_lock( &pc_cond_mutex ) ) != 0 ) {
    mutex_unlock( &pc_mutex );
    ostringstream os;
    os << "Could not lock cond mutex to stop printing: " << strerror( rc ) << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  request_print = false;
  
  if ( wait && is_printing ) {
    if ( ( rc = cond_wait( &pc_cond, &pc_cond_mutex ) ) !=0 ) {
      mutex_unlock( &pc_cond_mutex );
      mutex_unlock( &pc_mutex );
      ostringstream os;
      os << "Could not wait on signal to stop printing: " << strerror( rc ) << endl;
      LogError( os.str().c_str() );
      return false;
    }
  }
  
  mutex_unlock( &pc_cond_mutex );
  mutex_unlock( &pc_mutex );
  return true;
}

bool ThreadedPrinterSerial::ContinuePrinting( bool wait ) {  
  int rc;
  
  if ( printer_commands == NULL ) {
    ostringstream os;
    os << "Could not continue, since printing was never started in the first place" << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  if ( ( rc = mutex_lock( &pc_mutex ) ) != 0 ) {
    ostringstream os;
    os << "Could not lock mutex to continue printing: " << strerror( rc ) << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  if ( ! IsConnected() ) {
    mutex_unlock( &pc_mutex );
    return false;
  }
  
  if ( ( rc = mutex_lock( &pc_cond_mutex ) ) != 0 ) {
    mutex_unlock( &pc_mutex );
    ostringstream os;
    os << "Could not lock cond mutex to continue printing: " << strerror( rc ) << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  request_print = true;
  
  if ( wait && ! is_printing ) {
    if ( ( rc = cond_wait( &pc_cond, &pc_cond_mutex ) ) !=0 ) {
      mutex_unlock( &pc_cond_mutex );
      mutex_unlock( &pc_mutex );
      ostringstream os;
      os << "Could not wait on signal to stop printing: " << strerror( rc ) << endl;
      LogError( os.str().c_str() );
      return false;
    }
  }
  
  mutex_unlock( &pc_cond_mutex );
  mutex_unlock( &pc_mutex );
  return true;
}

unsigned long ThreadedPrinterSerial::GetPrintingProgress( unsigned long *bytes_printed ) {
  unsigned long lines;
  unsigned long bytes;
  
  mutex_lock( &pc_cond_mutex );
  
  lines = pc_lines_printed;
  bytes = pc_bytes_printed;
  
  mutex_unlock( &pc_cond_mutex );
  
  if ( bytes_printed != NULL )
    *bytes_printed = bytes;
  
  return lines;
}

bool ThreadedPrinterSerial::Send( string command ) {
  return command_buffer.Write( command.c_str(), true );
}

string ThreadedPrinterSerial::SendAndWaitResponse( string command ) {
  ThreadBufferReturnData::ReturnData *return_data = NULL;
  
  if ( ! command_buffer.Write( command.c_str(), true, -1, &return_data ) )
    return "";
  
  if ( return_data == NULL )
    return "";
  
  if ( ! command_buffer.WaitForReturnData( *return_data ) ) {
    delete return_data;
    return "";
  }
  
  string str = return_data->GetData();
  delete return_data;
  
  return str;
}

string ThreadedPrinterSerial::ReadResponse( bool wait ) {
  return response_buffer.Read( wait );
}

string ThreadedPrinterSerial::ReadLog( bool wait ) {
  return log_buffer.Read( wait );
}

string ThreadedPrinterSerial::ReadErrorLog( bool wait ) {
  return error_buffer.Read( wait );
}

////////////////////////////////////////////////////////////////////////////
//  Helper Thread Funcitons
////////////////////////////////////////////////////////////////////////////

void *ThreadedPrinterSerial::HelperMainStatic( void *arg ) {
  ThreadedPrinterSerial *serial = ( ThreadedPrinterSerial * ) arg;
  
  return serial->HelperMain();
}

void *ThreadedPrinterSerial::HelperMain( void ) {
  ThreadBufferReturnData::ReturnData *return_data;
  
  while ( true ) {
    CheckPrintingState();
    
    if ( command_buffer.Read( command_scratch, max_command_size, false, &return_data ) > 0 ) {
      SendCommand( true, return_data );
    } else if ( IsPrinting() ) {
      SendNextPrinterCommand();
    } else {
      nanosleep( &helper_thread_sleep, NULL );
    }
  }
  
  return NULL;
}

void ThreadedPrinterSerial::CheckPrintingState( void ) {
  mutex_lock( &pc_cond_mutex );
  
  if ( helper_cancel ) {
    helper_cancel = false;
    mutex_unlock( &pc_cond_mutex );
    thread_exit();
  }
  
  if ( request_print != is_printing ) {
    is_printing = request_print;
    printing_complete = false;
    cond_broadcast( &pc_cond );
  }
  
  mutex_unlock( &pc_cond_mutex );
}

void ThreadedPrinterSerial::SendNextPrinterCommand( void ) {
  unsigned long datalen;
  bool truncated = false;
  
  mutex_lock( &pc_cond_mutex );
  
  // Find the bounds of the next command
  const char *start = printer_commands + pc_bytes_printed;
  
  const char *stop;
  for ( stop = start; *stop != '\n' && *stop != '\0'; stop++ )
    ;
  
  datalen = stop - start;
  if ( datalen > max_command_size - 2 ) {
    datalen = max_command_size - 2;
    truncated = true;
  }
  
  // Copy command to scratch buffer.  Always add a newline.
  memcpy( command_scratch, start, datalen );
  char *loc = command_scratch + datalen;
  *loc++ = '\n';
  *loc++ = '\0';
  
  // Update status
  pc_lines_printed++;
  pc_bytes_printed = stop - printer_commands + ( ( *stop == '\n' ) ? 1 : 0 );
  
  // Update printing complete
  if ( *stop == '\0' || pc_lines_printed >= pc_stop_line )
    printing_complete = true;
  
  mutex_unlock( &pc_cond_mutex );
  
  if ( truncated ) {
    char warn[ 100 ];
    snprintf( warn, 99, "*** Warning: Truncated long printer command at line %lu\n", pc_lines_printed );
    if ( warn[ 98 ] != '\0' )
      warn[ 98 ] = '\n';
    warn[ 99 ] = '\0';
    LogLine( warn );
  }
  
  // Send the command and wait for response
  SendCommand( false );
}

void ThreadedPrinterSerial::SendCommand( bool buffer_response, ThreadBufferReturnData::ReturnData *return_data ) {
  // Don't send blank lines
  char *recvd = PrinterSerial::SendCommand();
  
  if ( recvd == NULL ) {
    if ( return_data != NULL )
      return_data->AddLine( "ok Error sending line\n" );
  }
  
  if ( strncasecmp( recvd, "!!", 2 ) == 0 ) {
    // !! Fatal Error
    response_buffer.Write( recvd, true );
    helper_active = false;
    Disconnect(); // This is safe.  With helper active false, no mutexes are needed and no threads are killed.
    thread_exit();
  }
  
  if ( return_data != NULL ) {
    return_data->AddLine( recvd );
  } else if ( buffer_response ) {
    // buffer resposne if it is "interesting", that is if it is more than
    // just the two letter "ok" reply followed by white space.
    char *loc;
    for ( loc = recvd + 2; *loc != ' '; loc++ )
      ;
    
    if ( *loc != '\n' )
      response_buffer.Write( recvd, false );
  }
}

void ThreadedPrinterSerial::RecvTimeout( void ) {
  CheckPrintingState();
}

// Log the line.  The provided line should end in a newline character.
void ThreadedPrinterSerial::LogLine( const char *line ) {
  log_buffer.Write( line, false );
}

// Log error the line.  The provided line should end in a newline character.
void ThreadedPrinterSerial::LogError( const char *error_line ) {
  error_buffer.Write( error_line, false );
}
