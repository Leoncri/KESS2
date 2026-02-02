#include "servercommunicationclass.h"
#include "../debugoutput.h"

ServerCommunication::ServerCommunication()
{
    stopThreads = true;
    listenThreadRunning = false;
    recvThreadRunning = false;

    tcpInterface = 0;

    gridServer = 0;

    for (auto i = 0; i < SERVER_NUM_COMM_SOCKETS; i++)
    {
        tcpConnections[i] = false;
    }
}

ServerCommunication::~ServerCommunication()
{
}

bool ServerCommunication::Initialize(GridServer* server)
{
    // variables
    bool bResult;

    // debug stuff
    OUTPUT_DEBUG_MESSAGE("Start initializing server communication")

    // initialize TCP Interface
    tcpInterface = new TCPServerInterface();
    if (!tcpInterface)
    {
        return false;
    }

    bResult = tcpInterface->Initialize(SERVER_PORT, SERVER_NUM_COMM_SOCKETS);
    if (!bResult)
    {
        return false;
    }

    // store grid server here
    gridServer = server;

    // debug
    OUTPUT_DEBUG_MESSAGE("Server communication initialized")

    return true;
}

void ServerCommunication::Shutdown()
{
    // check if threads are still running
    if (listenThreadRunning || recvThreadRunning)
    {
        // stop threads
        OUTPUT_INFO("Shutdown of server communication without stopping threads - stopping threads now...")
        StopThreads();
        OUTPUT_INFO("Threads stopped")
    }

    // shutdown TCP interface
    if (tcpInterface)
    {
        tcpInterface->Shutdown();
    }

    gridServer = 0;

    return;
}

bool ServerCommunication::StartThreads()
{
    // check if threads are already running
    if (listenThreadRunning || recvThreadRunning)
    {
        return false;
    }
    stopThreads = false;

    // create recv thread element
    recvThread = std::thread(ServerCommunicationRecvThread, this);

    // create listen thread element
    listenThread = std::thread(ServerCommunicationListenThread, this);

    return true;
}

void ServerCommunication::StopThreads()
{
    // stop
    stopThreads = true;

    // wait for threads to stop
    recvThread.join();
    listenThread.join();

    return;
}

void ServerCommunication::ListenThread()
{
    // variables
    int connection;

    // set thread to running
    listenThreadRunning = true;

    // debug
    OUTPUT_DEBUG_MESSAGE("Server communication listen thread started.")

    // thread loop
    while(!stopThreads)
    {
        // Accept new data connection
        connection = tcpInterface->Accept();

        // check if new connection was incoming
        if (connection == -1)
        {
            // continue
            continue;
        }

        // store connection, connection is already added to internal tcpInterface list
        tcpConnections[connection] = true;

        // debug
        OUTPUT_DEBUG_MESSAGE("New incoming connection")
    }

    // set thread to stopped
    listenThreadRunning = false;

    // debug
    OUTPUT_DEBUG_MESSAGE("Server communication listen thread stopped.")

    return;
}

void ServerCommunication::RecvThread()
{
    // variables
    int connection;
    GenericPacketHeader_t* header;
    int length;
    bool result;

    // set thread to running
    recvThreadRunning = true;

    // debug
    OUTPUT_DEBUG_MESSAGE("Server communication recv thread started.")

    // thread loop
    while(!stopThreads)
    {
        // get connection with some new data
        connection = tcpInterface->GetConnectionWithData();
        if (connection < 0)
        {
            // nothing to do here, wait and continue
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // get new data
        result = tcpInterface->Recv(connection, recvBuffer, sizeof(GenericPacketHeader_t));
        if(!result)
        {
            // something went wrong, remove connection from list
            OUTPUT_DEBUG_MESSAGE("Connection closed.")
            gridServer->ConnectionClosed(connection);
            tcpInterface->CloseConnection(connection);
            tcpConnections[connection] = false;
            continue;
        }

        header = (GenericPacketHeader_t*)recvBuffer;

        // get length from header and read in rest of packet
        length = header->length;
        if (length > MAX_BUFFER_SIZE)
        {
            // TODO: close connection for now as buffer overflow might occur
            OUTPUT_DEBUG_MESSAGE("Connection closed.")
            tcpInterface->CloseConnection(connection);
            tcpConnections[connection] = false;
            continue;
        }
        tcpInterface->Recv(connection, recvBuffer + sizeof(GenericPacketHeader_t), length - sizeof(GenericPacketHeader_t));

        // set conenction bits in header
        header->connection = 0x1 << connection;

        // process packet
        ProcessRecvPacket();
    }

    // set thread to stopped
    recvThreadRunning = false;

    // debug
    OUTPUT_DEBUG_MESSAGE("Server communication recv thread stopped.")

    return;
}

bool ServerCommunication::SendData(char* buffer, int size)
{
    // variables
    int rest = size;

    // get lock
    std::lock_guard lk(sendLock);

    // loop over all packets
    while (rest != 0)
    {
        // variables
        GenericPacketHeader_t* header = (GenericPacketHeader_t*)buffer;
        int pSize;
        int connection;
        bool result;

        // check if there is something to send
        if (!header->connection)
        {
            break;
        }

        // determine size and interface
        pSize = header->length;
        connection = header->connection;

        // send data to connections
        for (auto i = 0; i < SERVER_NUM_COMM_SOCKETS; i++)
        {
            // check if connection is available
            if ((header->connection & (0x1 << i)) && tcpConnections[i])
            {
                result = tcpInterface->Send(i, buffer, pSize);
                if (!result)
                {
                    // something went wrong, remove connection
                    tcpInterface->CloseConnection(i);
                    tcpConnections[i] = false;
                }
            }
        }

        // increase buffer and process next packet
        buffer += pSize;
        rest -= pSize;
    }

    return true;
}

void ServerCommunication::ProcessRecvPacket()
{
    // variables
    GenericPacketHeader_t* header = (GenericPacketHeader_t*)recvBuffer;
    uint8_t packetType, deviceType;

    // get packet type and device
    packetType = header->packetType;
    deviceType = header->deviceType;

    // process packets
    switch(packetType)
    {
        case PACKET_TYPE_ISALIVE:
        {
            // is alive packet, send back
            OUTPUT_DEBUG_MESSAGE("Incoming IsAlive packet")
            SetupIsAlivePacket(header->connection);
            break;
        }
        case PACKET_TYPE_COMMAND:
        {
            // command packet, notify server
            gridServer->NewCommand(recvBuffer, header->length);
            break;
        }
        default:
        {
            // no useful type
            SetupUnknownTypePacket(header->connection);
        }
    }

    return;
}

void ServerCommunication::SetupIsAlivePacket(uint16_t connection)
{
    // variables
    IsAlivePacket_t* isAlive = (IsAlivePacket_t*)sendBuffer;

    // generate packet
    isAlive->header.packetType = PACKET_TYPE_ISALIVE;
    isAlive->header.deviceType = DEVICE_TYPE_NONE;
    isAlive->header.deviceId = 0;
    isAlive->header.length = sizeof(IsAlivePacket_t);
    isAlive->header.connection = connection;
    isAlive->rsvd0 = 0;
    isAlive->rsvd1 = 0;

    // send packet
    SendData(sendBuffer, sizeof(IsAlivePacket_t));

    return;
}

void ServerCommunication::SetupUnknownTypePacket(uint16_t connection)
{
    // variables
    ErrorPacket_t* error = (ErrorPacket_t*)sendBuffer;

    // generate packet
    error->header.packetType = PACKET_TYPE_ERROR;
    error->header.deviceType = DEVICE_TYPE_NONE;
    error->header.deviceId = 0;
    error->header.length = sizeof(ErrorPacket_t);
    error->header.connection = connection;
    error->error = ERROR_TYPE_UNSUPPORTED;
    error->rsvd0 = 0;

    // send packet
    SendData(sendBuffer, sizeof(ErrorPacket_t));

    return;
}

void ServerCommunicationListenThread(ServerCommunication* comm)
{
    // call listen thread function
    comm->ListenThread();

    return;
}

void ServerCommunicationRecvThread(ServerCommunication* comm)
{
    // call recv thread function
    comm->RecvThread();

    return;
}