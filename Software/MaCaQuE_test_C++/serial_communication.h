/*
* Class for Teensy (Arduino) Serial Communication (header file)
*
* mostly adapted from: http://www.pjrc.com/tmp/host_software/
* alternative: http://playground.arduino.cc/Interfacing/CPPWindows
*
* This class opens one serial connection and controls it.
* 
* author: Michael Berger
* German Primate Center
* mail: mberger@dpz.eu
*/

#ifndef _SERIAL_COMMUNICATION_
#define _SERIAL_COMMUNICATION_

// OS specific implementations
#ifdef __APPLE__
#include <poll.h>
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>
#define PORTTYPE int
#define BAUD B115200
#define PORT "/dev/cu.usbmodem11261"
//"/dev/cu.usbmodem12341"
//#if defined(LINUX)
//#include <sys/ioctl.h>
//#include <linux/serial.h>
#else
#include <windows.h>
#define PORTTYPE HANDLE
#define BAUD 115200
#define PORT "COM3"
#endif

// further implementations
#include <mutex>
#include <string>



/**
  @brief:
**/

class serial_communication {

private:
    // private members
    // port handle
    PORTTYPE mPort;
    // Connection status
    bool connected;
    // mutex to make it save for mutlithreading

    std::mutex _mutex;

    
    
    
    // open a port and, set baudrate (+ standard cfg) or die
    PORTTYPE open_port_and_set_baud_or_die(const char *name, long baud);
    void close_port(PORTTYPE port);
    
    // get error msg
    void die(const char *format, ...);
    
    
    std::string name;
    long baud_rate;
    
#ifdef __APPLE__
//#elif __LINUX__
#else
    // config
    COMMCONFIG mCfg;
    //timeout config
    COMMTIMEOUTS mTimeout;
    //Get various information about the connection
    COMSTAT mStatus;
    //Keep track of last error
    DWORD mErrors;

#endif


public:
    // Constructor
    serial_communication();
    serial_communication(const std::string &nm);
    serial_communication(const std::string &nm, long baud);
  
    

    // Destructor
    ~serial_communication();


    //void connect();
    
#ifndef __APPLE__
    DWORD get_errors();
#endif

    bool connect();
    
    bool get_connected();

    

    // receive function
    int receive_bytes(unsigned char *buf, int len);

    // transmit function
    int transmit_bytes(const unsigned char *data, int len); // why unsigned char and not char as input?? (in original)

    // close the port



////OS specific function
//#ifdef __APPLE__
////#elif __LINUX__
//#else
    // functions to print info in command line
    void print_communication_state();
    void get_status();
  
    // clears buffer
	// MAC: flush stream (parameter in is not used)
	// WIN: in = 1: terminate outstanding read | 2: clear input | 3: terminates outstanding write | else: clear output
    bool purge(int in);

    
    // Arduino-Playground version (Windows only)
    int read_data(unsigned  char *buffer, unsigned int nbChar);
    //better use read_data
    int read_all_data(unsigned char *buffer);

  
};

#endif // _SERIAL_COMMUNICATION_