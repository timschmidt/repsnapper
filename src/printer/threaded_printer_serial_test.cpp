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

#include "threaded_printer_serial.h"

#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <stdlib.h>

using namespace std;

void *LogReader( void *arg ) {
  ThreadedPrinterSerial *tps = (ThreadedPrinterSerial *) arg;
  string str;
  
  while ( 1 ) {
    str = tps->ReadLog( true );
    if ( str.length() > 0 ) {
      //if ( ! tps->IsPrinting() )
      cerr << str;
      // Use cerr instead of cerr.  Wine seams to have a concurancy issue with
      // This thread and the main thread both using cout.
    }
  }
  
  return NULL;
}

void *ErrorReader( void *arg ) {
  ThreadedPrinterSerial *tps = (ThreadedPrinterSerial *) arg;
  string str;
  
  while ( 1 ) {
    str = tps->ReadErrorLog( true );
    if ( str.length() > 0 ) {
      cerr << str;
      // Use cerr instead of cerr.  Wine seams to have a concurancy issue with
      // This thread and the main thread both using cout.
    }
  }
  
  return NULL;
}

int main( int argc, char *argv[] ) {
  ThreadedPrinterSerial tps;
  char command[ 1024 + 10 ];
  thread_t log_reader;
  thread_t error_reader;
  bool is_printing;
  
  vector<string> ports = PrinterSerial::FindPorts();
  for ( unsigned int ind = 0; ind < ports.size(); ind++ ) {
    cout << "Found port: " << ports[ind] << endl;
    cout << "Testing: " << PrinterSerial::TestPort( ports[ind] ) << endl;
  }
  
  thread_create( &log_reader, LogReader, &tps );
  thread_create( &error_reader, ErrorReader, &tps );
  
  if ( argc >= 3 )
    tps.Connect( argv[1], strtol( argv[2], NULL, 10 ) );
  else if ( argc >= 2 )
    tps.Connect( argv[1], 115200 );
  else
    tps.Connect( ports[0], 115200 );
  
  while ( 1 ) {
    cin.getline( command, 1024 );
    if ( strncasecmp( command, "send ", 5 ) == 0 ) {
      string resp( "Resp: " );
      resp.append( tps.SendAndWaitResponse( command + 5 ) );
      resp.append( 1, '\n' );
      cout << resp;
    } else if ( strncasecmp( command, "file ", 5 ) == 0 ) {
      ifstream file( command + 5, ifstream::in );
      string str( "" );
      char block[ 1024 + 10 ];
      
      while ( file.good() ) {
	file.read( block, 1024 );
        str.append( block, file.gcount() );
      }
      
      file.close();
      tps.StartPrinting( str );
    } else if ( strncasecmp( command, "stop", 4 ) == 0 ) {
      tps.StopPrinting( true );
    } else if ( strncasecmp( command, "cont", 4 ) == 0 ) {
      tps.ContinuePrinting( true );
    } else if ( strncasecmp( command, "reset", 5 ) == 0 ) {
      tps.Reset();
    } else if ( strncasecmp( command, "quit", 1024 ) == 0 )
      break;
    
    is_printing = tps.IsPrinting();
    cout << "Is printing: " << is_printing << endl;
    if ( is_printing ) {
      cout << "At line: " << tps.GetPrintingProgress() << endl;
    }
  }
  
  tps.Disconnect();

  cout << "Disconnected" << endl;
  
#ifdef HAS_THREAD_CANCEL
#ifndef WIN32
  thread_cancel( log_reader );
  thread_cancel( error_reader );
  thread_join( log_reader );
  thread_join( error_reader );
#endif
#endif
  
  exit( 0 );
}
