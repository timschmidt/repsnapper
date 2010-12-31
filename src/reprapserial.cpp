/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "stdafx.h"
#include "reprapserial.h"
#include "convert.h"
#include "progress.h"
#include "modelviewcontroller.h"


RepRapSerial::RepRapSerial(Progress *progress, ProcessController *ctrl)
{
	m_bConnecting = false;
	m_bConnected = false;
	com = new RepRapBufferedAsyncSerial(this);
	m_bPrinting = false;
	startTime = 0;
	lastUpdateTime = 0;
	m_iLineNr = 0;
	gui = 0;
	debugMask = DEBUG_ECHO | DEBUG_INFO | DEBUG_ERRORS;
	logFile = 0;
	m_ctrl = ctrl;
	m_progress = progress;
	m_signal_printing_changed.emit();
}

void RepRapSerial::debugPrint(string s, bool selectLine)
{
	uint a=0;
	while(a<s.length())
	{
		if(s[a] == '\r' || s[a] == '\n') s[a] = ' ';
		a++;
	}
	time_t rawtime;
	tm * ptm;
	time ( &rawtime );
	ptm = localtime ( &rawtime );
	std::stringstream oss;
	oss << setfill('0') << setw(2) << ptm->tm_hour << ":" << setfill('0') << setw(2) << ptm->tm_min << ":" << setfill('0') << setw(2) << ptm->tm_sec << " Send >>" << s.c_str() << "\n";
	s=oss.str();

	if(gui)
	{
		ToolkitLock guard;

/*			Fl_Text_Buffer* buffer = gui->CommunationLog->buffer();
			buffer->append(s.c_str());
			cout << s;
			Fl::check();*/

		gui->CommunationLog->add(s.c_str());
		if(gui->AutoscrollButton->value())
			gui->CommunationLog->bottomline(gui->CommunationLog->size());
		if(selectLine)
		{
			gui->CommunationLog->select(gui->CommunationLog->size());
			gui->ErrorLog->add(s.c_str());
			if(gui->AutoscrollButton->value())
				gui->ErrorLog->bottomline(gui->ErrorLog->size());
		}

		while(gui->CommunationLog->size() > m_ctrl->KeepLines)
			gui->CommunationLog->remove(1);
		while(gui->ErrorLog->size() > m_ctrl->KeepLines)
			gui->ErrorLog->remove(1);
		while(gui->Echo->size() > m_ctrl->KeepLines)
			gui->Echo->remove(1);
	}
	else
		printf("%s", s.c_str());

	if(m_ctrl->FileLogginEnabled)
	{
	// is the files open?
	if(logFile == 0)
		{
		// Append or new?
		if(m_ctrl->ClearLogfilesWhenPrintStarts)
			logFile = fopen("./RepSnapper.log", "w");
		else
			{
			logFile = fopen("./RepSnapper.log", "a");
			fseek ( logFile , 0 , SEEK_END );	// goto end of file
			}
		}
	fputs ( s.c_str(), logFile );
	}

};
void RepRapSerial::echo(string s)
{

	uint a=0;
	while(a<s.length())
	{
		if(s[a] == '\r' || s[a] == '\n') s[a] = ' ';
		a++;
	}
	time_t rawtime;
	tm * ptm;
	time ( &rawtime );
	ptm = localtime ( &rawtime );
	std::stringstream oss;
	oss << setfill('0') << setw(2) << ptm->tm_hour << ":" << setfill('0') << setw(2) << ptm->tm_min << ":" << setfill('0') << setw(2) << ptm->tm_sec << " Echo >>" << s.c_str() << "\n";
	s=oss.str();

	if(gui)
	{
		ToolkitLock guard;

		gui->Echo->add(s.c_str());
		if(gui->AutoscrollButton->value())
		{
			gui->Echo->bottomline(gui->Echo->size());
			gui->Echo->redraw();
		}
	}
	else
		printf("%s", s.c_str());

	if(m_ctrl->FileLogginEnabled)
	{
	// is the files open?
	if(logFile == 0)
		{
		// Append or new?
		if(m_ctrl->ClearLogfilesWhenPrintStarts)
			logFile = fopen("./RepSnapper.log", "w");
		else
			{
			logFile = fopen("./RepSnapper.log", "a");
			fseek ( logFile , 0 , SEEK_END );	// goto end of file
			}
		}
	fputs ( s.c_str(), logFile );
	}

};

void RepRapSerial::StartPrint()
{
  m_iLineNr = 0;
  m_bPrinting = true;
  m_signal_printing_changed.emit();

  for (int i = 0; i < ReceivingBufferSize; i++)
    SendNextLine();
}

void RepRapSerial::SendNextLine()
{
	if ( com->errorStatus() ) 
	{
		m_bConnecting = false;
		m_bConnected = false;
		m_bPrinting = false;
		{
			ToolkitLock guard;
			gui->MVC->serialConnectionLost();
		}
		return;
	}
	if (m_bPrinting == false)
		return;
	if (m_iLineNr < buffer.size())
	{
		string a = buffer[m_iLineNr];
		SendData(a.c_str(), m_iLineNr++);
	}
	else	// we are done
	{
		m_bPrinting = false;
		buffer.clear();
		GDK_THREADS_ENTER();
		m_progress->stop("Print done");
		GDK_THREADS_LEAVE();
		Clear();
		m_signal_printing_changed.emit();
		return;
	}
	if (gui) {
		unsigned long time = Platform::getTickCount();
		if (startTime == 0)
			startTime = time;
		// it is just wasteful to update the GUI > once per sec.
		if (time - lastUpdateTime > 1000) {
			GDK_THREADS_ENTER();

			double elapsed = (time - startTime) / 1000.0;
			double max = m_progress->maximum();
			double lines_per_sec = elapsed > 1 ? (double)m_iLineNr / elapsed : 1.0;
			double remaining = (double)(max - m_iLineNr) / lines_per_sec;

			// elide small changes ...
			if (fabs (((double)m_iLineNr - m_progress->value()) / max) > 1.0/1000.0)
			  m_progress->update ((double)m_iLineNr);

			int remaining_seconds = (int)fmod (remaining, 60.0);
			int remaining_minutes = ((int)fmod (remaining, 3600.0) - remaining_seconds) / 60;
			int remaining_hours = (int)remaining / 3600;

			std::stringstream oss;

			/*
			 * Trade accuracy for reduced UI update frequency.
			 */
			if (remaining_hours > 0)
				oss << setw(2) << remaining_hours << "h" << remaining_minutes << "m";
			else if (remaining_minutes > 5)
				oss << setw(2) << remaining_minutes << "m";
			else if (remaining_seconds > 0)
				oss << setw(2) << remaining_minutes << "m" << remaining_seconds << "s";
			else
				oss << "Progress";

			m_progress->set_label (oss.str());

			GDK_THREADS_LEAVE();
		}

	}

}

void RepRapSerial::SendNow(string s)
{
	s+= "\n";
	debugPrint( string("Sending:") + s);
	com->write(s);

}

void RepRapSerial::SendData(string s, const int lineNr)
{
	if( !m_bConnected ) return;

	if( com->errorStatus() ) 
	{
		m_bConnecting = false;
		m_bConnected = false;
		m_bPrinting = false;
		{
			ToolkitLock guard;
			gui->MVC->serialConnectionLost();
		}
		return;
	}

	// Apply Downstream Multiplier

	float DSMultiplier = 1.0f;
	float ExtrusionMultiplier = 1.0f;
	if(gui)
	{
		ToolkitLock guard;

		DSMultiplier = gui->DownstreamMultiplierSlider->value();
		ExtrusionMultiplier = gui->DownstreamExtrusionMultiplierSlider->value();
	}

	if(DSMultiplier != 1.0f)
	{
		size_t pos = s.find( "F", 0);
		if( pos != string::npos )	//string::npos means not defined
		{
			size_t end = s.find( " ", pos);
			if(end == string::npos)
				end = s.length()-1;
			string number = s.substr(pos+1,end);
			string start = s.substr(0,pos+1);
			string after = s.substr(end+1,s.length()-1);
			float old_speed = ToFloat(number);
			s.clear();
			std::stringstream oss;
			oss << start << old_speed*DSMultiplier << " " <<after;
			s=oss.str();
		}
	}

	if(ExtrusionMultiplier != 1.0f)
	{
		size_t pos = s.find( "E", 0);
		if( pos != string::npos )	//string::npos means not defined
		{
			size_t end = s.find( " ", pos);
			if(end == string::npos)
				end = s.length()-1;
			string number = s.substr(pos+1,end);
			string start = s.substr(0,pos+1);
			string after = s.substr(end+1,s.length()-1);
			float old_speed = ToFloat(number);
			s.clear();
			std::stringstream oss;
			oss << start << old_speed*ExtrusionMultiplier << " " <<after;
			s=oss.str();
		}
	}


	string buffer;
	std::stringstream oss;
	oss << " N" << lineNr << " ";//*";
	buffer += oss.str();//	Hydra
	// strip comments
	
	string tmp=s;
	size_t found;
	found=tmp.find_first_of(";");
	if(found!=string::npos)
		tmp=tmp.substr(0,found);
	found=tmp.find_last_not_of(" ");
	if(found!=string::npos)
		tmp=tmp.substr(0,found+1);

	for(uint i=0;i<tmp.length();i++)
		if((tmp[i] < '*' || tmp[i] > 'z') && tmp[i] != ' ')	// *-z (ascii 42-122)
			tmp.erase(tmp.begin()+i--);

	buffer += tmp;
	buffer += " *";// Hydra
	oss.str( "" );
	// Calc checksum.
	short checksum = 0;
	unsigned char count=0;
	while(buffer[count] != '*')
		checksum = checksum^buffer[count++];

	oss << checksum;//std::setfill('0') << std::setw(2) << buffer.length()+2;
	buffer += oss.str();

	debugPrint( string("SendData:") + buffer);
	buffer += "\r\n";
	com->write(buffer);
}

extern void TempReadTimer(void *);

class ConnectionTimeOut
{
public:
	RepRapSerial* serial;
	ulong ConnectAttempt;
	int timer;
};

void ConnectionTimeOutMethod(void *data)
{
	ConnectionTimeOut* timeout = (ConnectionTimeOut*)data;
	if( timeout->serial->isConnecting() && timeout->ConnectAttempt == timeout->serial->GetConnectAttempt() )
	{
		if( --timeout->timer == 0)
		{
			timeout->serial->DisConnect("Connection attempt timed out");
		}
		else
		{
			// poll temperature once to ensure we get a responce if the firmware doesn't greet.
			timeout->serial->SendNow("M105");
			Fl::add_timeout(1.0f, &ConnectionTimeOutMethod, timeout);
			return;
		}
	}
	delete timeout;
}

void RepRapSerial::Connect(string port, int speed)
{
	bool error = false;
	if( m_bConnecting ) return;
	m_bConnected = false;
	m_bConnecting = true;
	c.notify_all();
	std::stringstream oss;
	oss << "Connecting to port: " << port  << " at speed " << speed;
	debugPrint( oss.str() );
	delete com;
	com = new RepRapBufferedAsyncSerial(this);
	try{
		com->open(port.c_str(), speed);
	} catch (std::exception& e) 
	{
		error = true;
		stringstream oss;
		oss<<"Exception: " << e.what() << ":" << port.c_str() << endl;
		debugPrint(oss.str(), true);
	}
	if( error )
	{
		ToolkitLock guard;
		m_bConnecting = false;
		gui->MVC->serialConnectionLost();
		return;
	}

	if( m_bValidateConnection )
	{
		ConnectionTimeOut* timeout= new ConnectionTimeOut();
		timeout->serial = this;
		timeout->ConnectAttempt = ++ConnectAttempt;
		timeout->timer = 5;

		Fl::add_timeout(1.0f, &ConnectionTimeOutMethod, timeout);
		// probe the device to see if it is a reprap
		SendNow("M105");
	}
	else
	{
		ToolkitLock guard;
		notifyConnection(true);
		gui->MVC->serialConnected();
	}
	Fl::add_timeout(1.0f, &TempReadTimer);
}

void RepRapSerial::notifyConnection (bool connected)
{
	m_bConnecting = false;
	m_bConnected = connected;
	c.notify_all();
}

void RepRapSerial::DisConnect(const char* reason)
{
	debugPrint(reason);
	DisConnect();
}

void RepRapSerial::DisConnect()
{
	if( m_bConnected )
		SendNow("M81");
	notifyConnection (false);
	com->close();
	Clear();

	ToolkitLock guard;
	gui->MVC->serialConnectionLost();
}

void RepRapSerial::SetLineNr(int nr)
{
	SendData("M110", nr);	// restart lineNr count
}

void RepRapSerial::SetDebugMask(int mask, bool on)
{
	if(on)
		debugMask |= mask;
	else
		debugMask &=~mask;

	SetDebugMask();

}
void RepRapSerial::SetDebugMask()
{
	std::stringstream oss;
	string buffer="M111 S";
	oss << debugMask;
	buffer += oss.str();
	SendNow(buffer);
}

/*
 * King sized, man-trap for the un-wary. This method is called
 * from a separate AsyncSerial thread; we need to get data
 * synchronisation right.
 */
void RepRapSerial::OnEvent(char* data, size_t dwBytesRead)
{
	// Read data, until there is nothing left
	data[dwBytesRead] = '\0';
	InBuffer += data;		// Buffer data for later analysis

	// Endchars = \r\n

	//		debugPrint( string("Received:\"") + szBuffer +"\" (" + stringify(dwBytesRead));
	{
		// Check inbuffer for good stuff

		// remove leading \n, \r, and spaces (also added \t for good measure)
		while(InBuffer.length() > 0 && (InBuffer.substr(0,1) == "\n" ||  InBuffer.substr(0,1) == "\r" || InBuffer.substr(0,1) == " " || InBuffer.substr(0,1) == "\t"))
			InBuffer = InBuffer.substr(1, InBuffer.length()-1);

		if(InBuffer[0] == 1)	// Ctrl
		{
			InBuffer = InBuffer.substr(2, InBuffer.length()-2);
			debugPrint("Recieved a Ctrl character", true);
		}
		if(InBuffer.size() == 0)
			return;
		size_t found;
		found=InBuffer.find_first_of("\r\n");

		while (found!=string::npos && found != 0)
		{
			bool knownCommand = true;
			string command = InBuffer.substr(0,found);

			if(0)
			{
				stringstream oss;
				oss << "Command:" << command;
				debugPrint(oss.str(), true);
			}

			if (command == "ok")	// most common, first
			{
				if(m_bPrinting)
					SendNextLine();
			}
			else if(command.substr(0,5) == "Echo:") // search, there's a parameter int (temperature)
			{
				string parameter = command.substr(5,command.length()-5);
				echo( string("Echo:") + parameter);
				// Check parameter
			}
			else if(command.substr(0,2) == "T:") // search, there's a parameter int (temperature)
			{
				string parameter = command.substr(2,command.length()-2);
				debugPrint( string("Received:") + command+ " with parameter " + parameter);

				ToolkitLock guard;

				// Reduce re-draws by only updating the GUI on a real change
				const char *old_value = gui->CurrentTempText->value();
				if (!old_value || strcmp (parameter.c_str(), old_value))
					gui->CurrentTempText->value(parameter.c_str());
			}
			else if(command == "start")
			{
				debugPrint( string("Received: start"));
			}
			else if(command.substr(0,3) == "E: ") // search, there's a parameter int (temperature_error, wait_till_hot)
			{
				string parameter = command.substr(3,command.length()-3);
				debugPrint( string("Received:") + command+ " with parameter " + parameter);
				// Check parameter
			}
			// this is the common case for the modern 5D reprap firmware 
			else if(command.substr(0,3) == "ok ") // search, there's a parameter string (debugstring)
			{

				//starting from the "ok", we'll parse the rest as "tokens"
				string s = get_next_token(command.substr(0,command.length()));
				uint l = s.length();
				while ( s.length() > 0 ) {
					string remainder = command.substr(l,command.length());
					uint ws = count_leading_whitespace(remainder);

					// s = the current token , l = the offset of this token in command
					//cout << "s:" << s << endl;
					
					// we already know this is how the line starts:
				        if ( s == "ok" ) { 
					// do nothing more

					// temperature token:
					} else if ( s.substr(0,2) == "T:" ) { 
						temp_param = s.substr(2,s.length());
				
						// Reduce re-draws by only updating the GUI on a real change
						const char *old_value = gui->CurrentTempText->value();
						if (!old_value || strcmp (temp_param.c_str(), old_value))
						gui->CurrentTempText->value(temp_param.c_str());


					// bed temperature token:
					} else if ( s.substr(0,2) == "B:" ) { 
						bedtemp_param = s.substr(2,s.length());

						// Reduce re-draws by only updating the GUI on a real change
						const char *old_value = gui->CurrentBedTempText->value();
						if (!old_value || strcmp (bedtemp_param.c_str(), old_value))
						gui->CurrentBedTempText->value(bedtemp_param.c_str());

					// a token we don't yet understand , dump to stdout for now
					} else  {
					
						cout << "unknown token:" << s << endl;	

					}

					s = get_next_token(remainder);
					l += s.length() + ws;
				}

				string parameter = command.substr(3,command.length()-3);
				debugPrint( string("Received:") + command+ " with parameter " + parameter + "**************************************", true);
				if(m_bPrinting)
					SendNextLine();
			}
			else if(command.substr(0,5) == "huh? ") // search, there's a parameter string (unknown command)
			{
				string parameter = command.substr(6,command.length()-5);
				debugPrint( string("Received:") + command+ " with parameter " + parameter, true);

				if(m_bPrinting)
					SendNextLine();
			}
			else if(command.substr(0,7) == "Resend:") // search, there's a parameter string (unknown command)
			{
				string parameter = command.substr(7,command.length()-7);
				debugPrint( string("Received:") + command+ " with parameter " + parameter, true);

				std::stringstream iss(parameter);
				iss >> m_iLineNr;	// Rewind to requested line

				if(m_bPrinting)
					SendNextLine();
			}
			else if(command.substr(0,3) == "rs ") // search, there's a parameter string (unknown command)
			{
				string parameter = command.substr(3,command.length()-3);
				debugPrint( string("Received:") + command+ " with parameter " + parameter, true);
				std::stringstream iss(parameter);
				iss >> m_iLineNr; // Rewind to requested line
				if(m_bPrinting)
					SendNextLine();
			}
			else if(command.substr(0,45) == "[FIRMWARE WARNING] invalid M-Code received: M") // search, there's a parameter string (unknown Mcode)
			{
				string parameter = command.substr(45,command.length()-45);
				debugPrint( string("Received:") + command+ " with parameter " + parameter, true);
			}
			else	// Unknown response
			{
				debugPrint( string("Received:") + command+"\n", true);
				knownCommand = false;
			}

			if (knownCommand && m_bConnecting)
			{
				ToolkitLock guard;

				// Tell GUI we are ready to go.
				notifyConnection(true);
				gui->MVC->serialConnected();
			}
			  
			InBuffer = InBuffer.substr(found);	// 2 end-line characters, \n\r

			// Empty eol control characters
			while(InBuffer.length() > 0 && (InBuffer.substr(0,1) == "\n" ||  InBuffer.substr(0,1) == "\r"))
				InBuffer = InBuffer.substr(1, InBuffer.length()-1);
			found = InBuffer.find_first_of("\r");
		}
	}
}

// given a string with "words" and "spaces", it returns the next word at the start of the string
string RepRapSerial::get_next_token(string str){

//	cout << "getting token from:" << str << endl; 

	// remove all leading spaces:
	while ( str[0] == ' ' || str[0] == '	' || str[0] == '\r' || str[0] == '\n' ) {
		str = str.substr(1,str.length()-1); // drop first char if whitespace, repeatedly
	}

	 uint data=str.find_first_of("	 \r\n",0); // locate a space or  a tab as word separator
         if ( data != string::npos ) {
          str = str.substr(0,data); 
         } 
//	cout << "got token:" << str << endl; 
	return str;

}
//count leading whitespaces only, return how many we would have stripped in the above functon
uint RepRapSerial::count_leading_whitespace(string str) {
	uint l = 0;
	// remove all leading spaces:
	while ( str[0] == ' ' || str[0] == '	' || str[0] == '\r' || str[0] == '\n' ) {
		l++;
		str = str.substr(1,str.length()-1); // drop first char if whitespace, repeatedly
	}
	return l;
}

void RepRapSerial::WaitForConnection(ulong timeoutMS)
{
	Guard a (m);
	boost::system_time until;
	until = boost::get_system_time() + boost::posix_time::milliseconds (timeoutMS);
	while (m_bConnecting && c.timed_wait(a, until));
}

void RepRapSerial::continuePrint()
{
  m_bPrinting = true;
  SendNextLine();
  m_signal_printing_changed.emit();
}

void RepRapSerial::pausePrint()
{
  m_bPrinting = false;
  m_signal_printing_changed.emit();
}
