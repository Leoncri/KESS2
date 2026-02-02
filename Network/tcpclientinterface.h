#pragma once

#include "networkinterface.h"
#include <stdint.h>

class TCPClientInterface : public NetworkInterface
{
public:
    // constructor and destructor
    TCPClientInterface();
    ~TCPClientInterface();

    // init and shutdown
    bool Initialize(uint32_t ip, int port);
    void Shutdown();

    // send functions
    bool SendData(char* buffer, int size);

    // recv functions
    int RecvData(char* buffer, int size);

    // returns true if there is something to read in this socket
    bool SocketReady();

private:
    // server address
    sockaddr_in serverAddr;
};