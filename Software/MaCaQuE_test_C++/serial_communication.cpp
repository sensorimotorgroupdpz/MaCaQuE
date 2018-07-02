/*
* Class for Teensy (Arduino) Serial Communication (cpp file)
*
* mostly adapted from: http://www.pjrc.com/tmp/host_software/
* 
* author: Michael Berger
* German Primate Center
* mail: mberger@dpz.eu
*/

// local includes
#include "serial_communication.h"

// global includes
#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>

#ifdef __APPLE__
#include <poll.h>
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#else
#include <tchar.h>
#endif


// Constructors
serial_communication::serial_communication()
{
    name=PORT;
    baud_rate=BAUD;
    connected=false;
    //this->mPort = this->open_port_and_set_baud_or_die(PORT, BAUD);
}

serial_communication::serial_communication(const std::string &nm)
{
    name=nm;
    baud_rate=BAUD;
    connected=false;
    //this->mPort = this->open_port_and_set_baud_or_die(name, BAUD);
}

serial_communication::serial_communication(const std::string &nm, long baud)
{
    name=nm;
    baud_rate=baud;
    connected=false;
    //this->mPort = this->open_port_and_set_baud_or_die(name, baud);
}

//Destructor
serial_communication::~serial_communication()
{
    if (connected)
    {
        this->close_port(this->mPort);
    }

    std::cout << "serial_communication destructor done" << std::endl;
}


#ifndef __APPLE__
DWORD serial_communication::get_errors()
{
    std::lock_guard<std::mutex> guard(this->_mutex);
    return this->mErrors;
}
#endif

bool serial_communication::connect()
{
    this->mPort = this->open_port_and_set_baud_or_die(name.c_str(), baud_rate);
    return connected;
}


bool serial_communication::get_connected()
{
    std::lock_guard<std::mutex> guard(this->_mutex);
    return this->connected;
}


// actual serial communications function, non-OS specific
PORTTYPE serial_communication::open_port_and_set_baud_or_die(const char *name, long baud)
{
    // port
    PORTTYPE fd;

    if (this->connected)
    {
        // what to do in this case? 
        // At the moment it is not considered, that multiple ports can be opened
    }

    this->connected = false;
    std::cout << "starting serial connection at port: " << name << std::endl;

#ifdef __APPLE__
	struct termios tinfo;
	fd = open(name, O_RDWR | O_NONBLOCK); // | O_NDELAY
	if (fd < 0) die("unable to open port %s\n", name);
	if (tcgetattr(fd, &tinfo) < 0) die("unable to get serial parms\n");
	if (cfsetspeed(&tinfo, baud) < 0) die("error in cfsetspeed\n");
	tinfo.c_cflag |= CLOCAL;
	if (tcsetattr(fd, TCSANOW, &tinfo) < 0) die("unable to set baud rate\n");
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
    // connected should be set if connection was established
    // is there an easier way to check it? :
    if ( (fd >= 0)&&(tcgetattr(fd, &tinfo) >= 0)&&(cfsetspeed(&tinfo, baud) >= 0)&&(tcsetattr(fd, TCSANOW, &tinfo) >= 0) )
    {
        this->connected = true;
    }

//#elif defined(LINUX)
    //	struct termios tinfo;
    //	struct serial_struct kernel_serial_settings;
    //	int r;
    //	fd = open(name, O_RDWR);
    //	if (fd < 0) die("unable to open port %s\n", name);
    //	if (tcgetattr(fd, &tinfo) < 0) die("unable to get serial parms\n");
    //	if (cfsetspeed(&tinfo, baud) < 0) die("error in cfsetspeed\n");
    //	if (tcsetattr(fd, TCSANOW, &tinfo) < 0) die("unable to set baud rate\n");
    //	r = ioctl(fd, TIOCGSERIAL, &kernel_serial_settings);
    //	if (r >= 0) {
    //		kernel_serial_settings.flags |= ASYNC_LOW_LATENCY;
    //		r = ioctl(fd, TIOCSSERIAL, &kernel_serial_settings);
    //		if (r >= 0) printf("set linux low latency mode\n");
    //	}
#else

	DWORD n;
	char portname[256];
	
    int num;
	if (sscanf_s(name, "COM%d", &num) == 1) {
		sprintf_s(portname, "\\\\.\\COM%d", num); // Microsoft KB115831
	} else {
		strncpy_s(portname, name, sizeof(portname)-1);
		//portname[n-1] = 0; // leads to error
	}
	//to have it work on windows, the project properties>configuration properties>General>character set must be set to multibyte character set instead of unicode
	fd = CreateFile(portname, GENERIC_READ | GENERIC_WRITE,
                    0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fd == INVALID_HANDLE_VALUE) 
    { die("unable to open port %s\n", name); } // connection failed
    else
    { 
        this->connected = true;  // connection succeeded
	
        // set configurations
        GetCommConfig(fd, &this->mCfg, &n);
	    this->mCfg.dcb.BaudRate = baud;
	    this->mCfg.dcb.fBinary = TRUE;
	    this->mCfg.dcb.fParity = FALSE;
	    this->mCfg.dcb.fOutxCtsFlow = FALSE;
	    this->mCfg.dcb.fOutxDsrFlow = FALSE;
	    this->mCfg.dcb.fOutX = FALSE;
	    this->mCfg.dcb.fInX = FALSE;
	    this->mCfg.dcb.fErrorChar = FALSE;
	    this->mCfg.dcb.fNull = FALSE;
	    this->mCfg.dcb.fRtsControl = RTS_CONTROL_ENABLE;
	    this->mCfg.dcb.fAbortOnError = FALSE;
	    this->mCfg.dcb.ByteSize = 8;
	    this->mCfg.dcb.Parity = NOPARITY;
	    this->mCfg.dcb.StopBits = ONESTOPBIT;
	    this->mCfg.dcb.fDtrControl = DTR_CONTROL_ENABLE;
	    SetCommConfig(fd, &this->mCfg, n);

        // set timeouts
	    GetCommTimeouts(fd, &this->mTimeout);
	    this->mTimeout.ReadIntervalTimeout = 0;
	    this->mTimeout.ReadTotalTimeoutMultiplier = 0;
	    this->mTimeout.ReadTotalTimeoutConstant = 1000;
	    this->mTimeout.WriteTotalTimeoutConstant = 0;
	    this->mTimeout.WriteTotalTimeoutMultiplier = 0;
	    SetCommTimeouts(fd, &this->mTimeout);
    }
#endif

    // check if everything worked out
    if (this->connected)
    {
        std::cout << "serial connection established" << std::endl;
    }
	return fd;  
}

int serial_communication::receive_bytes(unsigned char *buf, int len)
{
    std::lock_guard<std::mutex> guard(this->_mutex);
    // file to read len bytes of
	int count=0;
#ifdef __APPLE__
	int r;
	int retry=0;
	//char buf[512];
    
	//if (len > sizeof(buf) || len < 1) return -2;
	// non-blocking read mode
	fcntl(this->mPort, F_SETFL, fcntl(this->mPort, F_GETFL) | O_NONBLOCK);
	while (count < len) {
		r = read(this->mPort, buf + count, len - count);
		//printf("read, r = %d\n", r);
		if (r < 0 && errno != EAGAIN && errno != EINTR) return -1;
		else if (r > 0) count += r;
		else {
			// no data available right now, must wait
			fd_set fds;
			struct timeval t;
			FD_ZERO(&fds);
			FD_SET(this->mPort, &fds);
			t.tv_sec = 1;
			t.tv_usec = 0;
			r = select(this->mPort+1, &fds, NULL, NULL, &t);
			//printf("select, r = %d\n", r);
			if (r < 0) return -1;
			if (r == 0) return count; // timeout
		}
		retry++;
		if (retry > 1000) return -100; // no input
	}
	fcntl(this->mPort, F_SETFL, fcntl(this->mPort, F_GETFL) & ~O_NONBLOCK);
#else

    // number of bytes read by ReadFile
	DWORD bytesRead;
    // return value of ReadFile
	BOOL r;
	int waiting=0;
  
    // set timeout in a non-blocking state
	//GetCommTimeouts(port, &timeout);
	this->mTimeout.ReadIntervalTimeout = MAXDWORD; // non-blocking
	this->mTimeout.ReadTotalTimeoutMultiplier = 0;
	this->mTimeout.ReadTotalTimeoutConstant = 0;
	SetCommTimeouts(this->mPort, &this->mTimeout);

    
    // loop 
	while (count < len) {
        // read len-count bytes and write inte buffer
		r = ReadFile(this->mPort, buf + count, len - count, &bytesRead, NULL);
        // check if reading was successfull
		if (!r) die("read error\n");
        //
		if (bytesRead > 0) count += bytesRead;
		else 
        {
            //1 sec timeout
			if (waiting) break;  
			this->mTimeout.ReadIntervalTimeout = MAXDWORD;
			this->mTimeout.ReadTotalTimeoutMultiplier = MAXDWORD;
			this->mTimeout.ReadTotalTimeoutConstant = 1000;
			SetCommTimeouts(this->mPort, &this->mTimeout);
			waiting = 1;
		}
	}

    // set timouts back (this was not in the original code ... why?)
    this->mTimeout.ReadIntervalTimeout = 0;
	this->mTimeout.ReadTotalTimeoutMultiplier = 0;
	this->mTimeout.ReadTotalTimeoutConstant = 1000;
	SetCommTimeouts(this->mPort, &this->mTimeout);
#endif

	return count;
}


int serial_communication::transmit_bytes(const unsigned char *data, int len)
{
    std::lock_guard<std::mutex> guard(this->_mutex);
#ifdef __APPLE__
	return write(this->mPort, data, len);
#else
    // number of bytes written by WriteFile
	DWORD bytesWritten;
    // return value of writeFile
    BOOL r;

    //write data 
	r = WriteFile(this->mPort, data, len, &bytesWritten, NULL);
	if (!r) return 0;
	return bytesWritten;
#endif
}


void serial_communication::close_port(PORTTYPE port)
{
    std::lock_guard<std::mutex> guard(this->_mutex);
    this->connected = false;
#ifdef __APPLE__
	close(port);
#else
	CloseHandle(port);
#endif
}

// display error
void serial_communication::die(const char *format, ...)
{
    //std::lock_guard<std::mutex> guard(this->_mutex); // this is not locked since it is a protected member function
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	//exit(1);
}


// OS-specific functions:

// functions from arduino-playground class (Win only)
// debug info: some dcb values
void serial_communication::print_communication_state()
{
    std::lock_guard<std::mutex> guard(this->_mutex);
#ifdef __APPLE__
    std::cout<<"All is good you're on a mac"<<std::endl;
#else
    DCB dcb = this->mCfg.dcb;

    //  Print some of the DCB structure values
//    _tprintf( TEXT("\nBaudRate = %d, ByteSize = %d, Parity = %d, StopBits = %d\n"), 
//              dcb.BaudRate, 
//              dcb.ByteSize, 
//              dcb.Parity,
//              dcb.StopBits );
    std::cout<<"Baudrate = "<<dcb.BaudRate<<", ByteSize = "<<dcb.ByteSize<<",Parity = "<<dcb.Parity<<", StopBits = "<<dcb.StopBits<<std::endl;
#endif
}

//debug info: last error msg + status
void serial_communication::get_status()
{
#ifdef DOUBLE_MUTEX
    std::lock_guard<std::mutex> guard(this->read_mutex);
    std::lock_guard<std::mutex> guardw(this->write_mutex);
#else
    std::lock_guard<std::mutex> guard(this->_mutex);
#endif
#ifdef __APPLE__
    std::cout<<"All is good you're on a mac"<<std::endl;
#else
    //Use the ClearCommError function to get status info on the Serial port
    ClearCommError(this->mPort, &this->mErrors, &this->mStatus);
    if (this->mErrors)
	{
		std::cout << "Error detected: " << this->mErrors << std::endl;
	}

	std::cout << "number of bytes received but not yet read: " << this->mStatus.cbInQue << std::endl;
  
	std::cout << "number of bytes remaining to be transmitted, zero for a nonoverlapped write: " << this->mStatus.cbOutQue << std::endl;

    if (this->mStatus.fCtsHold)
	{
		std::cout << "transmission is waiting for the CTS (clear-to-send) signal to be sent" << std::endl;
	}
    if (this->mStatus.fDsrHold)
	{
		std::cout << "transmission is waiting for the DSR (data-set-ready) signal to be sent." << std::endl;
	}
    if (this->mStatus.fEof)
	{
		std::cout << "the end-of-file (EOF) character has been received." << std::endl;
	}
    if (this->mStatus.fRlsdHold)
	{
		std::cout << "transmission is waiting for the RLSD (receive-line-signal-detect) signal to be sent." << std::endl;
	}
    if (this->mStatus.fTxim)
	{
		std::cout << "there is a character queued for transmission that has come to the communications device by way of the TransmitCommChar function." << std::endl;
	}
    if (this->mStatus.fXoffHold)
	{
		std::cout << "transmission is waiting because the XOFF character was received." << std::endl;
	}
    if (this->mStatus.fXoffSent)
	{
		std::cout << "transmission is waiting because the XOFF character was transmitted." << std::endl;
	}
#endif
}

// purge input or output buffer
bool serial_communication::purge(int in)
{
    std::lock_guard<std::mutex> guard(this->_mutex);
#ifdef __APPLE__
    return tcflush(mPort, TCIOFLUSH);
#else
    // this function calles the purgeComm function
    DWORD flag;
  
    // input in sets one of the flags
    switch (in)
    {
    case 1:
        {
            flag = PURGE_RXABORT; // Terminates all outstanding overlapped read operations
            break;
        }
    case 2:
        {
            flag = PURGE_RXCLEAR; // clears input buffer
            break;
        }
    case 3:
        {
            flag = PURGE_TXABORT; // Terminates all outstanding overlapped write operations
            break;
        }
    default:
        {
            flag = PURGE_TXCLEAR; // clears output buffer
            break;
        }
    }

    return PurgeComm(this->mPort, flag);
#endif
}


// read data
int serial_communication::read_data(unsigned char *buffer, unsigned int nbChar)
{
    std::lock_guard<std::mutex> guard(this->_mutex);
    
    
#ifdef __APPLE__
    
    int count=0;
    
    unsigned int toRead;
    ioctl(this->mPort, FIONREAD, &toRead);
    fcntl(this->mPort, F_SETFL, fcntl(this->mPort, F_GETFL) | O_NONBLOCK);
    int r = read(this->mPort,  buffer + count, nbChar - count);
    if (r < 0 && errno != EAGAIN && errno != EINTR)
        return -1;
    else if (r > 0)
        count += r;
    else
        return -1;
    
    fcntl(this->mPort, F_SETFL, fcntl(this->mPort, F_GETFL) & ~O_NONBLOCK);
    return count;
    
#else
    //Number of bytes we'll have read
    DWORD bytesRead;
    //Number of bytes we'll really ask to read
    DWORD toRead;

    //Use the ClearCommError function to get status info on the Serial port
    ClearCommError(this->mPort, &this->mErrors, &this->mStatus);

    //Check if there is something to read
    if(this->mStatus.cbInQue>0)
    {
        //If there is we check if there is enough data to read the required number
        //of characters, if not we'll read only the available characters to prevent
        //locking of the application.
        if(this->mStatus.cbInQue > ((DWORD) nbChar))
        {
            toRead = (DWORD) nbChar;
        }
        else
        {
            toRead = this->mStatus.cbInQue;
        }

        //Try to read the require number of chars, and return the number of read bytes on success
        if(ReadFile(this->mPort, buffer, toRead, &bytesRead, NULL) && bytesRead != 0)
        {
            return ((int) bytesRead);
        }

    }

    //If nothing has been read, or that an error was detected return -1
    return -1;
#endif

}

// same as read data, but read out all the data in the que
//ATTENTION: if there is lots of data in the que, this can take a huge amount of time
// better use read_data
int serial_communication::read_all_data(unsigned char *buffer)
{
    std::lock_guard<std::mutex> guard(this->_mutex);
    
#ifdef __APPLE__
    
    int count=0;
    
    unsigned int toRead;
    ioctl(this->mPort, FIONREAD, &toRead);
    fcntl(this->mPort, F_SETFL, fcntl(this->mPort, F_GETFL) | O_NONBLOCK);
    int r = read(this->mPort, buffer + count, toRead - count);
    if (r < 0 && errno != EAGAIN && errno != EINTR)
        return -1;
    else if (r > 0)
        count += r;
    else
        return -1;
    
    fcntl(this->mPort, F_SETFL, fcntl(this->mPort, F_GETFL) & ~O_NONBLOCK);
    return count;
    
#else
    //Number of bytes we'll have read
    DWORD bytesRead;
    //Number of bytes we'll really ask to read
    unsigned int toRead;

    //Use the ClearCommError function to get status info on the Serial port
    ClearCommError(this->mPort, &this->mErrors, &this->mStatus);

    //Check if there is something to read
    if(this->mStatus.cbInQue>0)
    {

        toRead = this->mStatus.cbInQue;

        //Try to read the require number of chars, and return the number of read bytes on success
        if(ReadFile(this->mPort, buffer, toRead, &bytesRead, NULL) && bytesRead != 0)
        {
            return bytesRead;
        }

    }

    //If nothing has been read, or that an error was detected return -1
    return -1;
#endif

}
