#include "tcpclientinterface.h"
#include "../debugoutput.h"

TCPClientInterface::TCPClientInterface()
{
    // initialize socket
#ifdef USE_WINDOWS
    interfaceSocket = INVALID_SOCKET;
#else
    interfaceSocket = -1;
#endif
}

TCPClientInterface::~TCPClientInterface()
{
}

bool TCPClientInterface::Initialize(uint32_t ip, int port)
{
    // variables
    int iResult;
    struct timeval tv;
    fd_set writeSet;

    // create socket
    //OUTPUT_DEBUG_MESSAGE("Creating new socket")
    interfaceSocket = socket(AF_INET, SOCK_STREAM, 0);
#ifdef USE_WINDOWS
    if (interfaceSocket == INVALID_SOCKET)
#else
    if (interfaceSocket == -1)
#endif
    {
        OUTPUT_DEBUG_MESSAGE("Cannot create a new socket for TCP client interface")
        return false;
    }

    // set the socket to non-blocking mode
    unsigned long iMode = 1;
    iResult = ioctlsocket(interfaceSocket, FIONBIO, &iMode);
    if (iResult != NO_ERROR)
    {
        OUTPUT_DEBUG_MESSAGE("Cannot set socket to non-blocking mode.")
    }

    // set up server information
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(ip);
    serverAddr.sin_port = htons(port);

    // connect to server
    OUTPUT_DEBUG_MESSAGE("Try to connect to device")
    iResult = connect(interfaceSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
#ifdef USE_WINDOWS
    if (iResult == SOCKET_ERROR)
    {
        // this should always happen, check if the socket would block
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            // something went wrong, close socket
            OUTPUT_DEBUG_MESSAGE("Connect function is broken.")
            shutdown(interfaceSocket, SD_SEND);
            closesocket(interfaceSocket);
            interfaceSocket = INVALID_SOCKET;
            return false;
        }
    }
#else
    if (iResult < 0)
    {
        // close socket
        close(interfaceSocket)
        interfaceSocket = -1;
        return false;
    }
#endif

    // set socket to blocking again
    iMode = 0;
    iResult = ioctlsocket(interfaceSocket, FIONBIO, &iMode);
    if (iResult != NO_ERROR)
    {
        OUTPUT_DEBUG_MESSAGE("Cannot set socket to blocking mode.")
    }

    // call a select with timeout on the socket
    FD_ZERO(&writeSet);
    FD_SET(interfaceSocket, &writeSet);

    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    select(0, NULL, &writeSet, NULL, &tv);
    if (FD_ISSET(interfaceSocket, &writeSet))
    {
        return true;
    }

    OUTPUT_DEBUG_MESSAGE("Cannot connect to device")
    return false;
}

void TCPClientInterface::Shutdown()
{
    // the socket needs to be closed if needed
#ifdef USE_WINDOWS
    if (interfaceSocket != INVALID_SOCKET)
    {
        // shutdown socket
        shutdown(interfaceSocket, SD_SEND);

        // close socket
        closesocket(interfaceSocket);
        interfaceSocket = INVALID_SOCKET;
    }
#else
    if (interfaceSocket >= 0)
    {
        // shutdown socket
        shutdown(interfaceSocket, SD_SEND);
        
        // clsoe socket
        close(interfaceSocket)
        interfaceSocket = -1;
    }
#endif

    return;
}

bool TCPClientInterface::SendData(char* data, int length)
{
    // Variables
    FD_SET write_fs;
    struct timeval tv;
    int result = -1;

    // check if socket can be written
    FD_ZERO(&write_fs);
    FD_SET(interfaceSocket, &write_fs);

    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    select(0, NULL, &write_fs, NULL, &tv);

    if (FD_ISSET(interfaceSocket, &write_fs))
    {
        // send data, should work with both OS
        result = send(interfaceSocket, data, length, 0);
    }
    
    // check if data was correctly send
    if (result != length)
    {
        return false;
    }

    return true;
}

int TCPClientInterface::RecvData(char* buffer, int size)
{
    // variables
    int bytesToRead;
    int bytesRead;
    int result;

    // loop until all bytes are read in or the socket is empty
    bytesToRead = size;
    bytesRead = 0;
    while (bytesToRead > 0)
    {
        // check if there is something to read
        if (!SocketReady())
        {
            return bytesRead;
        }

        // read in
        result = recv(interfaceSocket, buffer + bytesRead, bytesToRead, 0);
        if (result < 0)
        {
            // error, return -1
            return -1;
        }

        // add bytes and take another approach
        bytesToRead -= result;
        bytesRead += result;
    }

    return bytesRead;
}

bool TCPClientInterface::SocketReady()
{
    // variables
    fd_set fds;
    struct timeval tv;

    // set up file descriptor set
    FD_ZERO(&fds);
    FD_SET(interfaceSocket, &fds);

    // timeout
    tv.tv_sec = 0;
    tv.tv_usec = 1;
    
    // call select
    select(interfaceSocket + 1, &fds, NULL, NULL, &tv);

    // check if socket is in list
    if(FD_ISSET(interfaceSocket, &fds))
    {
        return true;
    }

    return false;
}