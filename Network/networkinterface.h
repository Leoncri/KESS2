#pragma once

// parent class to bundle network interfaces for OS compatibilty

// comment out this line to compile under linux (not testet yet)

/*------------------------------------------------------------*/

#include "../config.h"

#ifdef USE_WINDOWS
#include <WinSock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class NetworkInterface
{
public:
    // constructor and destructor
    NetworkInterface();
    ~NetworkInterface();

    // send and receive
    virtual bool SendData(char* buffer, int size);
    virtual int RecvData(char* buffer, int size);

#ifdef USE_WINDOWS
    SOCKET GetInterfaceSocket();
#else
    int GetInterfaceSocket();
#endif

protected:
#ifdef USE_WINDOWS
    static bool winsockInit;
    static int numInterfaces;
    SOCKET interfaceSocket;
#else
    int interfaceSocket;
#endif
};