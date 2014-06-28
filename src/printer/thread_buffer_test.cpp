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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "thread_buffer.h"

#include <iostream>
#include <stdio.h>

//#define USE_RET_DATA

const ntime_t ns = { 10, 100 * 1000 * 1000 };
#ifndef USE_RET_DATA
//ThreadBuffer tb( 50, 1, ns, "*overflow*", false, false );
//ThreadBuffer tb( 20, false, ns, "\n*** Log overflow ***\n\n", true, false );
SignalingThreadBuffer tb( 50, 1, ns, "*overflow*", false, false );
#else
ThreadBufferReturnData tb( 50, ns, "*overflow*", true, true );
#endif

void *Reader( void * ) {
  string str;
#ifdef USE_RET_DATA
  char ret[1024];
  int count = 0;
  ThreadBufferReturnData::ReturnData *ret_data;
#endif
  
  cout << "Type commands seperated by newlines" << endl;
  cout << "Reponses start 10 seconds after first entry" << endl;
  cout << "with 3 seconds between sucessive reads" << endl;
#ifdef USE_RET_DATA
  cout << "Use a semicolon to seperate reply bundles" << endl;
#endif
  
  while ( 1 ) {
#ifndef USE_RET_DATA
    str = tb.Read( true );
#else
    str = tb.Read( true, &ret_data );
#endif
    cout << "Read: \"" << str << "\"" << endl;
#ifdef USE_RET_DATA
    if ( ret_data == NULL ) {
      cout << "Read has null ret_data" << endl;
    } else {
      snprintf( ret, 1000, "*ret: %d*\n", count++ );
      ret_data->AddLine( ret );
    }
#endif
    ntime_t nt = { 1, 0 };
    nsleep( &nt );
  }
  
  return NULL;
}

int main( void ) {
  char line[ 1024 + 10 ];
  thread_t thread;
#ifdef USE_RET_DATA
  ThreadBufferReturnData::ReturnData *ret_data;
#endif  

  thread_create( &thread, Reader, NULL );
  
  while ( 1 ) {
#ifndef USE_RET_DATA
    cin.getline( line, 1024 );
    tb.Write( line, false );
#else
    cin.getline( line, 1024, ';' );
    tb.Write( line, false, -1, &ret_data );
    if ( ret_data == NULL ) {
      cout << "Write has null ret_data" << endl;
    } else {
      tb.WaitForReturnData( *ret_data );
      cout << "Return: " << ret_data->GetData() << endl;
    }
#endif
  }
  
  return 0;
}
