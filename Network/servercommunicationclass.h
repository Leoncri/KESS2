#pragma once

class ServerCommunication;

#include <thread>
#include <mutex>
#include <chrono>
#include <functional>

#include "../config.h"
#include "../Communication/genericcommunicationdefines.h"

#include "../Network/tcpserverinterface.h"
#include "../Grid/gridserver.h"

#define SERVER_COMM_FLAG_SEND_TO_ALL    0xFFFF

class ServerCommunication
{
public:
    // Constructor and destructor
    ServerCommunication();
    ~ServerCommunication();

    // Initialize and shutdown
    bool Initialize(GridServer* server);
    void Shutdown();

    // Start and stop network threads
    bool StartThreads();
    void StopThreads();

    // Thread functions to run
    void ListenThread();
    void RecvThread();

    // Send data
    bool SendData(char* buffer, int size);

private:
    // functions
    void ProcessRecvPacket();
    void SetupIsAlivePacket(uint16_t connection);
    void SetupUnknownTypePacket(uint16_t connection);

private:
    // thread objects
    std::thread listenThread;
    std::thread recvThread;
    std::mutex sendLock;

    // thread variables
    bool listenThreadRunning;
    bool recvThreadRunning;
    bool stopThreads;

    // recv variables
    char recvBuffer[MAX_BUFFER_SIZE];

    // send variables
    char sendBuffer[MAX_BUFFER_SIZE];

    // tcp interface
    TCPServerInterface* tcpInterface;
    bool tcpConnections[SERVER_NUM_COMM_SOCKETS];
    
    // grid server
    GridServer* gridServer;
};

void ServerCommunicationListenThread(ServerCommunication* comm);
void ServerCommunicationRecvThread(ServerCommunication* comm);