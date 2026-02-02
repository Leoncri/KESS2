#pragma once

class GridServer;

#include <thread>
#include <chrono>
#include <functional>
#include <mutex>

#include "../config.h"
#include "../Communication/servercommunicationdefines.h"
#include "../Network/servercommunicationclass.h"
#include "../eventhandler.h"
#include "gridfile.h"
#include "gridhandler.h"
#include "../Auxillary/backgroundtask.h"

#define SUBSYSTEM_GRIDFILE          0x1
#define SUBSYSTEM_GRIDHANDLER       0x2

class GridServer
{
public:
    // constructor and destructor
    GridServer();
    ~GridServer();

    // init and shutdown
    bool Initialize();
    void Shutdown();

    // start server
    bool StartServer();
    void StopServer();

    // new command and connection handling
    void NewCommand(char* data, int size);
    void ConnectionClosed(uint16_t connection);

    // send data
    void SendData(char* buffer, int size);

    // function for asking for update function
    void SubsystemProcess(uint32_t subsystem);

    // Periodic thread functions
    void UpdateThreadFunction();
    void PeriodicDataThreadFunction();
    void BackgroundThreadFunction();

private:
    // functions
    void ProcessCommand(char* buffer, int size);

    // start and stop threads
    bool StartThreads();
    void StopThreads();

    // start and stop grid
    bool StartGrid();
    void StopGrid();

    bool IsDeviceType(uint8_t type, std::initializer_list<uint8_t> list);

    void SendRespondPacket(uint16_t connection, uint32_t id, uint32_t respond);
    void SendServerStatus();

private:
    // Event handler
    EventHandler* eventHandler;

    // grid file and grid handler
    GridFile* gridFile;
    GridHandler* gridHandler;

    // communication class
    ServerCommunication* serverCommunication;

    // thread objects
    std::thread updateThread;
    std::thread periodicDataThread;
    std::thread backgroundThread;
    bool updateThreadRunning;
    bool periodicDataThreadRunning;
    bool backgroundThreadRunning;
    bool stopThreads;

    // send and recv buffer
    std::mutex sendBufferLock;
    char recvBuffer[MAX_BUFFER_SIZE];
    char sendBuffer[MAX_BUFFER_SIZE];
    char pStatusBuffer[MAX_BUFFER_SIZE];

    // variables
    bool gridStarted;
    int gridFileVersion;
};

void UpdateThreadCallbackFunction(GridServer* server);
void PeriodicDataThreadCallbackFunction(GridServer* server);
void BackgroundThreadCallbackFunction(GridServer* server);