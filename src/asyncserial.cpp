/*
 * File:   asyncserial.cpp
 * Author: Terraneo Federico
 * Distributed under the Boost Software License, Version 1.0.
 * Created on September 7, 2009, 10:46 AM
 */
#include "stdafx.h"
#include "AsyncSerial.h"

#include <string>
#include <algorithm>

using namespace std;
using namespace boost;

//Class AsyncSerial

AsyncSerial::AsyncSerial(): io_(), port_(io_), thread_(), error_(false)
{
}

AsyncSerial::AsyncSerial(const std::string& devname, unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
        : io_(), port_(io_), thread_(), error_(false)
{
    open(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop);
}

void AsyncSerial::open(const std::string& devname, unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
{
    if(isOpen()) close();

    setErrorStatus(true);//If an exception is thrown, error_ remains true
    port_.open(devname);
    port_.set_option(asio::serial_port_base::baud_rate(baud_rate));
    port_.set_option(opt_parity);
    port_.set_option(opt_csize);
    port_.set_option(opt_flow);
    port_.set_option(opt_stop);

#if __APPLE__
    // Note that open() follows POSIX semantics: multiple open() calls to the same file will succeed
    // unless the TIOCEXCL ioctl is issued. This will prevent additional opens except by root-owned
    // processes.
    // See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.

    int fd = port_.native();
    if (ioctl(fd, TIOCEXCL) == -1) {
    	setErrorStatus(true);
    	return;
    }
#endif

    readStart();

    thread t(bind(&asio::io_service::run, &io_));
    thread_.swap(t);
    setErrorStatus(false);//If we get here, no error
}

bool AsyncSerial::isOpen() const
{
    return port_.is_open();
}

bool AsyncSerial::errorStatus() const
{
    return error_;
}

void AsyncSerial::close()
{
    if(!isOpen()) return;

    io_.post(bind(&AsyncSerial::doClose, this));
    thread_.join();
    if(errorStatus())
    {
      throw(boost::system::system_error(boost::system::error_code(),
					"Error while closing the device"));
    }
}

void AsyncSerial::write(const char *data, size_t size)
{
    {
        lock_guard<mutex> l(writeQueueMutex_);
        writeQueue_.insert(writeQueue_.end(),data,data+size);
    }
    io_.post(bind(&AsyncSerial::doWrite, this));
}

void AsyncSerial::write(const std::vector<char>& data)
{
	{
		lock_guard<mutex> l(writeQueueMutex_);
		writeQueue_.insert(writeQueue_.end(),data.begin(),data.end());
	}
	io_.post(bind(&AsyncSerial::doWrite, this));
}
void AsyncSerial::write(const std::string &data)
{
	{
		lock_guard<mutex> l(writeQueueMutex_);
		writeQueue_.insert(writeQueue_.end(),data.begin(),data.end());
	}
	io_.post(bind(&AsyncSerial::doWrite, this));
}

void AsyncSerial::writeString(const std::string& s)
{
    {
        lock_guard<mutex> l(writeQueueMutex_);
        writeQueue_.insert(writeQueue_.end(),s.begin(),s.end());
    }
    io_.post(bind(&AsyncSerial::doWrite, this));
}

AsyncSerial::~AsyncSerial()
{
    if(isOpen())
    {
        try {
            close();
        } catch(...)
        {
            //Don't throw from a destructor
        }
    }
}

void AsyncSerial::readStart()
{
    port_.async_read_some(asio::buffer(readBuffer_, readBufferSize),
            bind(&AsyncSerial::readEnd,
            this,
            asio::placeholders::error,
            asio::placeholders::bytes_transferred));
}

void AsyncSerial::readEnd(const boost::system::error_code& error,
        size_t bytes_transferred)
{
    if(error)
    {
    	//error can be true even because the serial port was closed.
        //In this case it is not a real error, so ignore
        if(isOpen())
        {
            doClose();
            setErrorStatus(true);
        }
    } else {
        if(callback_) callback_(readBuffer_,bytes_transferred);
       	OnEvent(readBuffer_,bytes_transferred);
        readStart();
    }
}

void AsyncSerial::doWrite()
{
    //If a write operation is already in progress, do nothing
    if(writeBuffer_==0)
    {
        lock_guard<mutex> l(writeQueueMutex_);
        writeBufferSize_=writeQueue_.size();
        writeBuffer_.reset(new char[writeQueue_.size()]);
        copy(writeQueue_.begin(),writeQueue_.end(),writeBuffer_.get());
        writeQueue_.clear();
        async_write(port_,asio::buffer(writeBuffer_.get(), writeBufferSize_),
            bind(&AsyncSerial::writeEnd, this, asio::placeholders::error));
    }
}

void AsyncSerial::writeEnd(const boost::system::error_code& error)
{
    if(!error)
    {
        lock_guard<mutex> l(writeQueueMutex_);
        if(writeQueue_.empty())
        {
            writeBuffer_.reset();
            writeBufferSize_=0;

            return;
        }
        writeBufferSize_=writeQueue_.size();
        writeBuffer_.reset(new char[writeQueue_.size()]);
        copy(writeQueue_.begin(),writeQueue_.end(),writeBuffer_.get());
        writeQueue_.clear();
        async_write(port_,asio::buffer(writeBuffer_.get(), writeBufferSize_),
            bind(&AsyncSerial::writeEnd, this, asio::placeholders::error));
    } else {
        setErrorStatus(true);
        doClose();
    }
}

void AsyncSerial::doClose()
{
    boost::system::error_code ec;
    port_.close(ec);
    if(ec) setErrorStatus(true);
}

void AsyncSerial::setErrorStatus(bool e)
{
    error_=e;
}

void AsyncSerial::setReadCallback(const
        function<void (const char*, size_t)>& callback)
{
    callback_=callback;
}

void AsyncSerial::clearReadCallback()
{
    callback_.clear();
}

//Class CallbackAsyncSerial

CallbackAsyncSerial::CallbackAsyncSerial(): AsyncSerial()
{

}

CallbackAsyncSerial::CallbackAsyncSerial(const std::string& devname,
        unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
        :AsyncSerial(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop)
{

}

void CallbackAsyncSerial::setCallback(const function<void (const char*, size_t)>& callback)
{
    setReadCallback(callback);
}

void CallbackAsyncSerial::clearCallback()
{
    clearReadCallback();
}

CallbackAsyncSerial::~CallbackAsyncSerial()
{
    clearReadCallback();
}

//Class BufferedAsyncSerial

BufferedAsyncSerial::BufferedAsyncSerial(): AsyncSerial()
{

}

BufferedAsyncSerial::BufferedAsyncSerial(const std::string& devname,
        unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
        :AsyncSerial(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop)
{
    setReadCallback(bind(&BufferedAsyncSerial::readCallback, this, _1, _2));
}

size_t BufferedAsyncSerial::read(char *data, size_t size)
{
    lock_guard<mutex> l(readQueueMutex_);
    size_t result=min(size,readQueue_.size());
    vector<char>::iterator it=readQueue_.begin()+result;
    copy(readQueue_.begin(),it,data);
    readQueue_.erase(readQueue_.begin(),it);
    return result;
}

std::vector<char> BufferedAsyncSerial::read()
{
    lock_guard<mutex> l(readQueueMutex_);
    vector<char> result;
    result.swap(readQueue_);
    return result;
}

std::string BufferedAsyncSerial::readString()
{
    lock_guard<mutex> l(readQueueMutex_);
    string result(readQueue_.begin(),readQueue_.end());
    readQueue_.clear();
    return result;
}

std::string BufferedAsyncSerial::readStringUntil(const std::string delim)
{
    lock_guard<mutex> l(readQueueMutex_);
    vector<char>::iterator it=findStringInVector(readQueue_,delim);
    if(it==readQueue_.end()) return "";
    string result(readQueue_.begin(),it);
    it+=delim.size();//Do remove the delimiter from the queue
    readQueue_.erase(readQueue_.begin(),it);
    return result;
}

void BufferedAsyncSerial::readCallback(const char *data, size_t len)
{
    lock_guard<mutex> l(readQueueMutex_);
    readQueue_.insert(readQueue_.end(),data,data+len);
}

std::vector<char>::iterator BufferedAsyncSerial::findStringInVector(
        std::vector<char>& v,const std::string& s)
{
    if(s.size()==0) return v.end();

    vector<char>::iterator it=v.begin();
    for(;;)
    {
        vector<char>::iterator result=find(it,v.end(),s[0]);
        if(result==v.end()) return v.end();//If not found return

        for(size_t i=0;i<s.size();i++)
        {
            vector<char>::iterator temp=result+i;
            if(temp==v.end()) return v.end();
            if(s[i]!=*temp) goto mismatch;
        }
        //Found
        return result;

        mismatch:
        it=result+1;
    }
}

BufferedAsyncSerial::~BufferedAsyncSerial()
{
    clearReadCallback();
}

