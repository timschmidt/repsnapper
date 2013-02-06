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

#include <iostream>
#include <vector>

using namespace std;

class PrinterSerial {
protected:
  static const unsigned long max_command_size = 1024;
  static const unsigned long max_command_prefix = 32;
  static const unsigned long max_command_postfix = 32;
  
  const unsigned long max_recv_block_ms;
  
#ifdef WIN32
  HANDLE device_handle;
#else
  int device_fd;
#endif
  
  unsigned long prev_cmd_line_number;
  
  char *full_command_scratch;
  char *command_scratch;
  char *full_recv_buffer;
  char *recv_buffer;
  
  char *SendCommand( void ); // Sends gcode command.  Performs formating and waits for reply.  The line starts at command_scratch + max_command_prefix.  If buffer_response, the reply is entered into the response_buffer.
  
  char *FormatLine( void ); // Formats line of gcode in command_scratch and returns a pointer to the starting character
  bool SendText( char *text ); // Sends indicated text exactly.  Does not wait for reply.  Performs logging.
  char *RecvLine( void ); // Waits for a complete line from the port and receives that line into recv_buffer (but not at the start of recv_buffer to make logging easier).  Returns pointer to start of recv'd data.  Performs logging.  
  
  virtual void RecvTimeout( void );
  virtual void LogLine( const char *line );
  virtual void LogError( const char *error_line );
  
  // Gcodes return:
  //   ok
  //   ok T:xx.x Bxx.x
  //   ok C: X:xx.x Y:xx.x Z:xx.x E:xx.x
  //   rs line_num <- resend this line number
  //   !! <- fatal error, printer is shutting down, disconnect
  //   // comments
  
public:
  PrinterSerial( unsigned long max_recv_block_ms );
  virtual ~PrinterSerial();
  
  static bool TestPort( const string device );
  static vector<string> FindPorts( void );
  
  virtual bool Connect( string device, int baudrate );
  virtual void Disconnect( void );
  virtual bool IsConnected( void );
  
  virtual bool Reset( void );
  
  virtual char *Send( const char *command );
};
