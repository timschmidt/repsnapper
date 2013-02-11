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

#include "printer_serial.h"

#include <string.h>

using namespace std;

int main( int argc, char *argv[] ) {
  PrinterSerial ps( 100 );
  char command[ 1024 ];
  
  vector<string> ports = PrinterSerial::FindPorts();
  for ( unsigned int ind = 0; ind < ports.size(); ind++ ) {
    cout << "Found port: " << ports[ind] << endl;
    cout << "Testing: " << PrinterSerial::TestPort( ports[ind] ) << endl;
  }
  
  if ( argc >= 2 )
    ps.Connect( argv[1], 115200 );
  else
    ps.Connect( ports[0], 115200 );
  
  cout << "Connected" << endl;
  
  /*  
#ifdef WIN32
  Sleep( 1000 );
#else
  sleep( 1 );
#endif
  */

  cout << ps.Send( "M115" );
  
  while ( 1 ) {
    cin.getline( command, 1024 );
    if ( strncasecmp( command, "reset", 1024 ) == 0 ) {
      if ( ps.Reset() )
	cout << "Success" << endl;
      else
	cout << "Failure" << endl;
      continue;
    } else if ( strncasecmp( command, "quit", 1024 ) == 0 )
      break;
    cout << ps.Send( command );
  }
  
  ps.Disconnect();
}
