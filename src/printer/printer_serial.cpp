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
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#endif

#include "printer_serial.h"

PrinterSerial::PrinterSerial( unsigned long max_recv_block_ms ) :
  max_recv_block_ms( max_recv_block_ms ) {
  
  full_command_scratch = new char[ max_command_size + max_command_prefix + max_command_postfix + 10 ];
  command_scratch = full_command_scratch + max_command_prefix;
  
  full_recv_buffer = new char[ max_command_size + max_command_prefix + 10 ];
  recv_buffer = full_recv_buffer + max_command_prefix;

#ifdef WIN32
  device_handle = INVALID_HANDLE_VALUE;
#else
  device_fd = -1;
#endif
  prev_cmd_line_number = 0;
}

PrinterSerial::~PrinterSerial() {
#ifdef WIN32
  if ( device_handle != INVALID_HANDLE_VALUE ) {
    CloseHandle( device_handle );
    device_handle = INVALID_HANDLE_VALUE;
  }
#else
  if ( device_fd >= 0 ) {
    close( device_fd );
    device_fd = -1;
  }
#endif
  
  delete [] full_command_scratch;
  delete [] full_recv_buffer;
}

bool PrinterSerial::TestPort( const string device ) {
#ifdef WIN32
  HANDLE h;
  h = CreateFile( device.c_str(),
		  GENERIC_READ | GENERIC_WRITE,
		  0,
		  NULL,
		  OPEN_EXISTING,
		  FILE_ATTRIBUTE_NORMAL,
		  NULL );
  if ( h == INVALID_HANDLE_VALUE )
    return false;
  
  CloseHandle( h );
  
  return true;
#else
  int fd;
  
  if ( ( fd = open( device.c_str(), O_RDWR | O_NOCTTY ) ) < 0 )
    return false;
  
  close( fd );
  return true;
#endif
}

vector<string> PrinterSerial::FindPorts() {
  vector<string> ports;
  
#ifdef WIN32
  char name[5];
  strncpy( name, "COMx", 5 );
  for ( int i = 0; i <= 9; i++ ) {
    name[3] = i + '0';
    if ( TestPort( name ) )
      ports.push_back( name );
  }
  
  return ports;
#else
  DIR *dir;
  struct dirent *de;
  
  if ( ( dir = opendir( "/dev" ) ) == NULL )
    return ports;
  
  while ( ( de = readdir( dir ) ) ) {
    if ( strncmp( de->d_name, "ttyUSB", 6 ) == 0 )
      ports.push_back( string( "/dev/" ) + de->d_name );
    else if ( strncmp( de->d_name, "ttyACM", 6 ) == 0 )
      ports.push_back( string( "/dev/" ) + de->d_name );
    else if ( strncmp( de->d_name, "cuaU", 4 ) == 0 )
      ports.push_back( string( "/dev/" ) + de->d_name );
  }
  
  closedir( dir );
  
  return ports;
#endif
}

bool PrinterSerial::RawConnect( string device, int baudrate ) {
#ifdef WIN32
  if ( IsConnected() || device_handle != INVALID_HANDLE_VALUE ) {
    Disconnect();
    if ( device_handle != INVALID_HANDLE_VALUE )
      return false;
  }
  
  device_handle = CreateFile( device.c_str(),
			      GENERIC_READ | GENERIC_WRITE,
			      0,
			      NULL,
			      OPEN_EXISTING,
			      FILE_ATTRIBUTE_NORMAL,
			      NULL );
  if ( device_handle == INVALID_HANDLE_VALUE ) {
    ostringstream os;
    os << "Could not open port " << device << " (" << GetLastError() << ")"<< endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  DCB dcb;
  if ( ! GetCommState( device_handle, &dcb ) ) {
    CloseHandle( device_handle );
    device_handle = INVALID_HANDLE_VALUE;
    ostringstream os;
    os << "Could not get port state " << device << " (" << GetLastError() << ")"<< endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  dcb.BaudRate = baudrate;
  dcb.fBinary = 1;
  dcb.fParity = 0;
  dcb.fOutxCtsFlow = 0;
  dcb.fOutxDsrFlow = 0;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fDsrSensitivity = 0;
  dcb.fOutX = 0;
  dcb.fInX = 0;
  dcb.fNull = 0;
  dcb.fRtsControl = RTS_CONTROL_ENABLE;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  dcb.EvtChar = '\n';
  
  if ( ! SetCommState( device_handle, &dcb ) ) {
    CloseHandle( device_handle );
    device_handle = INVALID_HANDLE_VALUE;
    ostringstream os;
    os << "Could not configure port " << device << " (" << GetLastError() << ")"<< endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  COMMTIMEOUTS ct;
  
  if ( ! GetCommTimeouts( device_handle, &ct ) ) {
    CloseHandle( device_handle );
    device_handle = INVALID_HANDLE_VALUE;
    ostringstream os;
    os << "Could not get port timeouts " << device << " (" << GetLastError() << ")"<< endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  ct.ReadIntervalTimeout = 2;
  ct.ReadTotalTimeoutConstant = max_recv_block_ms;
  ct.ReadTotalTimeoutMultiplier = 0;
  ct.WriteTotalTimeoutConstant = max_recv_block_ms;
  ct.WriteTotalTimeoutMultiplier= 0;
  
  if ( ! SetCommTimeouts( device_handle, &ct ) ) {
    CloseHandle( device_handle );
    device_handle = INVALID_HANDLE_VALUE;
    ostringstream os;
    os << "Could not set port timeouts " << device << " (" << GetLastError() << ")"<< endl;
    LogError( os.str().c_str() );
    return false;
  }
  
#else
  // Verify speed is valid and convert to posix value
  speed_t speed = B0;
  switch ( baudrate ) {
  case 50: speed = B50; break;
  case 75: speed = B75; break;
  case 110: speed = B110; break;
  case 134: speed = B134; break;
  case 150: speed = B150; break;
  case 200: speed = B200; break;
  case 300: speed = B300; break;
  case 600: speed = B600; break;
  case 1200: speed = B1200; break;
  case 1800: speed = B1800; break;
  case 2400: speed = B2400; break;
  case 4800: speed = B4800; break;
  case 9600: speed = B9600; break;
  case 19200: speed = B19200; break;
  case 38400: speed = B38400; break;
  case 57600: speed = B57600; break;
  case 115200: speed = B115200; break;
  case 230400: speed = B230400; break;
  default:
    ostringstream os;
    os << "Unknown baudrate " << baudrate << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  if ( IsConnected() || device_fd >= 0 ) {
    Disconnect();
    if ( device_fd >= 0 )
      return false;
  }
  
  // Open file
  if ( ( device_fd = open( device.c_str(), O_RDWR | O_NOCTTY ) ) < 0 ) {
    ostringstream os;
    os << "Could not open serial device: \"" << device << "\"" << strerror( errno ) << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  // Configure port
  struct termios attribs;
  if ( tcgetattr( device_fd, &attribs ) < 0 ) {
    int err = errno;
    close( device_fd );
    device_fd = -1;
    ostringstream os;
    os << "Could not get terminal attributes of serial port " << device << ": " << strerror( err ) << endl;
    LogError( os.str().c_str() );
    return false;
  }
  
  // Use RAW settings, except add ICANON.  ICANON makes the port line buffered
  // and read will return immediately when a newline is received.
  // HUPCL lowers the modem control lines (hang up) when the port is closed
  cfmakeraw( &attribs );
  attribs.c_lflag |= ICANON;
  attribs.c_cflag |= HUPCL;
  
  if( tcsetattr(device_fd, TCSANOW, &attribs ) < 0 ) {
    close( device_fd );
    device_fd = -1;
    LogError( "Error enabling raw mode\n" );
    return false;
  }
  
  // Set port speed
  if ( cfsetispeed( &attribs, speed ) < 0 ||
       cfsetospeed( &attribs, speed ) < 0 ) {
    close( device_fd );
    device_fd = -1;
    ostringstream os;
    os << "Error setting port speed to " << baudrate << endl;
    LogError( os.str().c_str() );
    return false;
  }

  if( tcsetattr(device_fd, TCSANOW, &attribs ) < 0) {
    close( device_fd );
    device_fd = -1;
    ostringstream os;
    os << "Error setting port baud rate to " << baudrate << endl;
    LogError( os.str().c_str() );
    return false;
  }
#endif
  
  // Reset line number
  prev_cmd_line_number = 0;
  
  return true;
}

bool PrinterSerial::Connect( string device, int baudrate ) {
  if ( ! RawConnect( device, baudrate ) )
    return false;
  
  // Read start line before returning
  // The printer seems to lock up if it recvs a command before the start
  // line has been sent
  RecvLine();
  
  return true;
}

void PrinterSerial::Disconnect( void ) {
#ifdef WIN32
  if ( device_handle != INVALID_HANDLE_VALUE ) {
    CloseHandle( device_handle );
    device_handle = INVALID_HANDLE_VALUE;
  }
#else
  if ( device_fd >= 0 ) {
    close( device_fd );
    device_fd = -1;
  }
#endif
}

bool PrinterSerial::IsConnected( void ) {
#ifdef WIN32
  return device_handle != INVALID_HANDLE_VALUE;
#else
  return device_fd >= 0;
#endif
}

bool PrinterSerial::RawReset( void ) {
#ifdef WIN32
  if ( device_handle == INVALID_HANDLE_VALUE )
    return false;
  
  if ( ! EscapeCommFunction( device_handle, CLRDTR ) )
    return false;
  
  if ( ! EscapeCommFunction( device_handle, SETDTR ) )
    return false;
#else
  if ( device_fd < 0 )
    return false;
  
  // First try usb reset
  if ( ioctl( device_fd, USBDEVFS_RESET, 0 ) != 0 ) {
    
    // Next try clear/set DTR
    if ( ioctl( device_fd, TIOCMBIC, TIOCM_DTR ) ||
	 ioctl( device_fd, TIOCMBIS, TIOCM_DTR ) ) {
      
      // Finally try setting baud rate to zero
      struct termios attribs;
      
      if ( tcgetattr( device_fd, &attribs ) != 0 )
	return false;
      
      speed_t orig_speed = cfgetispeed( &attribs );
      cfsetispeed( &attribs, B0 );
      cfsetospeed( &attribs, B0 );
      
      if ( tcsetattr( device_fd, TCSANOW, &attribs ) != 0 )
	return false;
      
      cfsetispeed( &attribs, orig_speed );
      cfsetospeed( &attribs, orig_speed );
      
      if ( tcsetattr( device_fd, TCSANOW, &attribs ) != 0 )
	return false;      
    }
  }
#endif
  
  // Reset line number
  prev_cmd_line_number = 0;  
  
  return true;
}

bool PrinterSerial::Reset( void ) {
  if ( ! Reset() )
    return false;

  // Read start line before returning
  // The printer seems to lock up if it recvs a command before the start
  // line has been sent
  RecvLine();
  
  return true;
}

char *PrinterSerial::Send( const char *command ) {
  strncpy( command_scratch, command, max_command_size );
  return SendCommand();
}

// Sends gcode command.  Performs formating and waits for reply.  The line starts at command_scratch + max_command_prefix.  If buffer_response, the reply is entered into the response_buffer.
char *PrinterSerial::SendCommand( void ) {
  char *formated;
  char *recvd;
  bool send_text = true;
  
  if ( ( formated = FormatLine() ) == NULL ) {
    // Printer can't handle blank lines
    // Don't send them, just return an "ok" response
    // They won't show up in the log, since no data was actually sent
    char *loc = recv_buffer;
    *loc++ = 'o';
    *loc++ = 'k';
    *loc++ = '\n';
    *loc++ = '\0';
    return recv_buffer;
  }
  
  while ( true ) {
    if ( send_text ) {
      if ( ! SendText( formated ) )
	return NULL;
    }
    
    if ( ( recvd = RecvLine() ) == NULL )
      return NULL;
    
    if ( strncasecmp( recvd, "ok", 2 ) == 0 || strncasecmp( recvd, "!!", 2 ) == 0 ) {
      return recvd;
    }
    
    if ( strncasecmp( recvd, "rs", 2 ) == 0 || strncasecmp( recvd, "resend:", 7 ) == 0 ) {
      // Checksum error, resend the line
      send_text = true;
    } else {
      send_text = false;
    }
  }
}

// Formats line of gcode in command_scratch and returns a pointer to the starting character
char *PrinterSerial::FormatLine( void ) {
  char *start = command_scratch;
  
  // Add prefix
  start--;
  *start = ' ';
  
  unsigned long this_line = prev_cmd_line_number + 1;
  
  if ( this_line == 0 ) {
    start--;
    *start = '0';
  } else {
    unsigned long count;
    for ( count = this_line; count > 0; count /= 10 ) {
      start--;
      *start = ( count % 10 ) + '0';
    }
  }
  start--;
  *start = 'N';
  
  // Calculate checksum
  unsigned char cksum = 0;
  char *loc;
  for ( loc = start; *loc != '\n' && *loc != ';' && *loc != '\0' && *loc != '*'; loc++ ) {
    cksum ^= *loc;
  }
  while ( loc[-1] == ' ' || loc[-1] == '\t' ) {
    loc--;
    cksum ^= *loc;
  }
  
  if ( loc <= command_scratch ) {
    // Line was all whitespace and/or comment, nothing to send
    return NULL;
  }
  
  // Write checksum
  *loc++ = '*';
  if ( cksum >= 100 ) {
    *loc++ = cksum / 100 + '0';
  }
  if ( cksum >= 10 ) {
    *loc++ = ( cksum / 10 ) % 10 + '0';
  }
  *loc++ = cksum % 10 + '0';
  
  // Terminate line
  *loc++ = '\n';
  *loc++ = '\0';
  
  prev_cmd_line_number++;
  
  return start;
}

// Sends indicated text exactly.  Does not wait for reply.  Performs logging.
// Text must point mutable memory with 4 bytes available before it
bool PrinterSerial::SendText( char *text ) {
  memcpy( text - 4, "<-- ", 4 );
  LogLine( text - 4 );

  size_t len = strlen( text );
  
#ifdef WIN32
  DWORD num;
  while ( len > 0 ) {
    if ( ! WriteFile( device_handle, &text, len, &num, NULL ) ) {
      LogLine( "*** Error Writing to port ***\n" );
      LogError( "*** Error Writing to port ***\n" );
      return false;
    } len -= num;
    text += num;
  }
#else
  ssize_t num;
  while ( len > 0 ) {
    if ( ( num = write( device_fd, text, len ) ) == -1 ) {
      int err = errno;
      char msg[ 256 ];
      LogLine( "*** Error Writing to port ***\n" );
      snprintf( msg, 250, "*** Error Writing to port: %s ***\n", strerror( err ) );
      if ( msg[ 248 ] != '\0' )
	msg[ 248 ] = '\n';
      msg[ 249 ] = '\0';
      
      LogError( msg );
      return false;
    } len -= num;
    text += num;
  }
#endif

  return true;
}

// Waits for a complete line from the port and receives that line into recv_buffer (but not at the start of recv_buffer to make logging easier).  Returns pointer to start of recv'd data.  Performs logging.
char *PrinterSerial::RecvLine( void ) {
  char *recvd = recv_buffer;
  char *buf = recvd;
  bool done = false;
  size_t tot_size = 0;
  
#ifdef WIN32
  DWORD num;
  
  while ( ! done ) {
    if ( tot_size + 20 >= max_command_size ) {
      LogLine( "*** Error: Received line too long ***\n" );
      LogError( "*** Error: Received line too long ***\n" );
      *buf++ = '\n';
      break;
    }
    if ( ! ReadFile( device_handle, buf, max_command_size - tot_size - 20, &num, NULL ) ) {
      LogLine( "*** Error Reading from port ***\n" );
      LogError( "*** Error Reading from port ***\n" );
      return NULL;
    }
    tot_size += num;
    buf += num;
    
    if ( num > 0 && ( buf[-1] == '\n' || buf[-1] == '\r' ) ) {
      done = true;
    } else {
      RecvTimeout();
    }
  }
  *buf = '\0';
#else
  ssize_t num;
  struct timeval timeout;
  fd_set set;
  
  // Read the data
  while ( ! done ) {
    // Make sure line is not too long
    if ( tot_size + 20 >= max_command_size ) {
      LogLine( "*** Error: Received line too long ***\n" );
      LogError( "*** Error: Received line too long ***\n" );
      *buf++ = '\n';
      break;
    }
    // Use select to allow a read timeout.
    // If the timeout is reached, call RecvTimeout
    timeout.tv_sec = 0;
    timeout.tv_usec = max_recv_block_ms * 1000;
    FD_ZERO( &set );
    FD_SET( device_fd, &set );
    select( device_fd + 1,
	    &set,
	    NULL,
	    NULL,
	    max_recv_block_ms == 0 ? NULL : &timeout );
    
    if ( FD_ISSET( device_fd, &set ) ) {
      // Read the data.  Use a loop since Posix does not guarentee that an
      // entire line will be read at once.
      //cout << "Reading" << endl;
      if ( ( num = read( device_fd, buf, max_command_size - tot_size - 20 ) ) == -1 ) {
	int err = errno;
	char msg[ 256 ];
	LogLine( "*** Error reading from port ***\n" );
	snprintf( msg, 250, "*** Error reading from port: %s ***\n", strerror( err ) );
	if ( msg[ 248 ] != '\0' )
	  msg[ 248 ] = '\n';
	msg[ 249 ] = '\0';
	
	LogError( msg );
	return NULL;
      }
      tot_size += num;
      buf += num;
      
      if ( num > 0 && ( buf[-1] == '\n' || buf[-1] == '\r' ) ) {
	done = true;
      }
    } else {
      RecvTimeout();
    }
  }
  *buf = '\0';
#endif
  
  // Ignore inital whitespace
  while ( isspace( *recvd ) ) {
    recvd++;
  }
  
  // Log the line
  memcpy( recvd - 4, "--> ", 4 );
  LogLine( recvd - 4 );
  
  return recvd;
}

void PrinterSerial::RecvTimeout( void ) {
}

void PrinterSerial::LogLine( const char *line ) {
  cout << line;
}

void PrinterSerial::LogError( const char *error_line ) {
  cerr << error_line;
}
