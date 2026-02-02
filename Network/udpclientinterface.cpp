#include "udpclientinterface.h"

UDPClientInterface::UDPClientInterface()
{
    // Initialize sockets
#ifdef USE_WINDOWS
    interfaceSocket = INVALID_SOCKET;
#else
    interfaceSocket = -1;
#endif
}

UDPClientInterface::~UDPClientInterface()
{
}

bool UDPClientInterface::Initialize(uint32_t ip, int port)
{
    // variables
    int iResult;

    // create socket
    interfaceSocket = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef USE_WINDOWS
    if (interfaceSocket == INVALID_SOCKET)
#else
    if (interfaceSocket < 0)
#endif
    {
        return false;
    }

    // set up recv information
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(ip);
    serverAddr.sin_port = htons(port);

    // bind socket
    iResult = bind(interfaceSocket, (const struct sockaddr*)&serverAddr, sizeof(sockaddr_in));
#ifdef USE_WINDOWS
    if (iResult == SOCKET_ERROR)
    {
        // close socket
        closesocket(interfaceSocket);
        interfaceSocket = INVALID_SOCKET;
        return false;
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

    return true;
}

void UDPClientInterface::Shutdown()
{
    // the socket needs to be closed if needed
#ifdef USE_WINDOWS
    if (interfaceSocket != INVALID_SOCKET)
    {
        // close socket
        closesocket(interfaceSocket);
        interfaceSocket = INVALID_SOCKET;
    }
#else
    if (interfaceSocket >= 0)
    {
        // clsoe socket
        close(interfaceSocket)
        interfaceSocket = -1;
    }
#endif

    return;
}

bool UDPClientInterface::SendData(char* data, int length)
{
    // send data, should work with both OS
    int result = sendto(interfaceSocket, data, length, 0, (const sockaddr*)&serverAddr, sizeof(sockaddr_in));

    // check if data was correctly send
    if (result != length)
    {
        return false;
    }

    return true;
}

int UDPClientInterface::RecvData(char* buffer, int length)
{
    // variables
    sockaddr_in recvAddr;
    int size = sizeof(sockaddr_in);

    // recv data from socket, should work with both OS
    int result = recvfrom(interfaceSocket, buffer, length, 0, (sockaddr*)&recvAddr, &size);

    // check against server address
    if (recvAddr.sin_addr.s_addr != serverAddr.sin_addr.s_addr)
    {
        return 0;
    }

    return result;
}

bool UDPClientInterface::SocketReady()
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