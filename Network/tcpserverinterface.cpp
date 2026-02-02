#include "tcpserverinterface.h"
#include "../debugoutput.h"

TCPServerInterface::TCPServerInterface()
{
    maxConnections = -1;
    connectionCount = 0;
    clientConnections = 0;
}

TCPServerInterface::~TCPServerInterface()
{
}

bool TCPServerInterface::Initialize(int port, int connections)
{
    // variables
    int iResult;
    int socketError;

    // debug
    OUTPUT_DEBUG_MESSAGE("Start initializing TCP server interface")
    OUTPUT_DEBUG_MESSAGE("Create listen socket...")

    // create socket
    interfaceSocket = socket(AF_INET, SOCK_STREAM, 0);
#ifdef USE_WINDOWS
    if (interfaceSocket == INVALID_SOCKET)
#else
    if (interfaceSocket < 0)
#endif
    {
        OUTPUT_ERROR("Cannot create listen socket")
        return false;
    }

    // set up addr descriptor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // debug
    OUTPUT_DEBUG_MESSAGE("Bind TCP server socket...")

    // bind socket
    iResult = bind(interfaceSocket, (const sockaddr*)&serverAddr, sizeof(serverAddr));
#ifdef USE_WINDOWS
    if (iResult == SOCKET_ERROR)
    {
        // close the newly created socket
        socketError = WSAGetLastError();
        OUTPUT_ERROR("Cannot bind TCP server socket; Error code " << socketError)
        closesocket(interfaceSocket);
        return false;
    }
#else
    if (iResult != 0)
    {
        // close the newly created socket
        close(interfaceSocket);
        return false;
    }
#endif

    // debug
    OUTPUT_DEBUG_MESSAGE("Set socket to listen mode...")

    // set socket to listen mode
    iResult = listen(interfaceSocket, MAX_BACKLOG);
#ifdef USE_WINDOWS
    if (iResult == SOCKET_ERROR)
    {
        // close the newly created socket
        OUTPUT_ERROR("Cannot set TCP server socket to listen mode")
        closesocket(interfaceSocket);
        return false;
    }
#else
    if (iResult != 0)
    {
        // close the newly created socket
        close(interfaceSocket);
        return false;
    }
#endif

    // create socket list for data connections
    maxConnections = connections;
    clientConnections = new TCPInterfaceClient_t[maxConnections];

    if (!clientConnections)
    {
#ifdef USE_WINDOWS
        // close the newly created socket
        OUTPUT_ERROR("Cannot create list for incoming connections")
        closesocket(interfaceSocket);
        return false;
#else
        // close the newly created socket
        OUTPUT_ERROR("Cannot create list for incoming connections")
        close(interfaceSocket);
        return false;
#endif
    }

    // set up socket list for later use
    for (auto i = 0; i < maxConnections; i++)
    {
#ifdef USE_WINDOWS
        clientConnections[i].socket = INVALID_SOCKET;
#else
        clientConnections[i].socket = -1;
#endif
    }

    // debug
    OUTPUT_DEBUG_MESSAGE("Initializing TCP server communication done")

    return true;
}

void TCPServerInterface::Shutdown()
{
    // delete data sockets
    if (clientConnections)
    {
        // close data sockets
        for (auto i = 0; i < maxConnections; i++)
        {
#ifdef USE_WINDOWS
            if (clientConnections[i].socket != INVALID_SOCKET)
            {
                closesocket(clientConnections[i].socket);
            }
#else
            if (clientConnections[i].socket >= 0)
            {
                close(clientConnections[i].socket);
            }
#endif
        }

        // delete sockets
        delete clientConnections;
        clientConnections = 0;
    }

    // close listen socket
#ifdef USE_WINDOWS
    if (interfaceSocket != INVALID_SOCKET)
    {
        closesocket(interfaceSocket);
        interfaceSocket = INVALID_SOCKET;
    }
#else
    if (interfaceSocket >= 0)
    {
        close(interfaceSocket);
        interfaceSocket = -1;
    }
#endif

    return;
}

int TCPServerInterface::Accept()
{
    // variables
    int slot = -1;
    int len = sizeof(sockaddr);
    FD_SET readSet;
    struct timeval tv;

    // call select to check if there is something to do
    // clear set
    FD_ZERO(&readSet);

    // set listen socket
    FD_SET(interfaceSocket, &readSet);

    // timeout
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    // select
#ifdef USE_WINDOWS
    auto aResult = select(0, &readSet, NULL, NULL, &tv);
    if (aResult == SOCKET_ERROR)
    {
        return -1;
    }
#else
    // find maximum socket
    auto aResult = select(interfaceSocket + 1, &readSet, NULL, NULL, &tv)
    if (result < 0)
    {
        return -1;
    }
#endif

    // check if listen socket is set
    if (!FD_ISSET(interfaceSocket, &readSet))
    {
        return -1;
    }

    // search for free slot in list
    for (auto i = 0; i < maxConnections; i++)
    {
#ifdef USE_WINDOWS
        if (clientConnections[i].socket == INVALID_SOCKET)
#else
        if (clientConnections[i].socket < 0)
#endif
        {
            slot = i;
            break;
        }
    }

    // check if we found a slot
    if (slot < 0)
    {
        return -1;
    }

    // wait for new connection to come in, should work for both OS. should be non-blocking now
    auto iResult = accept(interfaceSocket, (sockaddr*)&serverAddr, &len);

#ifdef USE_WINDOWS
    if (iResult == SOCKET_ERROR)
#else
    if (iResult != 0)
#endif
    {
        return -1;
    }

    // store new connection in free slot
    clientConnections[slot].socket = iResult;

    // increase number of connections
    connectionCount++;

    // debug
    OUTPUT_INFO("New Connection incoming on slot " << slot)

    return slot;
}

void TCPServerInterface::CloseConnection(int connection)
{
    // check if the connection id is within range
    if (connection >= maxConnections || connection < 0)
    {
        return;
    }

    // check if the connection existed
#ifdef USE_WINDOWS
    if (clientConnections[connection].socket == INVALID_SOCKET)
#else
    if (clientConnections[connection].socket == -1)
#endif
    {
        return;
    }

    // close socket
#ifdef USE_WINDOWS
    closesocket(clientConnections[connection].socket);
    clientConnections[connection].socket = INVALID_SOCKET;
#else
    close(clientConnections[connection].socket);
    clientConnections[connection].socket = -1;
#endif

    // decrease connection count
    connectionCount--;

    return;
}

bool TCPServerInterface::Send(int connection, char* data, int length)
{
    // variables
    int result;

    // check if the connection id is within range
    if (connection >= maxConnections || connection < 0)
    {
        return false;
    }

    // check if the connection exists
#ifdef USE_WINDOWS
    if (clientConnections[connection].socket == INVALID_SOCKET)
#else
    if (clientConnections[connection].socket == -1)
#endif
    {
        return false;
    }

    // sending data, should work with both OS
    result = send(clientConnections[connection].socket, data, length, 0);
    if (result <= 0)
    {
        // something went wrong, remove socket from list and notify calling function
        CloseConnection(connection);
        return false;
    }

    return true;
}

bool TCPServerInterface::SendData(char* buffer, int length)
{
    // variables
    bool result;

    // send data to all connections
    for (auto i = 0; i < maxConnections; i++)
    {
#ifdef USE_WINDOWS
        if (clientConnections[i].socket != INVALID_SOCKET)
#else
        if (clientConnections[connection].socket != -1)
#endif
        {
            result = Send(i, buffer, length);
            if (!result)
            {
                return false;
            }
        }
    }

    return true;
}

int TCPServerInterface::RecvData(char* buffer, int length)
{
    // cannot be solved, return -1
    return -1;
}

bool TCPServerInterface::Recv(int connection, char* buffer, int size)
{
    // variables
    int rest = size;
    int bytes;
    int loopCounter = 0;

    // check if the connection id is within range
    if (connection >= maxConnections || connection < 0)
    {
        return false;
    }

    // check if the connection exists
#ifdef USE_WINDOWS
    if (clientConnections[connection].socket == INVALID_SOCKET)
#else
    if (clientConnections[connection].socket == -1)
#endif
    {
        return false;
    }

    // loop until rest is zero
    while (rest > 0)
    {
        // check loop counter
        if (loopCounter > TCP_MAX_READ_ITERATIONS)
        {
            return false;
        }

        // read bytes
        bytes = RecvBytes(connection, buffer, rest);

        // check if some bytes were read
        if (bytes == 0)
        {
            loopCounter++;
            continue;
        }

        if (bytes < 0)
        {
            return false;
        }

        // decrease rest of bytes
        rest -= bytes;
        buffer += bytes;
    }

    return true;
}

int TCPServerInterface::GetConnectionWithData()
{
    // variables
    FD_SET readSet;

    // clear set
    FD_ZERO(&readSet);

    // append possible sockets
    for (auto i = 0; i < maxConnections; i++)
    {
#ifdef USE_WINDOWS
        if (clientConnections[i].socket != INVALID_SOCKET)
#else
        if (clientConnections[i].socket != -1)
#endif
        {
            FD_SET(clientConnections[i].socket, &readSet);
        }
    }

    // call select
#ifdef USE_WINDOWS
    auto result = select(0, &readSet, NULL, NULL, NULL);
    if (result == SOCKET_ERROR)
    {
        return -2;
    }
#else
    // find maximum socket
    int max = -1;
    for (auto i = 0; i < maxConnections; i++)
    {
        if (max < clientConnections[i].socket)
        {
            max = clientConnections[i].socket;
        }
    }

    // return -1 if no connection is valid
    if (max < 0)
    {
        return -2;
    }

    // call select
    auto result = select(max + 1, &readSet, NULL, NULL, NULL);
    if (result  < 0)
    {
        return -2;
    }
#endif

    // check if any socket is in the read set list
    for (auto i = 0; i < maxConnections; i++)
    {
        if (FD_ISSET(clientConnections[i].socket, &readSet))
        {
            return i;
        }
    }

    return -1;
}

int TCPServerInterface::RecvBytes(int connection, char* buffer, int length)
{
    // variables
    int result;

    //recv data
    result = recv(clientConnections[connection].socket, buffer, length, 0);
    if (result <= 0)
    {
        // something went wrong, notify calling function
        CloseConnection(connection);
        return -1;
    }

    return result;
}