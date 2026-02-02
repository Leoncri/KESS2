#pragma once

#include "networkinterface.h"
#include "../config.h"

#define MAX_BACKLOG     4

typedef struct
{
#ifdef USE_WINDOWS
    SOCKET socket;
#else
    int socket;
#endif
} TCPInterfaceClient_t;


class TCPServerInterface : public NetworkInterface
{
public:
    // Constructor and Destructor
    TCPServerInterface();
    ~TCPServerInterface();

    // Init and shutdown
    bool Initialize(int port, int maxConnections);
    void Shutdown();
    
    // Accept new socket
    int Accept();

    // close specific socket
    void CloseConnection(int connection);

    // Send function
    bool Send(int connection, char* data, int length);
    bool SendData(char* data, int length);

    // Recv function
    bool Recv(int connection, char* buffer, int length);
    int RecvData(char* buffer, int length);

    // Function to find out which connection can be read
    int GetConnectionWithData();

private:
    // readBytes function
    int RecvBytes(int connection, char* buffer, int length);

private:
    // server connections
    TCPInterfaceClient_t* clientConnections;
    sockaddr_in serverAddr;
    int maxConnections;
    int connectionCount;
};