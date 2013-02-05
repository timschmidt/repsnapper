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

#include <limits.h>

#include "thread.h"
#include "thread_buffer.h"
#include "printer_serial.h"

using namespace std;

class ThreadedPrinterSerial : protected PrinterSerial
{
  static const unsigned long command_buffer_size = 8192;
  static const unsigned long response_buffer_size = 4096;
  static const unsigned long log_buffer_size = 8192;
  
  static const ntime_t command_buffer_sleep;
  static const ntime_t response_buffer_sleep;
  static const ntime_t log_buffer_sleep;
  static const ntime_t helper_thread_sleep;
  
  // Rules:
  // request_print, is_printing, and printer_commands are initialized to NULL
  // To stop printing, thread must lock the mutex, set request_print to false
  //   and wait for the helper to signal on pc_cond.  Finally, release the
  //   mutex.
  // To start printing, lock the mutex, if is_printing is true, stop printing
  //   per the above steps.  Next, set printer commands to the desired commands
  //   and clear ps_status.  Then, set request_print to true and wait for
  //   the helper to signal on pc_cond.  Finally, release the mutex.
  // For the purpose of status bars and status lights,
  //   the values of is_printing and pc_status can be examined without needing
  //   to lock the mutex.
  // 
  // Helper:
  //   if request_print is different than is_printing, aquire the mutex,
  //     set is_printing to match request_print, signal on pc_cond, and relase
  //     the mutex.
  //   <<handle queued commands>
  //   if is_printing, send the next command from printer_commands.  Do NOT
  //     need to lock the mutex.
  
  mutex_t pc_mutex;
  bool request_print; // set by main thread(s), pc_mutex required
  bool is_printing; // set by helper, pc_cond_mutex required
  bool printing_complete; // set by helper, no mutex required
  cond_t pc_cond; // signaled by helper, pc_mutex and pc_cond_mutex required
  mutex_t pc_cond_mutex;
  const char *printer_commands; // set by main thread(s), pc_mutex required
  unsigned long pc_lines_printed; // when is_printing is false, set by main thread(s), pc_mutex required.  When is_printing is true, set by helper, pc_mutex requried
  unsigned long pc_bytes_printed; // when is_printing is false, set by main thread(s), pc_mutex required.  When is_printing is true, set by helper, pc_mutex required
  unsigned long pc_stop_line; // set by main thread(s), pc_mutex required
  
  ThreadBufferReturnData command_buffer;
  SignalingThreadBuffer response_buffer;
  ThreadBuffer log_buffer;
  ThreadBuffer error_buffer;
  
  bool helper_active;
  thread_t helper_thread;
  bool helper_cancel;
  
  void CheckPrintingState( void ); // Check if main thread is requesting printing and set helper thread switches accordingly
  
  void SendNextPrinterCommand( void );
  void SendCommand( bool buffer_response, ThreadBufferReturnData::ReturnData *return_data = NULL );
  
  void RecvTimeout( void );
  void LogLine( const char *line ); // Log the line.  The provided line should end in a newline character.
  void LogError( const char *error_line ); // Log the error.  The provided line should end in a newline character.
  
  static void *HelperMainStatic( void *arg );
  void *HelperMain( void );
  
 public:
  ThreadedPrinterSerial();
  virtual ~ThreadedPrinterSerial();
  
  // Connect to or disconnect from a printer
  virtual bool Connect( string device, int baudrate );
  virtual void Disconnect( void );
  bool IsConnected( void );
  
  virtual bool Reset( void );
  
  // Start printing gcode
  // Commands are sent one at a time in the background
  // Send and SendAndWaitResponse can safely be sent
  // while printing.
  virtual bool StartPrinting( string commands, unsigned long start_line = 1, unsigned long stop_line = ULONG_MAX );
  virtual bool IsPrinting( void );
  virtual bool StopPrinting( bool wait = true );
  virtual bool ContinuePrinting( bool wait = true );
  
  unsigned long GetPrintingProgress( unsigned long *bytes_printed = NULL );
  // Returns last line number ok'd by the printer
  // If printing is stopped, returns last line number of previous print
  // If bytes_printed is non-null, sets that value as well.
  
  //bool StartPeriodic( string commands, unsigned long interval_ms );
  //void StopPeriodic( void );
  
  bool Send( string command );
  // Command may be multiple commands separated by newlines (\n).
  // Such commands are queued atomically.
  // Commands may be sent when printing is active.
  // Commands sent with this interface have higher priority than commands
  // sent from StartPrinting.
  
  string ReadResponse( bool wait = false );
  // returns "" if wait is false and no response is ready
  // only returns responses from Send() and StartPeriodic() and NOT from
  // SendAndWaitResponse() or StartingPrinting()
  
  string SendAndWaitResponse( string command );
  // Send the string and wait for the response
  // May take up to several seconds if the printer is already processing
  // a long command
  
  string ReadLog( bool wait = false );
  // returns "" if wait is false and no log entries are ready

  string ReadErrorLog( bool wait = false );
  // returns "" if wait is false and no log entries are ready
};
