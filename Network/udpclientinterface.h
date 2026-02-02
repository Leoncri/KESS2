#pragma once

#include "networkinterface.h"
#include <stdint.h>

class UDPClientInterface : public NetworkInterface
{
public:
    // constructor and destructor
    UDPClientInterface();
    ~UDPClientInterface();

    // init and shutdown
    bool Initialize(uint32_t ip, int port);
    void Shutdown();

    // send functions
    bool SendData(char* data, int length);

    // recv functions
    int RecvData(char* buffer, int length);

    // returns true if there is something to read in this socket
    bool SocketReady();

private:
    // socket is defined by network interface class
    sockaddr_in serverAddr;
};